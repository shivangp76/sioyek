#include "utils/android_specific_utils.h"

#ifdef SIOYEK_ANDROID

#ifdef SIOYEK_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#include <qjniobject.h>
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
#include <signal.h>
#endif

#include "main_widget.h"

int last_keypad_size = 0;

std::optional<std::function<void(int, int)>> android_global_word_callback = {};
std::optional<std::function<void(QString)>> android_global_state_change_callback = {};
std::optional<std::function<void(QString)>> android_global_external_state_change_callback = {};
std::optional<std::function<QString()>> android_global_on_android_app_pause_callback = {};
std::optional<std::function<void(bool, bool, int)>> android_global_resume_state_callback = {};

QJniObject parseUriString(const QString& uriString) {
    return QJniObject::callStaticObjectMethod
    ("android/net/Uri", "parse",
        "(Ljava/lang/String;)Landroid/net/Uri;",
        QJniObject::fromString(uriString).object());
}

QString android_file_uri_from_content_uri(QString uri) {

    //    mainActivityObj = QtAndroid::androidActivity();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject uri_object = parseUriString(uri);

    QJniObject file_uri_object = QJniObject::callStaticObjectMethod("info/sioyek/sioyek/SioyekActivity",
        "getPathFromUri",
        "(Landroid/content/Context;Landroid/net/Uri;)Ljava/lang/String;", activity.object(), uri_object.object());
    return file_uri_object.toString();
}

int android_tts_get_max_text_size(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    return activity.callMethod<int>("ttsGetMaxTextSize", "()I");
}

bool android_tts_is_playing(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    // returns a boolean
    return activity.callMethod<bool>("ttsIsPlaying");
}

void android_tts_say(QString text, int start_offset) {

    QJniObject text_jni = QJniObject::fromString(text);
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    //QJniObject contentResolverObj = activity.callObjectMethod("saySomethingElse", "(Ljava/lang/String;)V", text_jni.object<jstring>());
    // activity.callMethod<void>("ttsSay", "(Ljava/lang/String;)V", text_jni.object<jstring>());
    activity.callMethod<void>("ttsSay", "(Ljava/lang/String;I)V", text_jni.object<jstring>(), start_offset);
}

void android_tts_pause(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("ttsPause");
}

void android_tts_stop(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("ttsStop");
}

void android_tts_set_rate(float rate){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("ttsSetRate", "(F)V", rate);
}

void android_brightness_set(float brightness){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("setScreenBrightness", "(F)V", brightness);
}

float android_brightness_get(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    return activity.callMethod<float>("getScreenBrightness", "()F");
}

// void android_tts_stop_service(){
//     QJniObject activity = QNativeInterface::QAndroidApplication::context();
//     activity.callMethod<void>("stopTtsService", "()V");
// }


extern std::vector<MainWidget*> windows;

// modified from https://github.com/mahdize/CrossQFile/blob/main/CrossQFile.cpp


void log_d(QString text){
    // call the activitie's myLogD method with text
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("myLogD", "(Ljava/lang/String;)V", QJniObject::fromString(text).object<jstring>());

}

QString android_file_name_from_uri(QString uri) {

    //    mainActivityObj = QtAndroid::androidActivity();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();

    QJniObject contentResolverObj = activity.callObjectMethod
    ("getContentResolver", "()Landroid/content/ContentResolver;");


    //	QAndroidJniObject cursorObj {contentResolverObj.callObjectMethod
    //		("query",
    //		 "(Landroid/net/Uri;[Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/database/Cursor;",
    //		 parseUriString(fileName()).object(), QAndroidJniObject().object(), QAndroidJniObject().object(),
    //		 QAndroidJniObject().object(), QAndroidJniObject().object())};

    QJniObject cursorObj{ contentResolverObj.callObjectMethod
        ("query",
         "(Landroid/net/Uri;[Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/database/Cursor;",
         parseUriString(uri).object(), QJniObject().object(), QJniObject().object(),
         QJniObject().object(), QJniObject().object()) };

    cursorObj.callMethod<jboolean>("moveToFirst");

    QJniObject retObj{ cursorObj.callObjectMethod
        ("getString","(I)Ljava/lang/String;", cursorObj.callMethod<jint>
         ("getColumnIndex","(Ljava/lang/String;)I",
          QJniObject::getStaticObjectField<jstring>
          ("android/provider/OpenableColumns","DISPLAY_NAME").object())) };

    QString ret{ retObj.toString() };
    return ret;
}

