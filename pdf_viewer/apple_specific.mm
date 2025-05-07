#include <QWidget>
#import <AVFoundation/AVFoundation.h>

// Define the callback function pointer type
typedef void (*AudioFinishedCallback)();

// Global variable to store the callback function
static AudioFinishedCallback g_audioFinishedCallback = NULL;

AVAudioPlayer* current_player = nil;

// Delegate class to handle audio player events
@interface AudioPlayerDelegateHandler : NSObject <AVAudioPlayerDelegate>
@end

@implementation AudioPlayerDelegateHandler

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag {
    if (flag && g_audioFinishedCallback) {
        g_audioFinishedCallback();
    }
}

@end

// Global instance of the delegate handler
static AudioPlayerDelegateHandler* g_audioDelegateHandler = nil;

// Function to set the audio finished callback
extern "C" void macos_setAudioFinishedCallback(AudioFinishedCallback callback) {
    g_audioFinishedCallback = callback;
}

extern "C" void macos_setMp3FileSource(const char* path) {
    // Initialize the delegate handler if it hasn't been already
    if (g_audioDelegateHandler == nil) {
        g_audioDelegateHandler = [[AudioPlayerDelegateHandler alloc] init];
    }

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
    // Set the delegate for the player
    current_player.delegate = g_audioDelegateHandler;
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
