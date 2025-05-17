#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchShowKeyboardWidget : public QWidget {
    Q_OBJECT
public:
    TouchShowKeyboardWidget(QWidget* parent);

    void connect_show_signal();
public slots:
    void handleShowKeyboard();

protected:
    QQuickWidget* quick_widget = nullptr;

};
