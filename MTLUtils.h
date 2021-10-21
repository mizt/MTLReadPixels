namespace MTLUtils {

    NSString *path(NSString *filename,NSString *identifier=nil) {
        
#ifdef TARGET_OS_OSX
        
        NSString *path;
        if(identifier==nil) {
            path = [[NSBundle mainBundle] bundlePath];
        }
        else {
            NSBundle *bundle = [NSBundle bundleWithIdentifier:identifier];
            path = [bundle resourcePath];
        }
        
        NSString *metallib = [NSString stringWithFormat:@"%@/%@",path,filename];
        
#else
        
        NSString *metallib = [[NSBundle mainBundle] pathForResource:[filename stringByDeletingPathExtension] ofType:@"metallib"];
        
#endif
        
        return metallib;
    }

    MTLTextureDescriptor *descriptor(MTLPixelFormat format, int w, int h) {
        return [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:format width:w height:h mipmapped:NO];
    }

    id<MTLBuffer> newBuffer(id<MTLDevice> device, long length, MTLResourceOptions options = MTLResourceOptionCPUCacheModeDefault) {
        return [device newBufferWithLength:length options:options];
    }

    void replace(id<MTLTexture> texture, void *data, int width, int height, int rowBytes) {
       [texture replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:data bytesPerRow:rowBytes];
    }
}

