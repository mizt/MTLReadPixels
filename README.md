# MTLReadPixels

For some reason [getBytes:bytesPerRow:fromRegion:mipmapLevel:](https://developer.apple.com/documentation/metal/mtltexture/1515751-getbytes?language=objc) for drawable's texture is very slow on M1 and A14, so copy drawable's texture by the compute shader and then get the bytes.

see also:  
[https://twitter.com/mogamogamachine/status/1389109666978811907](https://twitter.com/mogamogamachine/status/1389109666978811907)

#### macosx

    xcrun -sdk macosx metal -c copy.metal -o copy.air; xcrun -sdk macosx metallib copy.air -o copy-macosx.metallib

#### iphoneos

    xcrun -sdk iphoneos metal -c copy.metal -o copy.air; xcrun -sdk iphoneos metallib copy.air -o copy-iphoneos.metallib

#### iphonesimulator	

    xcrun -sdk iphonesimulator metal -c copy.metal -o copy.air; xcrun -sdk iphonesimulator metallib copy.air -o copy-iphonesimulator.metallib

#### remove copy.air

    rm ./copy.air


​	

