#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchSlider : public QWidget {
    Q_OBJECT
public:
    TouchSlider(float from, float to, float initial_value, QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleSelect(double item);
    void handleCancel();

signals:
    void itemSelected(float);
    void canceled();

private:
    QQuickWidget* quick_widget = nullptr;

};
