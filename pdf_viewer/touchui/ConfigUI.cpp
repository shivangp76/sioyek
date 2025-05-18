#include "touchui/ConfigUI.h"
#include "main_widget.h"

ConfigUI::ConfigUI(std::string name, MainWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_NoMousePropagation);
    main_widget = parent;
    config_name = name;
}

ConfigUI::~ConfigUI() {
    main_widget->on_config_changed(config_name, false);
}

void ConfigUI::on_change() {
    main_widget->on_config_changed(config_name);
}

void ConfigUI::set_should_persist(bool val) {
    this->should_persist = val;
}

QRect ConfigUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    return QRect(parent_width / 6, parent_height / 4, 2 * parent_width / 3, parent_height / 2);
}
