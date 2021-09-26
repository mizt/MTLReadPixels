#import "TargetConditionals.h"

#if TARGET_CPU_ARM64 && TARGET_OS_OSX

class MTLReadPixels {
    
    private:
        
        id<MTLDevice> _device = MTLCreateSystemDefaultDevice();
        id<MTLTexture> _texture;
        id<MTLBuffer> _clip;
        id<MTLComputePipelineState> _pipelineState;
        
        dispatch_semaphore_t _semaphore = dispatch_semaphore_create(0);
        
        unsigned int *_bytes;
        
        int _width;
        int _height;
        
        bool _isInit = false;
        
    public:
        
        MTLReadPixels(int w,int h, NSString *identifier=nil) {
            
            this->_width = w;
            this->_height = h;
            
            this->_bytes = new unsigned int[this->_width*this->_height];
            for(int k=0; k<this->_width*this->_height; k++) {
                this->_bytes[k] = 0xFF000000;
            }
            
    #ifdef TARGET_OS_OSX
            
            NSString *path;
            if(identifier==nil) {
                path = [[NSBundle mainBundle] bundlePath];
            }
            else {
                NSBundle *bundle = [NSBundle bundleWithIdentifier:@"jit.metal.map"];
                path = [bundle resourcePath];
            }
            
            NSString *metallib = [NSString stringWithFormat:@"%@/%s",path,"copy-macosx.metallib"];
            
    #else
            
            NSString *metallib = [[NSBundle mainBundle] pathForResource:@"copy-iphoneos" ofType:@"metallib"];
            
    #endif
            
            NSError *err = nil;
            NSFileManager *fileManager = [NSFileManager defaultManager];
            [fileManager attributesOfItemAtPath:metallib error:&err];
            
            if(!err) {
                
                dispatch_fd_t fd = open([metallib UTF8String],O_RDONLY);
                
                NSDictionary *attributes = [fileManager attributesOfItemAtPath:metallib error:&err];
                long size = [[attributes objectForKey:NSFileSize] integerValue];
                NSLog(@"%ld",size);
                
                if(size>0) {
                    
                    dispatch_read(fd,size,dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT,0),^(dispatch_data_t d,int e) {
                        
                        NSError *err = nil;
                        id<MTLLibrary> library = [this->_device newLibraryWithData:d error:&err];
                        
                        if(!err) {
                            
                            MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:this->_width height:this->_height mipmapped:NO];
                            desc.usage = MTLTextureUsageShaderRead|MTLTextureUsageShaderWrite;
                            this->_texture = [this->_device newTextureWithDescriptor:desc];
                            
                            this->_clip = [this->_device newBufferWithLength:sizeof(unsigned int)*2 options:MTLResourceOptionCPUCacheModeDefault];
                            unsigned int *clipContents = (unsigned int *)[this->_clip contents];
                            clipContents[0] = this->_width;
                            clipContents[1] = this->_height;
                            id<MTLFunction> function = [library newFunctionWithName:@"copy"];
                            this->_pipelineState = [this->_device newComputePipelineStateWithFunction:function error:nil];
                            if(!err) {
                                this->_isInit = true;
                                NSLog(@"load copy.metallib");
                            }
                        }
                        dispatch_semaphore_signal(this->_semaphore);
                        close(fd);
                    });
                    dispatch_semaphore_wait(this->_semaphore,DISPATCH_TIME_FOREVER);
                    
                }
            }
        }
        
        ~MTLReadPixels() {
            delete[] this->_bytes;
        }
        
        unsigned int *bytes() {
            return this->_bytes;
        }
        
        void setDrawableTexture(id<MTLTexture> src,int type=1) {
            
            if(type>=1&&this->_isInit) {
                id<MTLCommandQueue> queue = [this->_device newCommandQueue];
                id<MTLCommandBuffer> commandBuffer = queue.commandBuffer;
                id<MTLComputeCommandEncoder> encoder = commandBuffer.computeCommandEncoder;
                [encoder setComputePipelineState:this->_pipelineState];
                [encoder setTexture:src atIndex:0];
                [encoder setTexture:this->_texture atIndex:1];
                [encoder setBuffer:this->_clip offset:0 atIndex:0];
                MTLSize threadGroupSize = MTLSizeMake(8,8,1);
                MTLSize threadGroups = MTLSizeMake(
                    ceil(this->_width/(double)threadGroupSize.width),
                    ceil(this->_height/(double)threadGroupSize.height),
                1);
                [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];
                [encoder endEncoding];
                [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
                    [this->_texture getBytes:this->_bytes bytesPerRow:this->_width<<2 fromRegion:MTLRegionMake2D(0,0,this->_width,this->_height) mipmapLevel:0];
                    dispatch_semaphore_signal(this->_semaphore);
                }];
                [commandBuffer commit];
                [commandBuffer waitUntilCompleted];
                dispatch_semaphore_wait(this->_semaphore,DISPATCH_TIME_FOREVER);
            }
            else {
                [src getBytes:this->_bytes bytesPerRow:(this->_width<<2) fromRegion:MTLRegionMake2D(0,0,this->_width,this->_height) mipmapLevel:0];
            }
            
            
        }
};

#else

class MTLReadPixels {
    
    private:
        
        unsigned int *_bytes;
        
        int _width;
        int _height;
        
    public:
        
        MTLReadPixels(int w,int h) {
            
            this->_width = w;
            this->_height = h;
            
            this->_bytes = new unsigned int[this->_width*this->_height];
            for(int k=0; k<this->_width*this->_height; k++) {
                this->_bytes[k] = 0xFF000000;
            }
        }
        
        void setDrawableTexture(id<MTLTexture> src) {
            [src getBytes:this->_bytes bytesPerRow:(this->_width<<2) fromRegion:MTLRegionMake2D(0,0,this->_width,this->_height) mipmapLevel:0];
        }
        
        ~MTLReadPixels() {
            delete[] this->_bytes;
        }
        
        unsigned int *bytes() {
            return this->_bytes;
        }
        
};

#endif
