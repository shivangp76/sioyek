#include <QTimer>

#include "main_widget.h"
#include "document_view.h"
#include "controllers/tts_controller.h"
#include "touchui/AudioUI.h"
#include "touchui/TouchAudioButtons.h"
#include "network_manager.h"
#include "controllers/network_controller.h"

extern float TTS_RATE;
extern bool TOUCH_MODE;
extern std::wstring TTS_VOICE;
extern bool USE_LOCAL_TTS_WHILE_WAITING_FOR_HQ_TTS;

#ifdef SIOYEK_IOS
extern "C" void makeSureTTSCanUseSpeakers();
#endif


TTSController::TTSController(MainWidget* parent) : mw(parent){
}

TTSController::~TTSController(){
    if (is_reading) {
        is_reading = false;
        get_tts()->stop();
    }
    if (tts) {
        delete tts;
    }
}

void TTSController::handle_start_reading(bool force_local){
    int line_index = mw->main_document_view->get_line_index();
    if (line_index == -1) {
        show_error_message(L"You must select a line first (e.g. by right clicking on a line)");
        return;
    }

    is_reading = true;
    read_current_line(force_local);
    show_touch_auido_ui_if_in_touch_mode();
}

void TTSController::read_current_line(bool force_local){
    if (!force_local && high_quality_play_state.has_value()) {
        mw->handle_stop_reading();
        mw->handle_start_reading_high_quality();
    }
    else {

        if (!word_by_word_reading){
            if (is_reading){
                is_reading = false;
                prev_tts_state = "Stopped";
                get_tts()->stop();
            }
        }

        std::wstring selected_line_text = mw->main_document_view->get_selected_line_text().value_or(L"");
        //std::wstring text = main_document_view->get_selected_line_text().value_or(L"");
        tts_text.clear();
        tts_corresponding_line_rects.clear();
        tts_corresponding_char_rects.clear();

        int page_number = mw->main_document_view->get_vertical_line_page();
        AbsoluteRect ruler_rect = mw->main_document_view->get_ruler_rect().value_or(fz_empty_rect);
        int maximum_size = get_tts()->get_maximum_tts_text_size();
        int index_into_page = mw->doc()->get_page_text_and_line_rects_after_rect(
            page_number,
            maximum_size,
            ruler_rect,
            tts_text,
            tts_corresponding_line_rects,
            tts_corresponding_char_rects);


        //fz_stext_page* stext_page = doc()->get_stext_with_page_number(page_number);
        //std::vector<fz_stext_char*> flat_chars;
        //get_flat_chars_from_stext_page(stext_page, flat_chars, true);

        get_tts()->set_rate(TTS_RATE);
        if (word_by_word_reading) {
            int page_offset = mw->doc()->get_page_offset_into_super_fast_index(page_number);
            int current_offset_into_document = page_offset + index_into_page;
            if (tts_text.size() > 0) {
                get_tts()->say(QString::fromStdWString(tts_text), current_offset_into_document);
            }
        }
        else {
            get_tts()->say(QString::fromStdWString(selected_line_text));
        }

        // last_page_read = page_number;
        // last_index_into_page_read = index_into_page;

        is_reading = true;
    }
}

void TTSController::handle_stop_reading(){
    mw->dv()->is_waiting_for_high_quality_tts_result = false;
    bool was_high_quality_playing = high_quality_play_state.has_value() && media_player && media_player->isPlaying();

    if (high_quality_play_state.has_value() && media_player){
        media_player->stop();
    }

    if (was_high_quality_playing) {
        is_reading = false;
        high_quality_play_state = {};
    }
    else {
        is_reading = false;

        get_tts()->stop();

        if (TOUCH_MODE) {
            mw->pop_current_widget();
        }
    }
}

void TTSController::handle_pause(){
    is_reading = false;
    if (mw->is_high_quality_tts_playing()){
        get_media_player()->pause();
    }
    else{
        mw->get_tts()->pause();
    }
}

