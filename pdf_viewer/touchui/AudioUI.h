#pragma once
#include "touchui/ConfigUI.h"

class TouchAudioButtons;

class AudioUI : public ConfigUI {
    Q_OBJECT
public:
    AudioUI(MainWidget* parent);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    // void resizeEvent(QResizeEvent* resize_event) override;
    TouchAudioButtons* buttons = nullptr;
};
