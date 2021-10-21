#import "TargetConditionals.h"
#import "PixelBuffer.h"

template <typename T>
class MTLReadPixelsBasse {

    protected:
        
        PixelBuffer<T> *_buffer;
    
    public:
    
        std::string type() { return this->_buffer->type(); }
        int width() { return this->_buffer->width(); }
        int height() { return this->_buffer->height(); }
        int bpp() { return this->_buffer->bpp(); }
        void *bytes() { return this->_buffer->bytes(); }
        unsigned int rowBytes() { return this->_buffer->rowBytes(); }
        
        MTLReadPixelsBasse(int w,int h, int bpp=4) {
            this->_buffer = new PixelBuffer<T>(w,h,bpp);
        }
    
        ~MTLReadPixelsBasse() {
            delete this->_buffer;
        }
};

#if TARGET_CPU_ARM64 && TARGET_OS_OSX

template <typename T>
class MTLReadPixels : public MTLReadPixelsBasse<T> {

    private:
    
        bool _isInit = false;
        
        id<MTLDevice> _device = MTLCreateSystemDefaultDevice();
        id<MTLTexture> _texture;
        id<MTLBuffer> _clip;
        id<MTLComputePipelineState> _pipelineState;
            
        dispatch_semaphore_t _semaphore = dispatch_semaphore_create(0);
                
        MTLPixelFormat PixelFormat8Unorm = MTLPixelFormatRGBA8Unorm;

    public:
        
        MTLReadPixels(int w,int h, int bpp=4, NSString *identifier=nil) : MTLReadPixelsBasse<T>(w,h,bpp) {
            
    #ifdef TARGET_OS_OSX
            NSString *metallib = MTLUtils::path(@"copy-macosx.metallib",identifier);
    #else
            NSString *metallib = MTLUtils::path(@"copy-iphoneos.metallib",identifier);
    #endif
            
            NSError *err = nil;
            NSFileManager *fileManager = [NSFileManager defaultManager];
            [fileManager attributesOfItemAtPath:metallib error:&err];
            
            if(!err) {
                
                dispatch_fd_t fd = open([metallib UTF8String],O_RDONLY);
                    
                NSDictionary *attributes = [fileManager attributesOfItemAtPath:metallib error:&err];
                long size = [[attributes objectForKey:NSFileSize] integerValue];
                
                if(size>0) {
                    
                    dispatch_read(fd,size,dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0),^(dispatch_data_t d,int e) {
                    
                        NSError *err = nil;
                        id<MTLLibrary> library = [this->_device newLibraryWithData:d error:&err];
                                            
                        if(!err) {
                                                        
                            MTLTextureDescriptor *texDescriptor = nil;
                            
                            if(this->_buffer->type()=="f") {
                                if(bpp==4) {
                                    texDescriptor = MTLUtils::descriptor(MTLPixelFormatRGBA32Float,this->width(),this->height());
                                }
                                else if(bpp==2) {
                                    texDescriptor = MTLUtils::descriptor(MTLPixelFormatRG32Float,this->width(),this->height());
                                }
                                else if(bpp==1) {
                                    texDescriptor = MTLUtils::descriptor(MTLPixelFormatR32Float,this->width(),this->height());
                                }
                            }
                            else if(this->_buffer->type()=="S"&&bpp==4) {
                                texDescriptor = MTLUtils::descriptor(MTLPixelFormatRG16Unorm,this->width(),this->height());
                            }
                            else if(this->_buffer->type()=="I"&&bpp==4) {
                                texDescriptor = MTLUtils::descriptor(this->PixelFormat8Unorm,this->width(),this->height());
                            }
                            
                            if(texDescriptor) {
                                texDescriptor.usage = MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;
                                this->_texture = [this->_device newTextureWithDescriptor:texDescriptor];
                                this->_clip = MTLUtils::newBuffer(this->_device,sizeof(unsigned int)*2);
                                unsigned int *clipContents = (unsigned int *)[this->_clip contents];
                                clipContents[0] = this->width();
                                clipContents[1] = this->height();
                                id<MTLFunction> function = [library newFunctionWithName:@"copy"];
                                this->_pipelineState = [this->_device newComputePipelineStateWithFunction:function error:nil];
                                if(!err) {
                                    this->_isInit = true;
                                }
                            }
                        }
                        dispatch_semaphore_signal(this->_semaphore);
                        close(fd);
                    });
                    dispatch_semaphore_wait(this->_semaphore,DISPATCH_TIME_FOREVER);
                    
                }
            }
        }
    
        void *getBytes(id<MTLTexture> src, bool shader=true) {
                                    
            if(src) {
                if(shader&&this->_isInit) {
                                        
                    id<MTLCommandQueue> queue = [this->_device newCommandQueue];
                    id<MTLCommandBuffer> commandBuffer = queue.commandBuffer;
                    id<MTLComputeCommandEncoder> encoder = commandBuffer.computeCommandEncoder;
                    [encoder setComputePipelineState:this->_pipelineState];
                    [encoder setTexture:src atIndex:0];
                    [encoder setTexture:this->_texture atIndex:1];
                    [encoder setBuffer:this->_clip offset:0 atIndex:0];
                    
                    int w = this->width();
                    int h = this->height();
                    
                    int tx = 1;
                    int ty = 1;
                    
                    for(int k=1; k<5; k++) {
                        if(w%(1<<k)==0) tx = 1<<k;
                        if(h%(1<<k)==0) ty = 1<<k;
                    }
                    
                    MTLSize threadGroupSize = MTLSizeMake(tx,ty,1);
                    MTLSize threadGroups = MTLSizeMake(w/tx,h/ty,1);
                    
                    [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];
                    [encoder endEncoding];
                    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
                        
                        [this->_texture getBytes:this->_buffer->bytes() bytesPerRow:this->_buffer->rowBytes() fromRegion:MTLRegionMake2D(0,0,this->_buffer->width(),this->_buffer->height()) mipmapLevel:0];
                        dispatch_semaphore_signal(this->_semaphore);
                    }];
                    [commandBuffer commit];
                    [commandBuffer waitUntilCompleted];
                    
                    dispatch_semaphore_wait(this->_semaphore,DISPATCH_TIME_FOREVER);

                    return this->_buffer->bytes();
                }
                else {
                                        
                    [src getBytes:this->_buffer->bytes() bytesPerRow:this->_buffer->rowBytes() fromRegion:MTLRegionMake2D(0,0,this->_buffer->width(),this->_buffer->height()) mipmapLevel:0];
                    
                    return this->_buffer->bytes();
                }
            }
            
            return nullptr;
        }
};

#else

template <typename T>
class MTLReadPixels : public MTLReadPixelsBasse<T> {
    
    public:
    
        MTLReadPixels(int w,int h, int bpp=4) : MTLReadPixelsBasse<T>(w,h,bpp) {}
        
        void *getBytes(id<MTLTexture> src, bool shader=false) {
            if(src) {
                [src getBytes:this->_buffer->bytes() bytesPerRow:this->_buffer->rowBytes() fromRegion:MTLRegionMake2D(0,0,this->_buffer->width(),this->_buffer->height()) mipmapLevel:0];
                
                return this->_buffer->bytes();
            }
        
            return nullptr;
        }
        
        ~MTLReadPixels() {
        }
        
};

#endif
