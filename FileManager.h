namespace FileManager {
    
    bool exists(NSString *path) {
        static const NSFileManager *fileManager = [NSFileManager defaultManager];
        NSError *err = nil;
        [fileManager attributesOfItemAtPath:path error:&err];
        return (err)?false:true;
    }
    
    bool extension(NSString *str, NSString *ext) {
        return ([[str pathExtension] compare:ext]==NSOrderedSame)?true:false;
    }

    NSString *replace(NSString *str, NSString *from,NSString *to) {
        return [str stringByReplacingOccurrencesOfString:from withString:to];
    }
    
    NSString *replace(NSString *str,NSArray<NSString *> *from,NSString *to) {
        NSMutableString *mstr = [NSMutableString stringWithString:str];
        for(int n=0; n<[from count]; n++) {
            mstr.string = replace(mstr,from[n],to);
        }
        return [NSString stringWithString:mstr];
    }
    
    NSString *path(NSString *filename,NSString *identifier=nil) {
        if(identifier==nil) {
    #if TARGET_OS_OSX
            return [NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] bundlePath],filename];
    #else
            return [NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] resourcePath],filename];
    #endif
        }
        else {
            return [NSString stringWithFormat:@"%@/%@",[[NSBundle bundleWithIdentifier:identifier] resourcePath],filename];
        }
    }
}
