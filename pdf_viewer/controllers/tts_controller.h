#pragma once
#include <QString>
#include "book.h"
#include "utils.h"

class MainWidget;
class TextToSpeechHandler;


#ifdef Q_OS_APPLE
using SioyekMediaPlayer = MacosMediaPlayer;
#else
#ifdef SIOYEK_ADVANCED_AUDIO
using SioyekMediaPlayer = MyPlayer;
#else
using SioyekMediaPlayer = QMediaPlayer;
#endif
#endif

struct HighQualityPlayState {
    bool is_playing = false;
    int page_number = -1;
    int start_line = -1;
    std::vector<PagelessDocumentRect> line_rects;
    std::vector<float> timestamps;
    Document* doc = nullptr;
    std::optional<PagelessDocumentRect> last_focused_rect = {};
};

class TTSController{
private:
    MainWidget* mw;

    // is the TTS engine currently reading text?
    bool word_by_word_reading = false;
    QString prev_tts_state = "";
    bool tts_has_pause_resume_capability = false;
    bool tts_is_about_to_finish = false;
    std::wstring tts_text = L"";
    std::vector<PagelessDocumentRect> tts_corresponding_line_rects;
    std::vector<PagelessDocumentRect> tts_corresponding_char_rects;
    std::optional<PagelessDocumentRect> last_focused_rect = {};
public:
    bool is_reading = false;
    std::optional<HighQualityPlayState> high_quality_play_state = {};
    TextToSpeechHandler* tts = nullptr;
    SioyekMediaPlayer* media_player = nullptr;

    TTSController(MainWidget* parent);
    ~TTSController();

    void handle_start_reading(bool force_local);
    void read_current_line(bool force_local=false);
    void handle_stop_reading();
    void handle_pause();
    TextToSpeechHandler* get_tts();
    void show_touch_auido_ui_if_in_touch_mode();
    void ensure_player_state(QString state);
    void handle_start_reading_high_quality(bool should_preload=false);
    void focus_on_high_quality_text_being_read();
    void handle_app_tts_resume(bool is_playing, bool is_on_rest, int offset);
    SioyekMediaPlayer* get_media_player();
    bool is_high_quality_tts_playing();
#ifdef Q_OS_APPLE
    void apple_on_high_quality_tts_playback_finished();
#endif
};
