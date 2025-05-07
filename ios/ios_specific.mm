#include <QWidget>
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#import <MediaPlayer/MediaPlayer.h>

static AVSpeechSynthesizer* synthesizer = nil;
static AVAudioPlayer* fileAudioPlayer = nil;
static int currentSpokenWordLocation = -1;

extern "C" void on_ios_file_picked(QString file_path);

extern "C" void iosResumeFunc(){
}

extern "C" void iosPauseFunc(){
}

extern "C" int getLastSpokenWordLocation(){
    return currentSpokenWordLocation;
}

extern "C" void iosStopReading(){
    if (synthesizer){
        if ([synthesizer isSpeaking]){
            [synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
            [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStateStopped;
            [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = nil;
        }
    }
}

@interface SynthesizerDelegate : NSObject <AVSpeechSynthesizerDelegate>
@end

@implementation SynthesizerDelegate

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didStartSpeechUtterance:(AVSpeechUtterance *)utterance{
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance *)utterance{
    [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStateStopped;
    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = nil;
}

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer 
    willSpeakRangeOfSpeechString:(NSRange)characterRange 
    utterance:(AVSpeechUtterance *)utterance{

    if (characterRange.location != NSNotFound){
        currentSpokenWordLocation = (int)characterRange.location;
    }
}

@end


extern "C" AVSpeechSynthesizer* createSpeechSynthesizer(){
    synthesizer = [[AVSpeechSynthesizer alloc] init];
    return synthesizer;
}


void setupNowPlaying(){
    [MPNowPlayingInfoCenter defaultCenter].playbackState = MPNowPlayingPlaybackStatePlaying;

    NSMutableDictionary* nowPlayingInfo = [NSMutableDictionary dictionary];
    [nowPlayingInfo setObject:@"sioyek tts" forKey:MPMediaItemPropertyTitle];
    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = nowPlayingInfo;
}


void setupRemoteCommandCenterForTTS(){
    MPRemoteCommandCenter* sharedCommandCenter = [MPRemoteCommandCenter sharedCommandCenter];
    // MPRemoteCommand* playCommand = [sharedCommandCenter playCommand];
    [sharedCommandCenter.playCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [synthesizer continueSpeaking];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    sharedCommandCenter.playCommand.enabled = true;

    [sharedCommandCenter.pauseCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent * _Nonnull event) {
        [synthesizer pauseSpeakingAtBoundary:AVSpeechBoundaryImmediate];
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    sharedCommandCenter.pauseCommand.enabled = true;

}

double convertQtRateToNativeRate(double qtRate){
    const float range = qtRate >= 0
                      ? AVSpeechUtteranceMaximumSpeechRate - AVSpeechUtteranceDefaultSpeechRate
                      : AVSpeechUtteranceDefaultSpeechRate - AVSpeechUtteranceMinimumSpeechRate;
    return AVSpeechUtteranceDefaultSpeechRate + (qtRate * range);
}

extern "C" void iosPlayTextToSpeechInBackground(NSString* text, NSString* voiceName, double rate){
    if (synthesizer == nil) {
        createSpeechSynthesizer();
    }

    NSString* ttsVoiceName = voiceName ? voiceName : @"en-GB";
//    NSString* ttsVoiceName = @"en-GB";
    AVSpeechSynthesisVoice* voice = [AVSpeechSynthesisVoice voiceWithLanguage:ttsVoiceName];
    NSArray<AVSpeechSynthesisVoice*>* availableVoices = [AVSpeechSynthesisVoice speechVoices];

    for (int i =0; i < availableVoices.count; i++){
        if ([voiceName isEqualToString:availableVoices[i].name]){
            voice = availableVoices[i];
        }
    }

    // NSString* ttsVoiceName = voiceName ? voiceName : @"en";
    SynthesizerDelegate* delegate = [[SynthesizerDelegate alloc] init];

    // AVSpeechSynthesisVoice *defaultAvVoice = [AVSpeechSynthesisVoice voiceWithLanguage:locale.bcp47Name().toNSString()];

    AVSpeechUtterance* utterance = [[AVSpeechUtterance alloc] initWithString:text];
    utterance.voice = voice;
    utterance.rate = convertQtRateToNativeRate(rate);
    synthesizer.delegate = delegate;
    [synthesizer speakUtterance:utterance];
    setupNowPlaying();
    setupRemoteCommandCenterForTTS();
    [utterance release];
}

typedef void (*PinchGestureCallback)(float x, float y, float scale, float velocity, int state);
static PinchGestureCallback gPinchCallback = nullptr;

@interface PinchGestureDelegate : NSObject <UIGestureRecognizerDelegate>
@end

@implementation PinchGestureDelegate

- (void)handlePinch:(UIPinchGestureRecognizer*)sender {
    int numTouches = (int)[sender numberOfTouches];
    CGPoint centerPoint = [sender locationInView:sender.view];
    if (numTouches == 2){
        
        // get the centerpoint position

        if (gPinchCallback) {
            gPinchCallback(centerPoint.x, centerPoint.y, sender.scale, sender.velocity, (int)sender.state);
        }
        
        // Reset scale if ended to prepare for next gesture
        if (sender.state == UIGestureRecognizerStateEnded) {
            sender.scale = 1.0;
        }
    }
    else{
        if (gPinchCallback) {
            gPinchCallback(centerPoint.x, centerPoint.y, sender.scale, sender.velocity, (int)sender.state);
        }
    }
}

@end

static PinchGestureDelegate* gPinchDelegate = nil;

extern "C" void setupPinchGestureRecognizer(UIView* view, PinchGestureCallback callback) {
    // Store the callback function
    gPinchCallback = callback;
    
    // Create delegate if it doesn't exist
    if (gPinchDelegate == nil) {
        gPinchDelegate = [[PinchGestureDelegate alloc] init];
    }
    
    // Create the pinch gesture recognizer
    UIPinchGestureRecognizer* pinchRecognizer = [[UIPinchGestureRecognizer alloc] 
                                                initWithTarget:gPinchDelegate 
                                                action:@selector(handlePinch:)];
    
    pinchRecognizer.delegate = gPinchDelegate;
    [view addGestureRecognizer:pinchRecognizer];
}

extern "C" UIView* getUIViewFromQWidget(QWidget* widget) {
    if (!widget) return nil;
    
    // Get the native view for the widget
    return (__bridge UIView*)widget->winId();
}

extern "C" void registerPinchGestureForWidget(QWidget* widget, PinchGestureCallback callback) {
    UIView* view = getUIViewFromQWidget(widget);
    if (view) {
        setupPinchGestureRecognizer(view, callback);
    }
}

// define document picker delegate:
@interface UIDocumentPickerViewControllerDelegate : NSObject <UIDocumentPickerDelegate>

@property (nonatomic) QString rootPath;

@end

@implementation UIDocumentPickerViewControllerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls{
    if (urls.count > 0){
        NSURL *selectedFileURL = urls[0];
        
        // Start accessing the security-scoped resource
        BOOL startedAccessing = [selectedFileURL startAccessingSecurityScopedResource];
        
        if (startedAccessing) {
            // Convert rootPath from QString to NSString and use it as the destination directory
            NSString *destinationDirectory = [NSString stringWithUTF8String:self.rootPath.toUtf8().constData()];
            destinationDirectory = [destinationDirectory stringByAppendingPathComponent:@"pdf_docs"];

            NSString *fileName = [selectedFileURL lastPathComponent];
            NSString *destinationFilePath = [destinationDirectory stringByAppendingPathComponent:fileName];
            
            NSError *error = nil;
            
            // Ensure destination directory exists
            BOOL isDirectory;
            if (![[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory isDirectory:&isDirectory] || !isDirectory) {
                [[NSFileManager defaultManager] createDirectoryAtPath:destinationDirectory 
                                          withIntermediateDirectories:YES 
                                                           attributes:nil 
                                                                error:&error];
                if (error) {
                    [selectedFileURL stopAccessingSecurityScopedResource];
                    return;
                }
            }
            
            // Remove existing file if present
            if ([[NSFileManager defaultManager] fileExistsAtPath:destinationFilePath]) {
                [[NSFileManager defaultManager] removeItemAtPath:destinationFilePath error:nil];
            }
            
            // Copy using URLs instead of paths
            if ([[NSFileManager defaultManager] copyItemAtURL:selectedFileURL 
                                                       toURL:[NSURL fileURLWithPath:destinationFilePath] 
                                                       error:&error]) {
                on_ios_file_picked(QString::fromNSString(destinationFilePath));
            } else {
                qDebug() << "Error copying file:" << error.localizedDescription;
            }
            
            // Always stop accessing when done
            [selectedFileURL stopAccessingSecurityScopedResource];
        } else {
            qDebug() << "Failed to start accessing security-scoped resource";
        }
    }
}
@end

extern "C" QString promptUserToSelectPdfFile(QString rootPath){
    QString selectedFilePath = "";
    UIDocumentPickerViewController* documentPicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"com.adobe.pdf"] inMode:UIDocumentPickerModeOpen];
    auto delegate = [[UIDocumentPickerViewControllerDelegate alloc] init];
    delegate.rootPath = rootPath;
    documentPicker.delegate = delegate;
    documentPicker.allowsMultipleSelection = NO;
    documentPicker.modalPresentationStyle = UIModalPresentationFormSheet;
    UIViewController* rootViewController = [UIApplication sharedApplication].keyWindow.rootViewController;
    [rootViewController presentViewController:documentPicker animated:YES completion:nil];
    return selectedFilePath;
}


extern "C" void ios_debug(){
}

extern "C" void makeSureTTSCanUseSpeakers(){

    AVAudioSession* audioSession = [AVAudioSession sharedInstance];
    NSError* error = nil;
    [audioSession setCategory:AVAudioSessionCategoryPlayback error:&error];
    if (error){
        NSLog(@"Error setting audio session category: %@", error.localizedDescription);
    }
    [audioSession setActive:YES error:&error];
    if (error){
        NSLog(@"Error activating audio session: %@", error.localizedDescription);
    }

}
