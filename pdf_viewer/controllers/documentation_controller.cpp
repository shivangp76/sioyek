#include <QFile>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>

#include "controllers/documentation_controller.h"
#include "config.h"
#include "commands/base_commands.h"
#include "ui/documentation_ui.h"
#include "ui/ui_models.h"
#include "main_widget.h"
#include "document_view.h"
#include "document.h"
#include "utils/window_utils.h"
#include "database.h"

extern bool SHOW_DOCUMENTATION_IN_WIDGET;
extern bool ALIGN_LINK_DEST_TO_TOP;
extern int DOCUMENTATION_FONT_SIZE;

Path documentation_path(L":/data/sioyek_documentation.pdf");

DocumentationController::DocumentationController(MainWidget* parent) : BaseController(parent) {
}


void DocumentationController::load_sioyek_documentation(){
    if (sioyek_documentation_json_document.isNull()){
        QFile documentation_json_file(":/data/sioyek_documentation.json");
        if (documentation_json_file.open(QFile::ReadOnly)){
            sioyek_documentation_json_document = QJsonDocument().fromJson(documentation_json_file.readAll());
            documentation_json_file.close();
        }
    }
}

QString DocumentationController::get_config_documentation_with_title(QString config, QString title) {
    load_sioyek_documentation();

    const QJsonObject& config_title_to_documentation_map = sioyek_documentation_json_document["config_title_to_documentation_map"].toObject();
    QJsonArray related_commands = sioyek_documentation_json_document["config_related_commands_map"][title].toArray();
    QJsonArray related_configs = sioyek_documentation_json_document["config_related_configs_map"][title].toArray();
    QString relateds = get_related_command_and_configs_string(related_commands, related_configs);

    if (config_title_to_documentation_map.value(title).isString()) {
        QString doc_string = "##" + config_title_to_documentation_map.value(title).toString().trimmed();

        if (config.size() > 0) {
            QString current_value_string = QString::fromStdWString(
                config.toStdWString() + L" " + mw->config_manager->get_config_value_string(config.toStdWString())
                );
            QString res = doc_string + "\n\n" + "<hr/>\n\n#### current value:\n\n`" + current_value_string + "`\n\n";
            res += "[open config file in text editor](changeconfig-" + config + ")\n\n";
            res += "[temporarily change in sioyek](setconfig-" + config + ")\n\n";
            res += "[permanently change in sioyek](setsaveconfig-" + config + ")\n\n";

            return res + relateds;
        }
        else {
            return doc_string + relateds;
        }
    }

    return "";
}

QString DocumentationController::get_related_command_and_configs_string(QJsonArray related_commands, QJsonArray related_configs) {
    QString result = "";

    const QJsonObject& command_name_to_title_map = sioyek_documentation_json_document["command_name_to_title_map"].toObject();
    const QJsonObject& config_name_to_title_map = sioyek_documentation_json_document["config_name_to_title_map"].toObject();

    if (related_commands.size() > 0) {
        result += "\n\nrelated commands: ";
        for (int i = 0; i < related_commands.size(); i++) {
            result += "[" + related_commands[i].toString() + "](commands.md#" + command_name_to_title_map[related_commands[i].toString()].toString() + ")";
            if (i < related_commands.size() - 1) {
                result += ", ";
            }
        }
    }

    if (related_configs.size() > 0) {
        result += "\n\nrelated configs: ";
        for (int i = 0; i < related_configs.size(); i++) {
            result += "[" + related_configs[i].toString() + "](configs.md#" + config_name_to_title_map[related_configs[i].toString()].toString() + ")";
            if (i < related_configs.size() - 1) {
                result += ", ";
            }
        }
    }
    return result;
}

QString DocumentationController::get_command_documentation_with_title(QString title) {
    load_sioyek_documentation();

    const QJsonObject& command_title_to_documentation_map = sioyek_documentation_json_document["command_title_to_documentation_map"].toObject();

    QJsonArray related_commands = sioyek_documentation_json_document["command_related_commands_map"][title].toArray();
    QJsonArray related_configs = sioyek_documentation_json_document["command_related_configs_map"][title].toArray();

    if (command_title_to_documentation_map.value(title).isString()) {
        QString relateds = get_related_command_and_configs_string(related_commands, related_configs);
        return command_title_to_documentation_map.value(title).toString() + relateds;
    }

    return "";
}