TextToSpeechHandler* TTSController::get_tts() {
    if (tts) return tts;

#ifdef SIOYEK_IOS
    makeSureTTSCanUseSpeakers();
#endif

#ifdef SIOYEK_ANDROID
    tts = new AndroidTextToSpeechHandler();
#else
    tts = new QtTextToSpeechHandler();
#endif


    if (TTS_VOICE.size() > 0) {
        tts->set_voice(TTS_VOICE);
    }
    //void sayingWord(const QString &word, qsizetype id, qsizetype start, qsizetype length);

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)

    tts_has_pause_resume_capability = tts->is_pausable();
    if (tts->is_word_by_word()) {
        word_by_word_reading = true;
    }

    //void aboutToSynthesize(qsizetype id);
    tts->set_word_callback([&](int start, int length) {
        if (is_reading) {
            if (start >= tts_corresponding_line_rects.size()) return;

            PagelessDocumentRect line_being_read_rect = tts_corresponding_line_rects[start];
            PagelessDocumentRect char_being_read_rect = tts_corresponding_char_rects[start];

            int ruler_page = mw->main_document_view->get_vertical_line_page();

            DocumentRect char_being_read_document_rect = DocumentRect(char_being_read_rect, ruler_page);
            NormalizedWindowRect char_being_read_window_rect = char_being_read_document_rect.to_window_normalized(mw->main_document_view);

            int end = start + length;
            if (last_focused_rect.has_value() && (last_focused_rect.value() == line_being_read_rect)) {

                if (char_being_read_window_rect.x0 > 1) {
                    mw->main_document_view->move_visual_mark_next();
                    mw->invalidate_render();
                }
            }

            if (!tts_is_about_to_finish && ((!last_focused_rect.has_value()) || !(last_focused_rect.value() == line_being_read_rect))) {
                last_focused_rect = line_being_read_rect;
                DocumentRect line_being_read_document_rect = DocumentRect(line_being_read_rect, ruler_page);
                WindowRect line_being_read_window_rect = line_being_read_document_rect.to_window(mw->main_document_view);
                NormalizedWindowRect line_being_read_normalized_window_rect = line_being_read_document_rect.to_window_normalized(mw->main_document_view);
                mw->main_document_view->focus_rect(line_being_read_document_rect);

                if (line_being_read_normalized_window_rect.x0 < -1) { // if the next line is out of view
                    mw->move_horizontal(-line_being_read_window_rect.x0);
                }

                mw->invalidate_render();
            }


            if ((tts_text.size() - end) <= 5) {
                tts_is_about_to_finish = true;
            }

        }
        });


    tts->set_state_change_callback([&](QString state) {

        if ((!word_by_word_reading) && is_reading && (state == "Ready") && (prev_tts_state == "Speaking")){
            // when word_by_word_reading is not available, we can't rely on tts_is_about_to_finish
            mw->move_visual_mark(1);
            mw->invalidate_render();

        }
        else{
            if ((state == "Ready") || (state == "Error")) {
                if (is_reading && tts_is_about_to_finish) {
                    tts_is_about_to_finish = false;
                    mw->move_visual_mark(1);
                    mw->invalidate_render();
                }
            }
        }
        prev_tts_state = state;
        });

    tts->set_external_state_change_callback([&](QString state){
        mw->ensure_player_state_(state);
    });

    tts->set_on_app_pause_callback([&](){

        bool is_audio_ui_visible = false;
        if (mw->current_widget_stack.size() > 0){
            AudioUI* audio_ui = dynamic_cast<AudioUI*>(mw->current_widget_stack.back());
            if (audio_ui){
                is_audio_ui_visible = true;
            }
        }

        if (is_reading || is_audio_ui_visible) {
            return mw->get_rest_of_document_pages_text();
        }
        else{
            return QString("");
        }
    });

    tts->set_on_app_resume_callback([&](bool is_playing, bool is_on_rest, int offset){

        handle_app_tts_resume(is_playing, is_on_rest, offset);
    });

