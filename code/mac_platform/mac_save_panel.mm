
@interface LevelEditorSavePanelDelegate: NSObject<NSOpenSavePanelDelegate>
@end

@implementation LevelEditorSavePanelDelegate 

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
    return true;
}
@end
