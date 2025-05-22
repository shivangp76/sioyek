#pragma once

#ifdef SIOYEK_ANDROID
#include "utils.h"

QString android_file_name_from_uri(QString uri);
void android_tts_say(QString text);
void check_pending_intents(const QString workingDirPath);
void android_tts_pause();
void android_tts_stop();
void android_tts_set_rate(float rate);
// void android_tts_set_rest_of_document(QString rest);
void on_android_pause_global();
void on_android_resume_global();
void android_brightness_set(float brightness);
float android_brightness_get();

QString android_file_uri_from_content_uri(QString uri);

class AndroidTextToSpeechHandler : public TextToSpeechHandler {
public:
    // std::optional<std::function<void(int, int)>> word_callback = {};
    // std::optional<std::function<void(QString)>> state_change_callback = {};

    AndroidTextToSpeechHandler();

    void say(QString text, int start_offset=-1) override;

    void stop() override;

    void pause() override;

    bool is_playing() override;
    void set_rate(float rate);

    bool is_pausable();

    bool is_word_by_word();

    void set_word_callback(std::function<void(int, int)> callback);

    void set_state_change_callback(std::function<void(QString)> callback);
    void set_external_state_change_callback(std::function<void(QString)> callback);
    virtual void set_on_app_pause_callback(std::function<QString()>);
    virtual void set_on_app_resume_callback(std::function<void(bool, bool, int)>);
    int get_maximum_tts_text_size();
};
#endif