#else
    QObject::connect(tts, &QTextToSpeech::stateChanged, [&](QTextToSpeech::State state) {
        if ((state == QTextToSpeech::Ready) || (state == QTextToSpeech::Error)) {
            if (is_reading) {
                move_visual_mark(1);
                //read_current_line();
                invalidate_render();
            }
        }
        });
#endif

#ifdef SIOYEK_ANDROID
        // wait for the tts engine to be initialized
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif

    return tts;
}

void TTSController::show_touch_auido_ui_if_in_touch_mode(){
    if (TOUCH_MODE) {
        AudioUI * audio_ui_widget = new AudioUI(mw);
        mw->set_current_widget(audio_ui_widget);
        mw->show_current_widget();
    }
}

void TTSController::ensure_player_state(QString state){
    if (mw->current_widget_stack.size() > 0) {
        AudioUI* audio_ui = dynamic_cast<AudioUI*>(mw->current_widget_stack.back());
        if (audio_ui) {
            if (state == "Ready") {
                audio_ui->buttons->set_playing();
                is_reading = true;
            }
            if (state == "Ended") {
                audio_ui->buttons->set_paused();
                is_reading = false;
            }
        }
    }
}

void TTSController::handle_start_reading_high_quality(bool should_preload) {

    std::vector<PagelessDocumentRect> line_rects;
    std::vector<PagelessDocumentRect> char_rects;
    int current_page_number = mw->get_current_page_number();

    int line_number = mw->main_document_view->get_line_index();
    HighQualityPlayState play_state;
    play_state.doc = mw->doc();
    play_state.page_number = current_page_number;
    play_state.start_line = line_number;
    high_quality_play_state = play_state;
    float rate = TTS_RATE;

    std::wstring text;
    int index_into_page = mw->dv()->get_page_text_and_line_rects(current_page_number, text, line_rects);
    high_quality_play_state->line_rects = line_rects;

    if (mw->network_controller->sioyek_network_manager->does_pending_tts_command_exist(QString::fromStdString(mw->doc()->get_checksum()), QString::fromStdWString(text))){
        return;
    }

    QString status_message_id = mw->set_status_message(L"Performing Text to Speech");
    mw->dv()->is_waiting_for_high_quality_tts_result = true;
    bool tts_result_already_exists = mw->network_controller->sioyek_network_manager->tts(
                mw,
                text,
                mw->doc()->get_checksum(),
                mw->get_current_page_number(),
                rate,
                [&, index_into_page, status_message_id, rate, current_page_number](QString file_path, std::vector<float> timestamps) {
        mw->dv()->is_waiting_for_high_quality_tts_result = false;
        mw->set_status_message(L"", status_message_id);
        SioyekMediaPlayer* mp = get_media_player();
        std::wstring notification_file_name = mw->doc()->detect_paper_name();
#ifdef Q_OS_APPLE
        mp->setSource(QUrl::fromLocalFile(file_path), QString::fromStdWString(notification_file_name), current_page_number);
#else
        mp->setSource(QUrl::fromLocalFile(file_path));
#endif
        high_quality_play_state->timestamps = timestamps;
        //media_player->audioTracks().at(0).

        auto seek_to_location = [&, mp, index_into_page, timestamps, current_page_number, rate](bool seekable) mutable {
            if (seekable) {
                QTimer::singleShot(0, [&, mp, timestamps, index_into_page, current_page_number, rate]() mutable {

                    if (tts && tts->is_playing()){
                        int new_page_number = mw->get_current_page_number();

                        if (new_page_number != current_page_number){
                            // the low quality tts has read the entire page before the high quality tts result has arrived
                            // we don't need to do anything now
                            return;
                        }
                        else{
                            // we need to continue with high quality result from the last location

                            std::wstring dummy_text;
                            std::vector<PagelessDocumentRect> dummy_rects;
                            index_into_page = mw->dv()->get_page_text_and_line_rects(new_page_number, dummy_text, dummy_rects);
                            tts->stop();
                        }


                    }
                    if (index_into_page < timestamps.size()) {
                        float time = timestamps[index_into_page];
                        mp->setPosition(static_cast<int>(time * 1000));
                        mp->setPlaybackRate(rate);
                        mp->play();

                        if (high_quality_play_state) {
                            high_quality_play_state->is_playing = true;
                        }
                    }
                    });
            }
            };
        if (mp->isSeekable()) {
            seek_to_location(true);
        }
        else {
#ifndef SIOYEK_ADVANCED_AUDIO
            // QObject::connect(mp, &QMediaPlayer::seekableChanged, seek_to_location);
#endif
        }

        }, [&, status_message_id](QString failed_string_checksum) {
            mw->set_status_message(L"Text to Speech Failed", status_message_id);
            });

    if (!tts_result_already_exists && USE_LOCAL_TTS_WHILE_WAITING_FOR_HQ_TTS){
        handle_start_reading(true);
    }

    if (should_preload) {
        mw->network_controller->preload_next_page_for_tts(rate);

        //sioyek_network_manager->tts(this, )
    }
    show_touch_auido_ui_if_in_touch_mode();
}

