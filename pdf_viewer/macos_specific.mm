#include <AppKit/AppKit.h>
#include <QWidget>
#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>

AVAudioPlayer* current_player = nil;
extern "C" void changeTitlebarColor(WId winId, double red, double green, double blue, double alpha){
    if (winId == 0) return;
    NSView* view = (NSView*)winId;
    NSWindow* window = [view window];
    window.titlebarAppearsTransparent = YES;
    window.backgroundColor = [NSColor colorWithRed:red green:green blue:blue alpha: alpha];
}

@interface DraggableTitleView : NSView
@end

@implementation DraggableTitleView

// Handle mouse click events
- (void)mouseDown:(NSEvent *)event {
    // double-click to zoom
    if ([event clickCount] == 2) {
        [self.window zoom:nil];
    } else {
        // drag Window
        [self.window performWindowDragWithEvent:event];
    }
}

- (void)updateTrackingAreas {
    [self initTrackingArea];
}

-(void) initTrackingArea {
    NSTrackingAreaOptions options = (NSTrackingActiveAlways | NSTrackingInVisibleRect |
            NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved);

    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:[self bounds]
        options:options
        owner:self
        userInfo:nil];

    [self addTrackingArea:area];
}
-(void)mouseEntered:(NSEvent *)event {
    if((self.window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [[self.window standardWindowButton: NSWindowCloseButton] setHidden:NO];
        [[self.window standardWindowButton: NSWindowMiniaturizeButton] setHidden:NO];
        [[self.window standardWindowButton: NSWindowZoomButton] setHidden:NO];
    }
}

-(void)mouseExited:(NSEvent *)event {
    if((self.window.styleMask & NSWindowStyleMaskFullScreen) == 0) {
        [[self.window standardWindowButton: NSWindowCloseButton] setHidden:YES];
        [[self.window standardWindowButton: NSWindowMiniaturizeButton] setHidden:YES];
        [[self.window standardWindowButton: NSWindowZoomButton] setHidden:YES];
    }
}

@end

extern "C" void hideWindowTitleBar(WId winId) {
    if (winId == 0) return;

    NSView* nativeView = reinterpret_cast<NSView*>(winId);
    NSWindow* nativeWindow = [nativeView window];

    if(nativeWindow.titleVisibility == NSWindowTitleHidden){
        return;
    }

    [[nativeWindow standardWindowButton: NSWindowCloseButton] setHidden:YES];
    [[nativeWindow standardWindowButton: NSWindowMiniaturizeButton] setHidden:YES];
    [[nativeWindow standardWindowButton: NSWindowZoomButton] setHidden:YES];
    NSRect contentViewBounds = nativeWindow.contentView.bounds;

    DraggableTitleView *titleBarView = [[DraggableTitleView alloc] initWithFrame:NSMakeRect(0, 0, contentViewBounds.size.width, 22)];
    titleBarView.autoresizingMask = NSViewWidthSizable;
    titleBarView.wantsLayer = YES;
    titleBarView.layer.backgroundColor = [[NSColor clearColor] CGColor];

    [nativeWindow.contentView addSubview:titleBarView];

    [nativeWindow setTitleVisibility:NSWindowTitleHidden];
    [nativeWindow setStyleMask:[nativeWindow styleMask] | NSWindowStyleMaskFullSizeContentView];
    [nativeWindow setTitlebarAppearsTransparent:YES];
}

extern "C" void macos_setMp3FileSource(const char* path) {
    // plays the mp3 file using AVAudioPlayer
    NSString* nsPath = [NSString stringWithUTF8String:path];

    NSURL* url = [NSURL fileURLWithPath:nsPath];
    if (current_player == nil){
        current_player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:nil];
    }
    else{
        float oldRate = current_player.rate;
        [current_player stop];
        current_player = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:nil];
        current_player.enableRate = YES;
        current_player.rate = oldRate;
    }
    current_player.enableRate = YES;
}

extern "C" void macos_playMp3File(const char* path) {
    macos_setMp3FileSource(path);
    [current_player play];
}

extern "C" bool macos_isMp3Playing() {
    return current_player != nil && current_player.isPlaying;
}

extern "C" void macos_stopMp3File() {
    if (current_player != nil) {
        [current_player stop];
    }
}

extern "C" void macos_seekMp3File(float seconds){
    if (current_player != nil) {
        current_player.currentTime = seconds;
    }
}

extern "C" float macos_getMp3CurrentTime(){
    if (current_player != nil) {
        return current_player.currentTime;
    }
    return 0;
}

extern "C" float macos_pauseMp3File(){
    if (current_player != nil) {
        [current_player pause];
        return current_player.currentTime;
    }
    return 0;
}

extern "C" void macos_resumeMp3File(){
    if (current_player != nil) {
        [current_player play];
    }
}

extern "C" float macos_getMp3Duration(){
    if (current_player){
        return current_player.duration;
    }
    return 0;
}

extern "C" void macos_setPlaybackRate(float rate){
    if (current_player != nil) {
        current_player.rate = rate;
    }
}
extern "C" bool macos_isMp3Finished(){
    if (current_player != nil) {
        return current_player.currentTime >= current_player.duration;
    }
    return false;
}
