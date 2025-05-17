#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include "TouchShowKeyboardWidget.h"


class TouchTextEdit : public TouchShowKeyboardWidget {
    Q_OBJECT
public:
    TouchTextEdit(QString name, QString initial_value, bool is_password, QWidget* parent);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    void keyPressEvent(QKeyEvent* kevent) override;
    void set_text(const std::wstring& txt);

public slots:
    void handleConfirm(QString text);
    void handleCancel();

signals:
    void confirmed(QString text);
    void cancelled();

};
