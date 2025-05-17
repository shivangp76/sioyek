#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>
#include "TouchBaseWidget.h"


class TouchTextEdit : public TouchBaseWidget {
    Q_OBJECT
public:
    TouchTextEdit(QString name, QString initial_value, bool is_password, QWidget* parent);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    void keyPressEvent(QKeyEvent* kevent) override;
    void initialize_widget() override;
    void set_text(const std::wstring& txt);

public slots:
    void handleConfirm(QString text);
    void handleCancel();

signals:
    void confirmed(QString text);
    void cancelled();
private:
    QString name;
    QString initial_value;
    bool is_password;

};