void check_pending_intents(const QString workingDirPath)
{
    //    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        // create a Java String for the Working Dir Path
        QJniObject jniWorkingDir = QJniObject::fromString(workingDirPath);
        if (!jniWorkingDir.isValid()) {
            //            emit shareError(0, tr("Share: an Error occured\nWorkingDir not valid"));
            return;
        }
        activity.callMethod<void>("checkPendingIntents", "(Ljava/lang/String;)V", jniWorkingDir.object<jstring>());
        return;
    }
}


void setFileUrlReceived(const QString& url)
{
    if (windows.size() > 0) {
        windows[0]->open_document(url.toStdWString());
    }
}

void on_android_tts(int begin, int end){
    if (android_global_word_callback){
        android_global_word_callback.value()(begin, end - begin);
    }
    // qDebug() << "ttsed from " << begin << " to " << end;
}

void on_android_pause_global(){
    for (auto window : windows){
        QMetaObject::invokeMethod(window, "on_mobile_pause", Qt::QueuedConnection);
    }
}

void on_android_resume_global(){
    for (auto window : windows){
        QMetaObject::invokeMethod(window, "on_mobile_resume", Qt::QueuedConnection);
    }
}

void on_android_state_change(QString new_state){
    if (android_global_state_change_callback){
        android_global_state_change_callback.value()(new_state);
    }
}

void on_android_external_state_change(QString new_state){
    if (android_global_external_state_change_callback){
        android_global_external_state_change_callback.value()(new_state);
    }
}

bool on_android_resume_state(bool is_playing, bool is_on_rest, int offset){
    if (windows.size() > 0){
        for (auto window : windows){
            QMetaObject::invokeMethod(window, "handle_app_tts_resume", Qt::QueuedConnection,
                                      Q_ARG(bool, is_playing), Q_ARG(bool, is_on_rest), Q_ARG(int, offset));
        }
        return true;
    }
    return false;

}

QString on_android_get_rest_on_pause(){
    if (android_global_on_android_app_pause_callback){
        QString res = android_global_on_android_app_pause_callback.value()();
        return res;
    }
    return "";
}

