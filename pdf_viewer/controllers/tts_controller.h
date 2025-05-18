#pragma once
#include <QString>

class MainWidget;
class TextToSpeechHandler;

class TTSController{
private:
    MainWidget* mw;
public:
    TTSController(MainWidget* parent);
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
};