QString DocumentationController::get_command_documentation(QString command_name, bool is_config, QString* out_url, QString* out_file_name){
    load_sioyek_documentation();

    const QJsonObject& command_name_to_title_map = sioyek_documentation_json_document["command_name_to_title_map"].toObject();
    const QJsonObject& command_title_to_documentation_map = sioyek_documentation_json_document["command_title_to_documentation_map"].toObject();
    const QJsonObject& config_name_to_title_map = sioyek_documentation_json_document["config_name_to_title_map"].toObject();

    if (is_config) {
        QString config_name = command_name;
        const QJsonObject& config_title_to_documentation_map = sioyek_documentation_json_document["config_title_to_documentation_map"].toObject();
        const QJsonObject& config_name_to_file_name_map = sioyek_documentation_json_document["config_name_to_file_name_map"].toObject();
        const QJsonObject& config_related_commands_map = sioyek_documentation_json_document["config_related_commands_map"].toObject();
        const QJsonObject& config_related_configs_map = sioyek_documentation_json_document["config_related_configs_map"].toObject();

        if (config_name_to_title_map.value(config_name).isString()) {
            QString documentation_title = config_name_to_title_map.value(config_name).toString();
            if (out_url) *out_url = "configs.md#" + documentation_title;
            if (out_file_name) *out_file_name = config_name_to_file_name_map.value(config_name).toString();
            return get_config_documentation_with_title(config_name, documentation_title);
        }
        else if (!config_name_to_file_name_map.value(config_name).isNull()) {
            if (out_url) *out_url = "configs.md#" + config_name_to_file_name_map.value(config_name).toString();
            if (out_file_name) *out_file_name = config_name_to_file_name_map.value(config_name).toString();
        }
    }

    if (command_name_to_title_map.value(command_name).isString()){
        QString documentation_title = command_name_to_title_map.value(command_name).toString();
        const QJsonObject& command_name_to_file_name_map = sioyek_documentation_json_document["command_name_to_file_name_map"].toObject();
        const QJsonObject& command_related_commands_map = sioyek_documentation_json_document["command_related_commands_map"].toObject();
        const QJsonObject& command_related_configs_map = sioyek_documentation_json_document["command_related_configs_map"].toObject();

        if (command_title_to_documentation_map.value(documentation_title).isString()) {
            if (out_url) *out_url = "commands.md#" + documentation_title;
            if (out_file_name) *out_file_name = command_name_to_file_name_map.value(command_name).toString();
            //return command_title_to_documentation_map.value(documentation_title).toString();
            return get_command_documentation_with_title(documentation_title);
        }
    }

    return "";
}

void DocumentationController::print_undocumented_configs(){
    load_sioyek_documentation();
    std::vector<Config*> all_configs = mw->config_manager->get_configs();
    std::vector<QRegularExpression> regex_config_titles; // some documentation config titles are regexes e.g.: "highlight_type_[a-z]"


    for (auto key : sioyek_documentation_json_document["config_name_to_title_map"].toObject().keys()) {
        if ((key.indexOf("[") >= 0)){
            regex_config_titles.push_back(QRegularExpression(key));
        }
        else if (key.indexOf("*") >= 0) {
            key = key.replace("*", "[a-z_]*");
            regex_config_titles.push_back(QRegularExpression(key));
        }
    }

    for (auto& config : all_configs) {
        QString config_name = QString::fromStdWString(config->name);
        if (sioyek_documentation_json_document["config_name_to_title_map"][config_name].toString().size() == 0) {
            // don't print configs like highlight_type_a to highlight_type_z
            if (config_name.size() > 2 && config_name[config_name.size() - 2] == '_') {
                continue;
            }
            if ((config_name.indexOf("visual_mark") >= 0) || (config_name.indexOf("vertical_line") >= 0)) {
                // old names replaced with ruler_ commands
                continue;
            }

            bool found_regex = false;
            for (auto regex : regex_config_titles) {
                if (regex.match(config_name).hasMatch()) {
                    found_regex = true;
                }
            }
            if (found_regex) {
                continue;
            }
            qDebug() << config_name;

        }
    }

}

void DocumentationController::print_undocumented_commands(){
    load_sioyek_documentation();

    auto all_commands = mw->command_manager->get_all_command_names();
    for (auto& command : all_commands) {
        if (sioyek_documentation_json_document["command_name_to_title_map"][command].toString().size() == 0) {
            if (command.indexOf("config_") == -1) {
                qDebug() << command;
            }
        }
    }
}

void DocumentationController::print_documented_but_removed_commands() {

    load_sioyek_documentation();

    auto all_commands = mw->command_manager->get_all_command_names();
    QStringList documented_command_names;

    for (auto key : sioyek_documentation_json_document["command_name_to_title_map"].toObject().keys()) {
        auto value = sioyek_documentation_json_document["command_name_to_title_map"].toObject()[key].toString();
        auto commmands_names = value.split("-");

        for (auto commmand_name : commmands_names) {
            documented_command_names.push_back(commmand_name);
        }
    }

    for (auto& documented_command : documented_command_names) {
        if (!all_commands.contains(documented_command)) {
            qDebug() << documented_command;
        }
    }
}

