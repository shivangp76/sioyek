#pragma once
#include <QWidget>

class MainWidget;

class ConfigUI : public QWidget {
    //class ConfigUI : public QQuickWidget{
    Q_OBJECT
public:
    ConfigUI(std::string name, MainWidget* parent);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE virtual QRect get_prefered_rect(QRect parent_rect);
    void set_should_persist(bool val);
    void on_change();
    ~ConfigUI();

protected:
    MainWidget* main_widget;
    std::string config_name;
    bool should_persist = true;
};
