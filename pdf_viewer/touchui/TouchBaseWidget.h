#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchBaseWidget : public QWidget {
    Q_OBJECT
public:
    TouchBaseWidget(QWidget* parent);

    virtual void initialize_widget() = 0;
    void connect_show_signal();
public slots:
    void handleShowKeyboard();
    void resizeEvent(QResizeEvent* resize_event);

protected:
    QQuickWidget* quick_widget = nullptr;
    void initialize_base();

};
