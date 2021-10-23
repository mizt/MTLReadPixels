namespace MTLUtils {

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

