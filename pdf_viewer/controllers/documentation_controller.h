#pragma once
#include <QJsonDocument>

#include "controllers/base_controller.h"

extern float MENU_SCREEN_WDITH_RATIO;
extern float MENU_SCREEN_HEIGHT_RATIO;

class DocumentationController : public BaseController {
protected:
    QJsonDocument sioyek_documentation_json_document;
public:
    DocumentationController(MainWidget* parent);
    void load_sioyek_documentation();
    QString get_config_documentation_with_title(QString config, QString command_name);
    QString get_command_documentation_with_title(QString command_name);
    QString get_command_documentation(QString command_name, bool is_config, QString* out_url, QString* out_file_name);
    QString get_related_command_and_configs_string(QJsonArray related_commands, QJsonArray related_configs);

    void print_undocumented_configs();
    void print_undocumented_commands();
    void print_documented_but_removed_commands();
    void show_documentation_with_title(QString doctype, QString title);
    void open_documentation_file_for_name(QString doctype, QString name);
    void show_markdown_text_widget(QString url, QString text);
    void show_command_documentation(QString command_name, bool is_config);
    void handle_documentation_search();
};
