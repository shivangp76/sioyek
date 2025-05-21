#include "touchui/AudioUI.h"
#include "touchui/TouchAudioButtons.h"
#include "main_widget.h"
#include <QVBoxLayout>

extern float TTS_RATE;

AudioUI::AudioUI(MainWidget* parent) : ConfigUI("", parent) {

    buttons = new TouchAudioButtons(this);
    buttons->set_rate(TTS_RATE);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(buttons);
    setLayout(layout);

    QObject::connect(buttons, &TouchAudioButtons::playPressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_play();
        });

    QObject::connect(buttons, &TouchAudioButtons::stopPressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_stop_reading();
        });

    QObject::connect(buttons, &TouchAudioButtons::pausePressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_pause();
        });


    QObject::connect(buttons, &TouchAudioButtons::rateChanged, [&](qreal rate) {
        TTS_RATE = (rate + 2);
        buttons->set_rate(TTS_RATE);
        main_widget->handle_pause();
        main_widget->handle_play();
        });
}

QRect AudioUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int w = parent_width;
    int h = parent_height / 6;
    return QRect((parent_width - w) / 2, parent_height - h, w, h);
}