extern "C" {
    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_setFileUrlReceived(JNIEnv* env,
            jobject obj,
            jstring url)
    {
        const char* urlStr = env->GetStringUTFChars(url, NULL);
        Q_UNUSED(obj)
            setFileUrlReceived(urlStr);
        env->ReleaseStringUTFChars(url, urlStr);
        return;
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_qDebug(JNIEnv* env,
            jobject obj,
            jstring url)
    {
        const char* urlStr = env->GetStringUTFChars(url, NULL);
        qDebug() << urlStr;
        Q_UNUSED(obj)
            env->ReleaseStringUTFChars(url, urlStr);
        return;
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onTts(JNIEnv* env,
            jobject obj,
            jint begin, jint end)
    {
        on_android_tts((int)begin, (int)end);
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onAndroidPause(JNIEnv* env,
                                                          jobject obj)
    {
        on_android_pause_global();
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onAndroidResume(JNIEnv* env,
                                                          jobject obj)
    {
        on_android_resume_global();
    }

    // public static native void onAndroidKeypadHide();
    // public static native void onAndroidKeypadShow(int size);

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onAndroidKeypadHide(JNIEnv* env,
                                                          jobject obj)
    {
        // if (is_onscreen_keyboard_visible){
        //     QMetaObject::invokeMethod(this, "resize_child_widgets_with_window_rect", Qt::QueuedConnection, Q_ARG(QRect, half_rect));
        // }
        // else{
        //     QMetaObject::invokeMethod(this, "resize_child_widgets_with_window_rect", Qt::QueuedConnection, Q_ARG(QRect, full_rect));
        // }

        last_keypad_size = 0;

        if (windows.size() > 0){
            QMetaObject::invokeMethod(windows[0], "on_onscreen_keyboard_hidden", Qt::QueuedConnection);
        }
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onAndroidKeypadShow(JNIEnv* env,
            jobject obj,
            jint height)
    {
        last_keypad_size = height;
        if (windows.size() > 0){
            QMetaObject::invokeMethod(windows[0], "on_onscreen_keyboard_shown", Qt::QueuedConnection);
        }
    }

    // JNIEXPORT void JNICALL
    //     Java_info_sioyek_sioyek_SioyekActivity_onAndroidPause(JNIEnv* env,
    //                                                       jobject obj)
    // {
    //     on_android_pause
    // }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onTtsStateChange(JNIEnv* env,
            jobject obj,
            jstring new_state)
    {

        const char* state_str = env->GetStringUTFChars(new_state, NULL);
        Q_UNUSED(obj)
            env->ReleaseStringUTFChars(new_state, state_str);
        on_android_state_change(state_str);
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onExternalTtsStateChange(JNIEnv* env,
            jobject obj,
            jstring new_state)
    {

        const char* state_str = env->GetStringUTFChars(new_state, NULL);
        Q_UNUSED(obj)
            env->ReleaseStringUTFChars(new_state, state_str);
        on_android_external_state_change(state_str);
    }

    JNIEXPORT bool JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onResumeState(JNIEnv* env,
            jobject obj,
            jboolean is_playing,
            jboolean reading_rest,
            jint offset)
    {

        Q_UNUSED(obj)
        return on_android_resume_state(is_playing, reading_rest, offset);
    }

    JNIEXPORT jstring JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_getRestOnPause(JNIEnv* env,
              jobject obj)
    {

        Q_UNUSED(obj)
        QString res = on_android_get_rest_on_pause();
        std::string res_std = res.toStdString();
        return env->NewStringUTF(res_std.c_str());
    }

}

AndroidTextToSpeechHandler::AndroidTextToSpeechHandler() {
}

int AndroidTextToSpeechHandler::get_maximum_tts_text_size(){
    return android_tts_get_max_text_size();
}

void AndroidTextToSpeechHandler::say(QString text, int start_offset) {
    android_tts_say(text, start_offset);
}

void AndroidTextToSpeechHandler::stop() {
    android_tts_stop();
}

void AndroidTextToSpeechHandler::pause() {
    android_tts_pause();
}

void AndroidTextToSpeechHandler::set_rate(float rate) {
    android_tts_set_rate(std::pow(4.0f, rate));
}

bool AndroidTextToSpeechHandler::is_pausable() {
    return true;
}

bool AndroidTextToSpeechHandler::is_word_by_word() {
    return true;
}

bool AndroidTextToSpeechHandler::is_playing() {
    return android_tts_is_playing();
}

void AndroidTextToSpeechHandler::set_word_callback(std::function<void(int, int)> callback) {
    android_global_word_callback = callback;
}

void AndroidTextToSpeechHandler::set_state_change_callback(std::function<void(QString)> callback) {
    android_global_state_change_callback = callback;
}

void AndroidTextToSpeechHandler::set_external_state_change_callback(std::function<void(QString)> callback) {
    android_global_external_state_change_callback = callback;
}

void AndroidTextToSpeechHandler::set_on_app_pause_callback(std::function<QString()> callback){
    android_global_on_android_app_pause_callback = callback;
}

void AndroidTextToSpeechHandler::set_on_app_resume_callback(std::function<void(bool, bool, int)> callback){
    android_global_resume_state_callback = callback;
}
#endif