void DocumentationController::show_documentation_with_title(QString doctype, QString title) {


    load_sioyek_documentation();

    if (!SHOW_DOCUMENTATION_IN_WIDGET) {

        const QJsonObject& command_name_to_file_name_map = sioyek_documentation_json_document["command_name_to_file_name_map"].toObject();
        const QJsonObject& config_name_to_file_name_map = sioyek_documentation_json_document["config_name_to_file_name_map"].toObject();

        QString file_title;
        if (doctype == "command") {
            file_title = command_name_to_file_name_map[title].toString();
        }
        else {
            file_title = config_name_to_file_name_map[title].toString();
        }
        open_documentation_file_for_name(doctype, file_title);
    }
    else {
        QString documentation_url = "";
        QString document;

        const QJsonObject& command_title_to_documentation_map = sioyek_documentation_json_document["command_title_to_documentation_map"].toObject();
        const QJsonObject& config_title_to_documentation_map = sioyek_documentation_json_document["config_title_to_documentation_map"].toObject();
        //for (auto key : command_title_to_documentation_map.keys()) {
        //    qDebug() << key;
        //    qDebug() << (key == title);
        //}

        if (doctype == "command") {
            if (command_title_to_documentation_map.contains(title)) {
                document = command_title_to_documentation_map[title].toString();
            }
        }
        else if (doctype == "config") {
            if (config_title_to_documentation_map.contains(title)) {
                document = config_title_to_documentation_map[title].toString();
            }
        }
        mw->show_markdown_text_widget(documentation_url, document);
    }


}

void DocumentationController::open_documentation_file_for_name(QString doctype, QString name) {
    if (name == "") {
        mw->push_state();
        mw->open_document_at_location(documentation_path.get_path(), 0, 0, 0, {}, false);
        mw->main_document_view->scroll_mid_to_top();
        return;
    }

    std::string nameddest_link = "file://sioyek_documentation.pdf#nameddest=" + doctype.toStdString() + ":" + name.toStdString();
    Document* documentation_document = mw->document_manager->get_document(documentation_path.get_path());

    if (!documentation_document->doc) {
        documentation_document->open();
    }

    ParsedUri parsed_uri = parse_uri(mw->mupdf_context, documentation_document->doc, nameddest_link);
    int page = parsed_uri.page - 1;
    mw->push_state();
    mw->open_document_at_location(documentation_path.get_path(), page, 0, parsed_uri.y, {}, false);

    if (ALIGN_LINK_DEST_TO_TOP) {
        mdv()->scroll_mid_to_top();
    }

    mw->invalidate_render();
}

void DocumentationController::show_markdown_text_widget(QString url, QString text) {

    SioyekDocumentationTextBrowser* text_edit = new SioyekDocumentationTextBrowser(mw);
    text_edit->setStyleSheet("QTextBrowser{" + get_status_stylesheet(false, DOCUMENTATION_FONT_SIZE) + "border-radius: 4px; padding: 10px;}\n" + get_scrollbar_stylesheet());
    text_edit->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

    int w = mw->width() * MENU_SCREEN_WDITH_RATIO;
    int h = mw->height() * MENU_SCREEN_HEIGHT_RATIO;
    text_edit->setReadOnly(true);
    text_edit->move(mw->width() / 2 - w / 2, mw->height() / 2 - h / 2);
    text_edit->resize(w, h);


    text_edit->setSource(url, QTextDocument::ResourceType::MarkdownResource);

    text_edit->setMarkdown(text);
    mw->push_current_widget(text_edit);
    text_edit->show();
}

void DocumentationController::show_command_documentation(QString command_name, bool is_config) {
    if (SHOW_DOCUMENTATION_IN_WIDGET) {
        SioyekDocumentationTextBrowser* text_edit = new SioyekDocumentationTextBrowser(mw);
        text_edit->setStyleSheet(get_status_stylesheet(false, DOCUMENTATION_FONT_SIZE));
        text_edit->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

        int w = mw->width() / 2;
        int h = mw->height() / 2;
        text_edit->setReadOnly(true);
        text_edit->move(mw->width() / 2 - w / 2, mw->height() / 2 - h / 2);
        text_edit->resize(w, h);

        QString documentation_url = "";
        QString doucmentation_name;
        auto doc = get_command_documentation(command_name, is_config, &documentation_url, &doucmentation_name);

        text_edit->setSource(documentation_url, QTextDocument::ResourceType::MarkdownResource);

        text_edit->setMarkdown(doc);
        mw->push_current_widget(text_edit);
        text_edit->show();
    }
    else {
        QString documentation_url = "";
        QString doucmentation_name;
        auto doc = get_command_documentation(command_name, is_config, &documentation_url, &doucmentation_name);
        QString doctype = documentation_url.startsWith("conf") ? "config" : "command";
        open_documentation_file_for_name(doctype, doucmentation_name);
    }
}

void DocumentationController::handle_documentation_search() {

    load_sioyek_documentation();
    mw->db_manager->index_documentation(sioyek_documentation_json_document);

    auto search_widget = DocumentationSearchWidget::create(mw);
    search_widget->set_select_fn([&, search_widget](int index) {
            FulltextSearchResult result = search_widget->result_model->search_results[index];
            QStringList parts = QString::fromStdWString(result.document_title).split("/");

            mw->pop_current_widget();
            if (parts[0] == "command") {
                show_documentation_with_title("command", parts[1]);
            }
            else if (parts[0] == "config") {

                show_documentation_with_title("config", parts[1]);
            }
            //qDebug() << result.document_title;
        });

    mw->set_current_widget(search_widget);
    mw->show_current_widget();
}