void TTSController::focus_on_high_quality_text_being_read() {


    if ((media_player != nullptr) && (media_player->isPlaying()) && high_quality_play_state.has_value()) {

#ifdef SIOYEK_ADVANCED_AUDIO
        if (media_player->isFinished()) {
            mw->handle_high_quality_media_end_reached();
        }
#endif

        float current_time = static_cast<float>(media_player->position()) / 1000.0f;
        int index = -1;
        for (int i = 0; i < high_quality_play_state->timestamps.size(); i++) {
            if (high_quality_play_state->timestamps[i] > current_time) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            PagelessDocumentRect rect_to_focus = high_quality_play_state->line_rects[index];
            if (!(rect_to_focus == high_quality_play_state->last_focused_rect)) {
                mw->main_document_view->focus_rect(DocumentRect(rect_to_focus, high_quality_play_state->page_number));
                high_quality_play_state->last_focused_rect = rect_to_focus;
                mw->invalidate_render();
            }
        }

    }
}

void TTSController::handle_app_tts_resume(bool is_playing, bool is_on_rest, int offset){
    // set_status_message(L"got on resume callback");

    // return;
    if (is_reading && (is_playing == false)){
        is_reading = false;
    }

    if (is_playing){
        ensure_player_state("Ended");

        handle_stop_reading();

        if (mw->doc() && mw->doc()->is_super_fast_index_ready()){
            if (offset > 0){
                mw->focus_on_character_offset_into_document(offset);
            }
        }
        else{
            mw->dv()->on_super_fast_compute_focus_offset = offset;
        }
    }
    else{
        ensure_player_state("Ended");
    }
}

SioyekMediaPlayer* TTSController::get_media_player(){
    if (media_player == nullptr) {

#ifdef Q_OS_APPLE
#ifdef SIOYEK_IOS
    makeSureTTSCanUseSpeakers();
#endif
        media_player = new MacosMediaPlayer();
#else
#ifndef SIOYEK_ADVANCED_AUDIO
        QAudioOutput* audio_output = new QAudioOutput(this);
        audio_output->setVolume(10);
        media_player = new QMediaPlayer(this);
        media_player->setAudioOutput(audio_output);
        QObject::connect(media_player, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::MediaStatus::EndOfMedia) {
                //qDebug() << "end of media reached";
                handle_high_quality_media_end_reached();
            }
            });
#else
        media_player = new MyPlayer();
#endif
#endif
    }
    return media_player;
}

bool TTSController::is_high_quality_tts_playing() {
    if (media_player) {
        return media_player->isPlaying();
    }
    return false;
}

#ifdef Q_OS_APPLE
void TTSController::apple_on_high_quality_tts_playback_finished(){

    if (media_player){
        MacosMediaPlayer* macos_specific_media_player = dynamic_cast<MacosMediaPlayer*>(media_player);
        if (!media_player->isPlaying()){
            // on the macos-specific media player, when we reach the end of the media,
            // the position resets to 0 and isPlaying will be false, so the if statement below
            // this one does not trigger on macos, so we handle it here instead

            mw->handle_high_quality_media_end_reached();
        }
    }
}
#endif
