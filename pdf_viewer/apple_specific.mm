#include <QWidget>
#import <AVFoundation/AVFoundation.h>
#import <MediaPlayer/MediaPlayer.h>

// Define the callback function pointer type
typedef void (*AudioFinishedCallback)(void* data);

#ifdef Q_OS_IOS
static void setupAudioSessionAndRemoteCommands();
static void updateNowPlayingInfo();
static bool remoteCommandsInitialized = false;
#endif

// Global variable to store the callback function
static AudioFinishedCallback g_audioFinishedCallback = NULL;
static void* g_audioFinishedData = 0;

QString notification_title = "";

AVAudioPlayer* current_player = nil;


// Delegate class to handle audio player events
@interface AudioPlayerDelegateHandler : NSObject <AVAudioPlayerDelegate>
@end

@implementation AudioPlayerDelegateHandler

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag {
    if (flag) { // Check 'flag' for successful completion
#ifdef Q_OS_IOS
        updateNowPlayingInfo(); // Update lock screen info to reflect finished state
#endif
        if (g_audioFinishedCallback) {
            g_audioFinishedCallback(g_audioFinishedData);
        }
    }
}

@end

// Global instance of the delegate handler
static AudioPlayerDelegateHandler* g_audioDelegateHandler = nil;

// Function to set the audio finished callback
extern "C" void macos_setAudioFinishedCallback(AudioFinishedCallback callback, void* data) {
    g_audioFinishedCallback = callback;
    g_audioFinishedData = data;
}

extern "C" void macos_setMp3FileSource(const char* path, const char* notification_text) {
    notification_title = notification_text;

#ifdef Q_OS_IOS
    // Setup audio session and remote commands once, if not already done
    if (!remoteCommandsInitialized) {
        setupAudioSessionAndRemoteCommands();
    }
#endif
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
#ifdef Q_OS_IOS
    updateNowPlayingInfo(); // Update Now Playing info for the new track (initially not playing)
#endif
}

extern "C" bool macos_isMp3Playing() {
    return current_player != nil && current_player.isPlaying;
}

extern "C" void macos_stopMp3File() {
    if (current_player != nil) {
        [current_player stop];
#ifdef Q_OS_IOS
        updateNowPlayingInfo();
#endif
    }
}

extern "C" void macos_seekMp3File(float seconds){
    if (current_player != nil) {
        current_player.currentTime = seconds;
#ifdef Q_OS_IOS
        updateNowPlayingInfo();
#endif
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
#ifdef Q_OS_IOS
        updateNowPlayingInfo();
#endif
        return current_player.currentTime;
    }
    return 0;
}

extern "C" void macos_resumeMp3File(){
    if (current_player != nil) {
        [current_player play];
#ifdef Q_OS_IOS
        updateNowPlayingInfo();
#endif
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
#ifdef Q_OS_IOS
        updateNowPlayingInfo();
#endif
    }
}

extern "C" bool macos_isMp3Finished(){
    if (current_player != nil) {
        return current_player.currentTime >= current_player.duration;
    }
    return false;
}

#ifdef Q_OS_IOS
// Static helper to update Now Playing information
static void updateNowPlayingInfo() {
    if (current_player == nil || current_player.url == nil) {
        // Clear Now Playing info if there's no player or no URL
        [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:nil];
        return;
    }

    NSMutableDictionary *nowPlayingInfo = [NSMutableDictionary dictionary];
    // Extract filename from path for title
//    NSString *filePath = [NSString stringWithUTF8String:current_player.url.path.UTF8String];
//    NSString* notificationText = QString().tons
    
    nowPlayingInfo[MPMediaItemPropertyTitle] = notification_title.toNSString();
    nowPlayingInfo[MPMediaItemPropertyMediaType] = @(MPMediaTypeMusic);
    nowPlayingInfo[MPMediaItemPropertyPlaybackDuration] = @(current_player.duration);
    nowPlayingInfo[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(current_player.currentTime);

    if (current_player.isPlaying) {
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackRate] = @(current_player.rate);
    } else {
        nowPlayingInfo[MPNowPlayingInfoPropertyPlaybackRate] = @(0.0); // Explicitly 0 when not playing/paused
        // If finished, ensure elapsed time is reported as duration
        if (current_player.currentTime >= current_player.duration) {
             nowPlayingInfo[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(current_player.duration);
        }
    }

    [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:nowPlayingInfo];
}


// Function to setup audio session and remote command center for iOS
static void setupAudioSessionAndRemoteCommands() {
    if (remoteCommandsInitialized) {
        return;
    }

    // Configure Audio Session for playback
    AVAudioSession *session = [AVAudioSession sharedInstance];
    NSError *error = nil;
    if (![session setCategory:AVAudioSessionCategoryPlayback error:&error]) {
        NSLog(@"Error setting audio session category: %@", error);
    }
    // It's good practice to handle activation errors, though for simplicity here, we omit some.
    if (![session setActive:YES error:&error]) {
        NSLog(@"Error activating audio session: %@", error);
    }

    MPRemoteCommandCenter *commandCenter = [MPRemoteCommandCenter sharedCommandCenter];

    // Play command
    [commandCenter.playCommand setEnabled:YES];
    [commandCenter.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        if (current_player && !current_player.isPlaying) {
            macos_resumeMp3File(); // This function calls updateNowPlayingInfo
        }
        return MPRemoteCommandHandlerStatusSuccess;
    }];

    // Pause command
    [commandCenter.pauseCommand setEnabled:YES];
    [commandCenter.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        if (current_player && current_player.isPlaying) {
            macos_pauseMp3File(); // This function calls updateNowPlayingInfo
        }
        return MPRemoteCommandHandlerStatusSuccess;
    }];

    // Stop command
    [commandCenter.stopCommand setEnabled:YES];
    [commandCenter.stopCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        if (current_player) {
            macos_stopMp3File(); // This function calls updateNowPlayingInfo
        }
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    // Toggle Play/Pause command
    [commandCenter.togglePlayPauseCommand setEnabled:YES];
    [commandCenter.togglePlayPauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        if (current_player) {
            if (current_player.isPlaying) {
                macos_pauseMp3File();
            } else {
                macos_resumeMp3File();
            }
        }
        return MPRemoteCommandHandlerStatusSuccess;
    }];

    // ChangePlaybackPosition command (scrubbing)
    [commandCenter.changePlaybackPositionCommand setEnabled:YES];
    [commandCenter.changePlaybackPositionCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        MPChangePlaybackPositionCommandEvent *positionEvent = (MPChangePlaybackPositionCommandEvent *)event;
        if (current_player) {
            macos_seekMp3File(positionEvent.positionTime); // This function calls updateNowPlayingInfo
        }
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    
    remoteCommandsInitialized = true;
}
#endif
