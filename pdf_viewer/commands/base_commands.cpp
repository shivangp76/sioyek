#include "pch.h"

#include <qlocalsocket.h>
#include <qdir.h>

#include "main_widget.h"
#include "document_view.h"

#include "commands/command_parser.h"
#include "commands/base_commands.h"

extern bool USE_KEYBOARD_POINT_SELECTION;
extern bool SHOW_MOST_RECENT_COMMANDS_FIRST;
extern bool FILL_TEXTBAR_WITH_SELECTED_TEXT;
extern bool NUMERIC_TAGS;
extern bool HIDE_OVERLAPPING_LINK_LABELS;

void CommandManager::update_command_last_use(std::string command_name) {
    command_last_uses[command_name] = QDateTime::currentDateTime();
}

void CommandManager::handle_new_javascript_command(std::wstring command_name_, JsCommandInfo info, bool is_async, std::wstring preloaded_code) {
    // if the code is available in the preloaded_code, we can use that instead of reading from the file
    std::string command_name = utf8_encode(command_name_);
    auto [command_parent_file_path, line_number, command_file_path, entry_point] = info;

    if (preloaded_code.size() > 0) {
        new_commands[command_name] = [command_name, preloaded_code, entry_point = entry_point, is_async, this](MainWidget* w) {
            return std::make_unique<JavascriptCommand>(command_name, preloaded_code, entry_point, is_async, w);
            };
        return;
    }

    QDir parent_dir = QFileInfo(QString::fromStdWString(command_parent_file_path)).dir();
    QFileInfo javascript_file_info(QString::fromStdWString(command_file_path));
    QString absolute_file_path;

    if (javascript_file_info.filePath().startsWith("~")){
        absolute_file_path = QDir::homePath() + javascript_file_info.filePath().mid(1);
    }
    else if (javascript_file_info.isRelative()) {
        absolute_file_path = parent_dir.absoluteFilePath(javascript_file_info.filePath());
    }
    else {
        absolute_file_path = javascript_file_info.absoluteFilePath();
    }
    QFile code_file(absolute_file_path);
    if (code_file.open(QIODevice::ReadOnly)) {
        QTextStream in(&code_file);
        QString code = in.readAll();
        new_commands[command_name] = [command_name, code, entry_point = entry_point, is_async, this](MainWidget* w) {
            return std::make_unique<JavascriptCommand>(command_name, code.toStdWString(), entry_point, is_async, w);
            };
    }
    else{
        qDebug() << "Could not open " << absolute_file_path;
    }
}

std::unique_ptr<Command> CommandManager::get_command_with_name(MainWidget* w, std::string name) {

    if (new_commands.find(name) != new_commands.end()) {
        return new_commands[name](w);
    }
    return nullptr;
}

QStringList CommandManager::get_all_command_names() {
    std::vector<std::pair<QDateTime, QString>> pairs;
    QStringList res;

    if (SHOW_MOST_RECENT_COMMANDS_FIRST) {
        for (const auto& com : new_commands) {
            pairs.push_back(std::make_pair(command_last_uses[com.first], QString::fromStdString(com.first)));
        }

        std::stable_sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
            });

        for (auto [_, com] : pairs) {
            res.push_back(com);
        }
    }
    else {
        for (const auto& com : new_commands) {
            res.push_back(QString::fromStdString(com.first));
        }
    }


    return res;
}

std::unique_ptr<Command> CommandManager::create_macro_command(MainWidget* w, std::string name, std::wstring macro_string) {
    return std::make_unique<MacroCommand>(w, this, name, macro_string);
}

std::unique_ptr<Command> CommandManager::create_macro_command_with_args(MainWidget* w, std::string name, QString command, QStringList args) {
    return std::make_unique<MacroCommand>(w, this, name, command, args);
}

Command::Command(std::string name, MainWidget* widget_) : command_cname(name), widget(widget_) {

}

void Command::on_text_change(const QString& new_text) {

}

DocumentView* Command::dv(){

    if (widget){
        return widget->dv();
    }

    return nullptr;
}

bool Command::is_holdable() {
    return false;
}

void Command::on_key_hold() {

}

bool Command::is_menu_command() {
    // returns true if the command is a macro designed to run only on menus (m)
    // we then ignore keys in menus if they are bound to such commands so they
    // can be later handled by our own InputHandler
    return false;
}

Command::~Command() {

}

QStringList Command::get_autocomplete_options() {
    return {};
}

std::optional<std::wstring> Command::get_text_suggestion(int index) {
    return {};
}

void Command::perform_up() {
}

void Command::set_result_socket(QLocalSocket* socket) {
    result_socket = socket;
}

void Command::set_result_mutex(bool* id, std::wstring* result_location) {
    is_done = id;
    result_holder = result_location;
}

void Command::set_generic_requirement(QVariant value) {

}

void Command::handle_generic_requirement() {

}

AbsoluteRect get_rect_from_string(std::wstring str) {
    QStringList parts = QString::fromStdWString(str).split(' ');

    AbsoluteRect result;
    result.x0 = parts[0].toFloat();
    result.y0 = parts[1].toFloat();
    result.x1 = parts[2].toFloat();
    result.y1 = parts[3].toFloat();
    return result;
}

AbsoluteDocumentPos get_point_from_string(std::wstring str) {
    QStringList parts = QString::fromStdWString(str).split(' ');

    AbsoluteDocumentPos result;
    result.x = parts[0].toFloat();
    result.y = parts[1].toFloat();
    return result;
}

void Command::set_next_requirement_with_string(std::wstring str) {
    std::optional<Requirement> maybe_req = next_requirement(widget);
    if (maybe_req) {
        Requirement req = maybe_req.value();
        if (req.type == RequirementType::Text || req.type == RequirementType::Password || req.type == RequirementType::OptionalText) {
            set_text_requirement(str);
        }
        else if (req.type == RequirementType::Symbol) {
            set_symbol_requirement(str[0]);
        }
        else if (req.type == RequirementType::File || req.type == RequirementType::Folder) {
            set_file_requirement(str);
        }
        else if (req.type == RequirementType::Rect) {
            set_rect_requirement(get_rect_from_string(str));
        }
        else if (req.type == RequirementType::Point) {
            if (USE_KEYBOARD_POINT_SELECTION && is_alpha_only(str)) {
                int index = get_index_from_tag(utf8_encode(str));
                AbsoluteDocumentPos abspos = dv()->get_index_document_pos(index).to_absolute(widget->doc());
                set_point_requirement(abspos);
            }
            else {
                set_point_requirement(get_point_from_string(str));
            }
        }
        else if (req.type == RequirementType::Generic) {
            set_generic_requirement(QString::fromStdWString(str));
        }
    }
}

std::optional<Requirement> Command::next_requirement(MainWidget* widget) {
    return {};
}

std::optional<std::wstring> Command::get_result() {
    return result;
}

void Command::set_text_requirement(std::wstring value) {}
void Command::set_symbol_requirement(char value) {}
void Command::set_file_requirement(std::wstring value) {}
void Command::set_rect_requirement(AbsoluteRect value) {}
void Command::set_point_requirement(AbsoluteDocumentPos value) {}

std::wstring Command::get_text_default_value() {
    return L"";
}

bool Command::pushes_state() {
    return false;
}

bool Command::requires_document() {
    return true;
}

void Command::set_num_repeats(int nr) {
    num_repeats = nr;
}

int Command::get_num_repeats() {
    return num_repeats;
}

void Command::pre_perform() {

}

void Command::on_cancel() {

}

std::optional<AbsoluteRect> Command::get_text_editor_rectangle() {
    return {};
}

void Command::run() {
    if (this->requires_document() && !(widget->main_document_view_has_document())) {
        return;
    }
    widget->add_command_being_performed(this);
    perform();
    widget->validate_render();
    on_result_computed();
    widget->remove_command_being_performed(this);
}

std::vector<char> Command::special_symbols() {
    std::vector<char> res;
    return res;
}


std::string Command::get_name() {
    return command_cname;
}

std::string Command::get_symbol_hint_name() {
    return get_name();
}

void Command::on_result_computed() {

    if (dynamic_cast<MacroCommand*>(this)) {
        return;
    }
    if (dynamic_cast<LazyCommand*>(this)) {
        return;
    }
    if (result_socket && result_socket->isOpen()) {
        if (!result.has_value()) {
            // result_socket->write("<NULL>");
            result_socket->write("<RESULT>\n");
            return;
        }
        std::string result_str = utf8_encode(result.value());
        if (result_str.size() > 0) {
            result_str = "<RESULT>\n" + result_str;
            result_socket->write(result_str.c_str());
        }
        else {
            // result_socket->write("<NULL>");
            result_socket->write("<RESULT>\n");
            result_socket->flush();
        }
    }
    if (is_done) {
        if (result) {
            *result_holder = result.value();
        }
        else {
            *result_holder = L"";
        }

        *is_done = true;
    }

}

std::string Command::get_human_readable_name() {
    std::string res = get_name();
    if (widget->command_manager->command_human_readable_names.find(res) != widget->command_manager->command_human_readable_names.end()) {
        return widget->command_manager->command_human_readable_names[res];
    }

    return res;
}

GenericPathCommand::GenericPathCommand(std::string name, MainWidget* w) : Command(name, w){

}

std::optional<Requirement> GenericPathCommand::next_requirement(MainWidget* widget) {
    if (selected_path) {
        return {};
    }
    else {
        return Requirement{ RequirementType::Generic, "File path" };
    }
}

void GenericPathCommand::set_generic_requirement(QVariant value) {
    selected_path = value.toString().toStdWString();
}

GenericGotoLocationCommand::GenericGotoLocationCommand(std::string name, MainWidget* w) : Command(name, w) {};

std::optional<Requirement> GenericGotoLocationCommand::next_requirement(MainWidget* widget) {
    if (target_location.has_value()) {
        return {};
    }
    else {
        return Requirement{ RequirementType::Generic, "Target Location" };
    }
}

void GenericGotoLocationCommand::set_generic_requirement(QVariant value) {
    target_location = value.toFloat();
}

void GenericGotoLocationCommand::perform() {
    widget->main_document_view->set_offset_y(target_location.value());
    widget->validate_render();
}

bool GenericGotoLocationCommand::pushes_state() {
    return true;
}

GenericPathAndLocationCommadn::GenericPathAndLocationCommadn(std::string name, MainWidget* w, bool is_hash_) : Command(name, w) { is_hash = is_hash_; }
inline std::optional<Requirement> GenericPathAndLocationCommadn::next_requirement(MainWidget* widget) {
    if (target_location) {
        return {};
    }
    else {
        return Requirement{ RequirementType::Generic, "Location" };
    }
}

void GenericPathAndLocationCommadn::set_generic_requirement(QVariant value) {
    target_location = value;
}

void GenericPathAndLocationCommadn::perform() {
    QList<QVariant> values = target_location.value().toList();
    std::wstring file_name = values[0].toString().toStdWString();

    if (values.size() == 1) {
        if (is_hash) {
            widget->open_document_with_hash(utf8_encode(file_name));
            widget->validate_render();
        }
        else {
            widget->open_document(file_name);
            widget->validate_render();
        }
    }
    else if (values.size() == 2) {
        float y_offset = values[1].toFloat();
        widget->open_document(file_name, 0.0f, y_offset);
        widget->validate_render();
    }
    else {
        float x_offset = values[1].toFloat();
        float y_offset = values[2].toFloat();

        widget->open_document(file_name, x_offset, y_offset);
        widget->validate_render();
    }
}

bool GenericPathAndLocationCommadn::pushes_state() {
    return true;
}

class NoopCommand : public Command {
public:
    static inline const std::string cname = "noop";
    static inline const std::string hname = "Do nothing";

    NoopCommand(MainWidget* widget_) : Command(cname, widget_) {}

    void perform() {
    }

    bool requires_document() { return false; }
};

std::optional<std::wstring> LazyCommand::get_result() {
    if (actual_command) {
        return actual_command->get_result();
    }
    return {};
}

void LazyCommand::set_result_socket(QLocalSocket* socket) {
    noop->set_result_socket(socket);
    result_socket = socket;
    if (actual_command) {
        actual_command->set_result_socket(socket);
    }
}

void LazyCommand::set_result_mutex(bool* p_is_done, std::wstring* result_location) {
    result_holder = result_location;
    is_done = p_is_done;
    if (actual_command) {
        actual_command->set_result_mutex(p_is_done, result_location);
    }
}

Command* LazyCommand::get_command() {
    if (actual_command) {
        return actual_command.get();
    }
    else {
        actual_command = std::move(command_manager->get_command_with_name(widget, command_name));
        if (!actual_command) return noop.get();

        for (auto arg : command_params) {
            actual_command->set_next_requirement_with_string(arg);
        }

        if (actual_command && (result_socket != nullptr)) {
            actual_command->set_result_socket(result_socket);
        }

        if (actual_command && (is_done != nullptr)) {
            actual_command->set_result_mutex(is_done, result_holder);
        }

        return actual_command.get();
    }
}

LazyCommand::LazyCommand(std::string name, MainWidget* widget_, CommandManager* manager, CommandInvocation invocation) : Command(name, widget_) {
    command_manager = manager;
    command_name = invocation.command_name.toStdString();
    for (auto arg : invocation.command_args) {
        command_params.push_back(arg.toStdWString());
    }
    noop = std::make_unique<NoopCommand>(widget_);

    //parse_command_text(command_text);
}

void LazyCommand::set_text_requirement(std::wstring value) { get_command()->set_text_requirement(value); }
void LazyCommand::set_symbol_requirement(char value) { get_command()->set_symbol_requirement(value); }
void LazyCommand::set_file_requirement(std::wstring value) { get_command()->set_file_requirement(value); }
void LazyCommand::set_rect_requirement(AbsoluteRect value) { get_command()->set_rect_requirement(value); }
void LazyCommand::set_generic_requirement(QVariant value) { get_command()->set_generic_requirement(value); }
void LazyCommand::handle_generic_requirement() { get_command()->handle_generic_requirement(); }
void LazyCommand::set_point_requirement(AbsoluteDocumentPos value) { get_command()->set_point_requirement(value); }
void LazyCommand::set_num_repeats(int nr) { get_command()->set_num_repeats(nr); }
std::vector<char> LazyCommand::special_symbols() { return get_command()->special_symbols(); }
void LazyCommand::pre_perform() { get_command()->pre_perform(); }
bool LazyCommand::pushes_state() { return get_command()->pushes_state(); }
bool LazyCommand::requires_document() { return get_command()->requires_document(); }

std::optional<Requirement> LazyCommand::next_requirement(MainWidget* widget) {
    return get_command()->next_requirement(widget);
}

void LazyCommand::perform() {
    auto com = get_command();
    if (com) {
        com->run();
    }
}

std::string LazyCommand::get_name() {
    auto com = get_command();
    if (com) {
        return com->get_name();
    }
    else {
        return "";
    }
}

std::unique_ptr<Command> MacroCommand::get_subcommand(CommandInvocation invocation) {
    std::string subcommand_name = invocation.command_string().toStdString();
    auto subcommand = widget->command_manager->get_command_with_name(widget, subcommand_name);
    if (subcommand) {
        for (auto arg : invocation.command_args) {
            subcommand->set_next_requirement_with_string(arg.toStdWString());
        }
        return std::move(subcommand);
    }
    else {
        return std::move(std::make_unique<LazyCommand>(subcommand_name, widget, widget->command_manager, invocation));
    }
}

void MacroCommand::set_result_socket(QLocalSocket* rsocket) {
    result_socket = rsocket;
    for (auto& subcommand : commands) {
        subcommand->set_result_socket(result_socket);
    }
}

void MacroCommand::set_result_mutex(bool* id, std::wstring* result_location) {
    is_done = id;
    result_holder = result_location;
    for (auto& subcommand : commands) {
        subcommand->set_result_mutex(id, result_location);
    }
}

void MacroCommand::initialize_from_invocations(std::vector<CommandInvocation> command_invocations) {
    for (auto command_invocation : command_invocations) {
        if (command_invocation.command_name.size() > 0) {
            if (command_invocation.command_name[0] == '[') {
                is_modal = true;
            }
        }
    }

    for (auto command_invocation : command_invocations) {
        if (command_invocation.command_name.size() > 0) {

            if (is_modal) {
                modes.push_back(command_invocation.mode_string().toStdString());
            }

            commands.push_back(get_subcommand(command_invocation));
            performed.push_back(false);
            pre_performed.push_back(false);
        }
    }
}

MacroCommand::MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::vector<CommandInvocation> command_invocations) : Command(name_, widget_) {
    command_manager = manager;
    name = name_;

    initialize_from_invocations(command_invocations);
}

MacroCommand::MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::vector<std::unique_ptr<Command>>&& cmds) : Command(name_, widget_) {
    command_manager = manager;
    name = name_;

    for (auto&& cmd : cmds) {
        commands.push_back(std::move(cmd));
        performed.push_back(false);
        pre_performed.push_back(false);
    }
}

MacroCommand::MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, QString command_name, QStringList args) : Command(name_, widget_) {
    command_manager = manager;
    name = name_;
    CommandInvocation invocation;
    invocation.command_name = command_name;
    invocation.command_args = args;
    initialize_from_invocations({ invocation });

}

MacroCommand::MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(name_, widget_) {
    //commands = std::move(commands_);
    command_manager = manager;
    name = name_;
    raw_commands = commands_;


    QString str = QString::fromStdWString(commands_);
    ParseState parser;
    parser.str = str;
    parser.index = 0;
    std::vector<CommandInvocation> command_invocations = parser.parse_next_macro_command();

    initialize_from_invocations(command_invocations);
}

std::optional<std::wstring> MacroCommand::get_result() {
    if (commands.size() > 0) {
        return commands.back()->get_result();
    }
    return {};
}

void MacroCommand::set_text_requirement(std::wstring value) {
    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            commands[current_mode_index]->set_text_requirement(value);
        }
    }
    else {
        for (int i = 0; i < commands.size(); i++) {
            std::optional<Requirement> req = commands[i]->next_requirement(widget);
            if (req) {
                if (req.value().type == RequirementType::Text || req.value().type == RequirementType::Password) {
                    commands[i]->set_text_requirement(value);
                }
                return;
            }
        }
    }
}

bool MacroCommand::is_menu_command() {
    if (is_modal) {
        bool res = false;
        for (std::string mode : modes) {
            if (mode == "m") {
                res = true;
            }
        }
        return res;
    }
    return false;
}

void MacroCommand::set_generic_requirement(QVariant value) {
    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            commands[current_mode_index]->set_generic_requirement(value);
        }
    }
    else {
        for (int i = 0; i < commands.size(); i++) {
            std::optional<Requirement> req = commands[i]->next_requirement(widget);
            if (req) {
                if (req.value().type == RequirementType::Generic) {
                    commands[i]->set_generic_requirement(value);
                }
                return;
            }
        }
    }
}

void MacroCommand::handle_generic_requirement() {
    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            commands[current_mode_index]->handle_generic_requirement();
        }
    }
    else {
        for (int i = 0; i < commands.size(); i++) {
            std::optional<Requirement> req = commands[i]->next_requirement(widget);
            if (req) {
                if (req.value().type == RequirementType::Generic) {
                    commands[i]->handle_generic_requirement();
                }
                return;
            }
        }
    }
}

void MacroCommand::set_symbol_requirement(char value) {
    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            commands[current_mode_index]->set_symbol_requirement(value);
        }
    }
    else {

        for (int i = 0; i < commands.size(); i++) {
            std::optional<Requirement> req = commands[i]->next_requirement(widget);
            if (req) {
                if (req.value().type == RequirementType::Symbol) {
                    commands[i]->set_symbol_requirement(value);
                }
                return;
            }
        }
    }
}

bool MacroCommand::requires_document() {

    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0 && current_mode_index < commands.size()) {
            return commands[current_mode_index]->requires_document();
        }
        return false;
    }
    else {
        for (int i = 0; i < commands.size(); i++) {
            if (commands[i]->requires_document()) {
                return true;
            }
        }
        return false;
    }

    return true;
}

void MacroCommand::set_file_requirement(std::wstring value) {
    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            commands[current_mode_index]->set_file_requirement(value);
        }
    }
    else {
        for (int i = 0; i < commands.size(); i++) {
            std::optional<Requirement> req = commands[i]->next_requirement(widget);
            if (req) {
                if (req->type == RequirementType::File || req->type == RequirementType::Folder) {
                    commands[i]->set_file_requirement(value);
                }
                return;
            }
        }
    }
}

int MacroCommand::get_command_index_for_requirement_type(RequirementType reqtype) {
    if (is_modal) {
        return get_current_mode_index();
    }
    else {
        for (int i = 0; i < commands.size(); i++) {
            std::optional<Requirement> req = commands[i]->next_requirement(widget);
            if (req) {
                if (req.value().type == reqtype) {
                    return i;
                }
                return -1;
            }
        }
    }
    return -1;
}

void MacroCommand::set_rect_requirement(AbsoluteRect value) {
    int index = get_command_index_for_requirement_type(RequirementType::Rect);
    if (index >= 0) {
        commands[index]->set_rect_requirement(value);
    }
}

void MacroCommand::set_point_requirement(AbsoluteDocumentPos value) {
    int index = get_command_index_for_requirement_type(RequirementType::Point);
    if (index >= 0) {
        commands[index]->set_point_requirement(value);
    }
}

std::wstring MacroCommand::get_text_default_value() {
    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            return commands[current_mode_index]->get_text_default_value();
        }
    }
    else {
        int index = get_current_executing_command_index();
        if (index >= 0 && index < commands.size()) {
            return commands[index]->get_text_default_value();
        }
    }
}

void MacroCommand::pre_perform() {
    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            commands[current_mode_index]->pre_perform();
        }
    }
    else {
        for (int i = 0; i < performed.size(); i++) {
            if (performed[i] == false) {
                commands[i]->pre_perform();
                return;
            }
        }
    }
}

std::optional<Requirement> MacroCommand::next_requirement(MainWidget* widget) {
    if (is_modal && (modes.size() != commands.size())) {
        std::wcerr << L"Invalid modal command : " << raw_commands;
        return {};
    }

    if (is_modal) {
        int current_mode_index = get_current_mode_index();
        if (current_mode_index >= 0) {
            return commands[current_mode_index]->next_requirement(widget);
        }
        return {};
    }
    else {
        for (int i = 0; i < commands.size(); i++) {
            if (commands[i]->next_requirement(widget)) {
                return commands[i]->next_requirement(widget);
            }
            else {
                if (!performed[i]) {
                    perform_subcommand(i);
                }
            }
        }
        return {};
    }
}

void MacroCommand::perform_subcommand(int index) {
    if (!performed[index]) {
        if (commands[index]->pushes_state()) {
            widget->push_state();
        }

        // addresses https://github.com/ahrm/sioyek/issues/1114#issuecomment-2465490589
        // e.g. when defining a custom command like goto_beginning;fit_to_page_smart we want to pass the num_repeats to goto_beginning
        // of course since there are multiple commands it is ambiguous which command actually gets the num_repeats, here we just pass
        // it to the first command
        if (index == 0 && (num_repeats > 0)) {
            commands[index]->set_num_repeats(num_repeats);
        }
        else {
            commands[index]->set_num_repeats(0);
        }

        commands[index]->run();
        performed[index] = true;
    }
}

bool MacroCommand::is_enabled() {
    if (!is_modal) {
        return commands.size() > 0;
    }
    else {
        std::string mode_string = widget->get_current_mode_string();
        for (int i = 0; i < modes.size(); i++) {
            if (mode_matches(mode_string, modes[i])) {
                return true;
            }
        }
        return false;
    }

}

bool MacroCommand::is_holdable() {
    if (!is_modal) {
        return false;
    }
    else {
        int mode_index = get_current_mode_index();
        if (mode_index >= 0) {
            return commands[mode_index]->is_holdable();
        }
        return false;
    }
}

void MacroCommand::perform_up() {
    if (is_holdable()) {
        int mode_index = get_current_mode_index();
        if (mode_index >= 0) {
            commands[mode_index]->perform_up();
        }
    }
}

void MacroCommand::on_key_hold() {
    if (is_holdable()) {
        int mode_index = get_current_mode_index();
        if (mode_index >= 0) {
            commands[mode_index]->on_key_hold();
        }
    }
}

void MacroCommand::perform() {
    if (!is_modal) {
        for (int i = 0; i < commands.size(); i++) {

            if (!performed[i]) {
                perform_subcommand(i);
            }
        }
    }
    else {
        if (modes.size() != commands.size()) {
            qDebug() << "Invalid modal command : " << QString::fromStdWString(raw_commands);
            return;
        }
        std::string mode_string = widget->get_current_mode_string();

        for (int i = 0; i < commands.size(); i++) {
            if (mode_matches(mode_string, modes[i])) {
                perform_subcommand(i);
                return;
            }
        }
    }
}

int MacroCommand::get_current_mode_index() {
    if (is_modal) {
        std::string mode_str = widget->get_current_mode_string();
        for (int i = 0; i < modes.size(); i++) {
            if (mode_matches(mode_str, modes[i])) {
                return i;
            }
        }
        return -1;

    }

    return -1;
}

int MacroCommand::get_current_executing_command_index() {
    if (is_modal) {
        return get_current_mode_index();
    }
    else {
        for (int i = 0; i < performed.size(); i++) {
            if (!performed[i]) return i;
        }
    }
    return -1;
}

void MacroCommand::on_text_change(const QString& new_text) {
    if (is_modal) {
        int mode_index = get_current_mode_index();
        if (mode_index != -1) {
            commands[mode_index]->on_text_change(new_text);
        }
    }
    else {
        if (commands.size() == 1) {
            commands[0]->on_text_change(new_text);
        }
    }
}

std::vector<char> MacroCommand::special_symbols() {
    int command_index = get_current_executing_command_index();
    if (command_index != -1) {
        return commands[command_index]->special_symbols();
    }
    return {};
}

bool MacroCommand::mode_matches(std::string current_mode, std::string command_mode) {
    for (auto c : command_mode) {
        if (current_mode.find(c) == std::string::npos) {
            return false;
        }
    }
    return true;
}

std::string MacroCommand::get_name() {
    if (name.size() > 0 || commands.size() == 0) {
        return name;
    }
    else {
        if (is_modal) {
            int mode_index = get_current_mode_index();
            if (mode_index != -1) {
                return commands[mode_index]->get_name();
            }
            return "";
        }
        else {

            std::string res;
            for (auto& command : commands) {
                res += command->get_name();
            }
            return res;
        }
        //return "[macro]" + commands[0]->get_name();
    }
}

std::string MacroCommand::get_human_readable_name() {
    if (name.size() > 0 || commands.size() == 0) {
        return name;
    }
    else {
        if (is_modal) {
            int mode_index = get_current_mode_index();
            if (mode_index != -1) {
                return commands[mode_index]->get_human_readable_name();
            }
            return "";
        }
        else {

            std::string res;
            for (auto& command : commands) {
                res += command->get_human_readable_name();
            }
            return res;
        }
        //return "[macro]" + commands[0]->get_name();
    }
}


JavascriptCommand::JavascriptCommand(std::string command_name, std::wstring code_, std::optional<std::wstring> entry_point_, bool is_async_, MainWidget* w) : Command(command_name, w), command_name(command_name) {
    code = code_;
    entry_point = entry_point_;
    is_async = is_async_;
};

void JavascriptCommand::perform() {
    widget->run_javascript_command(code, entry_point, is_async);
}

std::string JavascriptCommand::get_name() {
    return command_name;
}


SymbolCommand::SymbolCommand(std::string name, MainWidget* w) : Command(name, w) {}

std::optional<Requirement> SymbolCommand::next_requirement(MainWidget* widget) {
    if (symbol == 0) {
        return Requirement{ RequirementType::Symbol, "symbol" };
    }
    else {
        return {};
    }
}

void SymbolCommand::set_symbol_requirement(char value) {
    this->symbol = value;
}

std::string SymbolCommand::get_human_readable_name() {
    auto res = Command::get_human_readable_name();
    if (symbol != 0) {
        res += "(";
        res.push_back(symbol);
        res += ")";
    }
    return res;
}

TextCommand::TextCommand(std::string name, MainWidget* w) : Command(name, w) {}

std::string TextCommand::text_requirement_name() {
    return "text";
}

std::optional<Requirement> TextCommand::next_requirement(MainWidget* widget) {
    if (text.has_value()) {
        return {};
    }
    else {
        return Requirement{ RequirementType::Text, text_requirement_name() };
    }
}

void TextCommand::set_text_requirement(std::wstring value) {
    this->text = value;
}

std::wstring TextCommand::get_text_default_value() {
    if (FILL_TEXTBAR_WITH_SELECTED_TEXT) {
        return widget->dv()->get_selected_text();
    }
    return L"";
}

GenericWaitCommand::GenericWaitCommand(std::string name, MainWidget* w) : Command(name, w) {};

std::optional<Requirement> GenericWaitCommand::next_requirement(MainWidget* widget) {
    if (!finished) {
        return Requirement{ RequirementType::Generic, "dummy" };
    }
    else {
        return {};
    }
}

GenericWaitCommand::~GenericWaitCommand() {

    if (timer) {
        timer->stop();
        timer->deleteLater();
    }
}

void GenericWaitCommand::set_generic_requirement(QVariant value) {
    finished = true;
}

void GenericWaitCommand::handle_generic_requirement() {
    if (finished) return;

    timer = new QTimer();
    timer->setInterval(100);
    timer_connection = QObject::connect(timer, &QTimer::timeout, [this]() {
        if (is_ready()) {
            widget->advance_waiting_command(get_name());
        }
        });
    timer->start();

}

void GenericWaitCommand::perform() {
}

JsCallCommand::JsCallCommand(std::string command_name, std::wstring func_, MainWidget* widget) : Command(command_name, widget) {
    funcall = func_ + L"()";
};

void JsCallCommand::perform() {
    widget->run_javascript_command(funcall, {}, false);
}

std::string JsCallCommand::get_name() {
    return command_name;
}

KeyboardSelectPointCommand::KeyboardSelectPointCommand(MainWidget* w, std::unique_ptr<Command> original_command) : Command("keyboard_point_select", w) {
    origin = std::move(original_command);
    std::optional<Requirement> next_requirement = origin->next_requirement(widget);
    if (next_requirement && next_requirement->type == Rect) {
        requires_rect = true;
    }
    //if(origin->next_requirement(widget) && origin->next_requirement(widget)-> )
};

void KeyboardSelectPointCommand::on_cancel() {
    origin->on_cancel();
    widget->set_highlighted_tags({});

}

bool KeyboardSelectPointCommand::is_done() {
    if (!text.has_value()) return false;

    if (requires_rect) {
        return text.value().size() == 4;
    }
    else {
        return text.value().size() == 2;
    }
}

std::optional<Requirement> KeyboardSelectPointCommand::next_requirement(MainWidget* widget) {
    bool done = is_done();

    if (done) {
        return {};
    }
    else {
        if (requires_rect) {
            if (text.has_value() && text.value().size() >= 2) {
                return Requirement{ RequirementType::Symbol, "bottom right location" };
            }
            else {
                return Requirement{ RequirementType::Symbol, "top left location" };
            }
        }
        else {
            return Requirement{ RequirementType::Symbol, "point location" };
        }
    }
}

void KeyboardSelectPointCommand::perform() {

    widget->clear_tag_prefix();
    widget->set_highlighted_tags({});
    result = text.value();

    if (requires_rect) {

        std::string tag1 = utf8_encode(result.value().substr(0, 2));
        std::string tag2 = utf8_encode(result.value().substr(2, 2));
        int index1 = get_index_from_tag(tag1);
        int index2 = get_index_from_tag(tag2);
        AbsoluteDocumentPos pos1 = dv()->get_index_document_pos(index1).to_absolute(widget->doc());
        AbsoluteDocumentPos pos2 = dv()->get_index_document_pos(index2).to_absolute(widget->doc());
        AbsoluteRect rect(pos1, pos2);
        origin->set_rect_requirement(rect);
        widget->set_rect_select_mode(false);
        //origin->set_point_requirement(abspos);
    }
    else {

        int index = get_index_from_tag(utf8_encode(result.value()));
        AbsoluteDocumentPos abspos = dv()->get_index_document_pos(index).to_absolute(widget->doc());
        origin->set_point_requirement(abspos);
    }

    dv()->clear_keyboard_select_highlights();
    widget->advance_command(std::move(origin));
}

void KeyboardSelectPointCommand::pre_perform() {
    if (already_pre_performed) return;

    widget->clear_tag_prefix();
    dv()->highlight_window_points();
    widget->invalidate_render();
    already_pre_performed = true;
}

std::string KeyboardSelectPointCommand::get_name() {
    return "keyboard_point_select";
}


void KeyboardSelectPointCommand::set_symbol_requirement(char value) {
    if (text.has_value()) {
        text.value().push_back(value);
    }
    else {
        std::wstring val;
        val.push_back(value);
        this->text = val;
    }

    if (!is_done()) {
        if (requires_rect && (text.value().size() >= 2)) {
            widget->set_tag_prefix(text.value().substr(2));
        }
        else {
            widget->set_tag_prefix(text.value());
        }
    }
    // update selected tags
    if (text.has_value()) {
        if (text->size() >= 2) {
            std::string tag = utf8_encode(text.value().substr(0, 2));
            widget->set_highlighted_tags({ tag });
        }
    }
}

OpenLinkCommand::OpenLinkCommand(MainWidget* w) : Command(cname, w) {};
OpenLinkCommand::OpenLinkCommand(std::string name, MainWidget* w) : Command(name, w) {};

std::string OpenLinkCommand::text_requirement_name() {
    return "Label";
}

bool OpenLinkCommand::is_done() {
    if ((NUMERIC_TAGS && text.has_value())) return true;
    if ((!NUMERIC_TAGS) && text.has_value() && (text.value().size() == get_num_tag_digits(widget->num_visible_links()))) return true;
    return false;
}

std::optional<Requirement> OpenLinkCommand::next_requirement(MainWidget* widget) {
    bool done = is_done();

    if (done) {
        return {};
    }
    else {
        if (!NUMERIC_TAGS) {
            return Requirement{ RequirementType::Symbol, "Label" };
        }
        else if ((widget->num_visible_links() < 10) && (NUMERIC_TAGS)) {
            return Requirement{ RequirementType::Symbol, "Label" };
        }
        else {
            return Requirement{ RequirementType::Text, text_requirement_name() };
        }
    }
}

void OpenLinkCommand::perform_with_link(PdfLink link) {
    widget->handle_open_link(link);
}

void OpenLinkCommand::perform() {
    //widget->handle_open_link(text.value());
    if (!already_pre_performed) {
        pre_perform();
    }

    int index = get_index_from_tag(utf8_encode(text.value()));
    if (index >= 0 && index < tagged_links.size()) {
        PdfLink selected_link = tagged_links[index];
        perform_with_link(selected_link);
    }
    widget->dv()->set_highlight_words({});
    widget->set_highlight_links(false);

}

void OpenLinkCommand::pre_perform() {
    if (already_pre_performed) return;
    already_pre_performed = true;

    std::vector<PdfLink> visible_links;
    std::vector<PdfLink> actual_tagged_links;
    widget->clear_tag_prefix();
    widget->dv()->get_visible_links(visible_links);

    if (visible_links.size() > 0) {
        actual_tagged_links.push_back(visible_links[0]);

        for (int j = 1; j < visible_links.size(); j++) {
            if (HIDE_OVERLAPPING_LINK_LABELS) {
                auto other_link = actual_tagged_links.back();
                auto link = visible_links[j];
                float distance = std::abs(other_link.rects[0].x0 - link.rects[0].x0) + std::abs(other_link.rects[0].y0 - link.rects[0].y0);
                if (distance > 10) {
                    actual_tagged_links.push_back(link);
                }

            }
            else {
                actual_tagged_links.push_back(visible_links[j]);
            }
        }
    }

    std::vector<DocumentRect> visible_link_rects;

    for (auto link : actual_tagged_links) {
        visible_link_rects.push_back(DocumentRect{ link.rects[0] , link.source_page});
    }

    tagged_links = actual_tagged_links;
    
    widget->dv()->set_highlight_words(visible_link_rects);
    widget->dv()->set_should_highlight_words(true);
    widget->set_highlight_links(true);
}

void OpenLinkCommand::set_text_requirement(std::wstring value) {
    this->text = value;
}

void OpenLinkCommand::set_symbol_requirement(char value) {
    if (text.has_value()) {
        text.value().push_back(value);
    }
    else {
        std::wstring val;
        val.push_back(value);
        this->text = val;
    }

    if (!is_done()) {
        widget->set_tag_prefix(text.value());
    }
}

TagCommand::TagCommand(std::string cname, MainWidget* w) : Command(cname, w) {

}

bool TagCommand::is_done() {
    return get_num_tag_digits(tags.size()) == selected_tag.size();
}

std::optional<Requirement> TagCommand::next_requirement(MainWidget* widget) {
    if (is_done()) {
        return {};
    }
    else {
        return Requirement{ RequirementType::Symbol, "tag" };
    }
}

void TagCommand::set_symbol_requirement(char value) {
    selected_tag.push_back(value);
    if (!is_done()) {
        widget->set_tag_prefix(utf8_decode(selected_tag));
    }
}

void TagCommand::pre_perform() {
    if (!pre_performed) {
        widget->clear_tag_prefix();
        tags = dv()->highlight_words();
        pre_performed = true;
    }
}

CustomCommand::CustomCommand(MainWidget* widget_, std::string name_, std::wstring command_) : Command(name_, widget_) {
    raw_command = command_;
    name = name_;
}

std::optional<Requirement> CustomCommand::next_requirement(MainWidget* widget) {
    if (command_requires_rect(raw_command) && (!command_rect.has_value())) {
        Requirement req = { RequirementType::Rect, "Command Rect" };
        return req;
    }
    if (command_requires_text(raw_command) && (!command_text.has_value())) {
        Requirement req = { RequirementType::Text, "Command Text" };
        return req;
    }
    return {};
}

void CustomCommand::set_rect_requirement(AbsoluteRect rect) {
    command_rect = rect;
}

void CustomCommand::set_point_requirement(AbsoluteDocumentPos point) {
    command_point = point;
}

void CustomCommand::set_text_requirement(std::wstring txt) {
    command_text = txt;
}

void CustomCommand::perform() {
    widget->execute_command(raw_command, command_text.value_or(L""));
}

std::string CustomCommand::get_name() {
    return name;
}

HoldableCommand::HoldableCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(name_, widget_) {
    command_manager = manager;
    name = name_;
    QString str = QString::fromStdWString(commands_);
    QStringList parts = str.split('|');
    if (parts.size() == 2) {
        down_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{down}", parts[0].toStdWString());
        up_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{up}", parts[1].toStdWString());
    }
    else {
        down_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{down}", parts[0].toStdWString());
        hold_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{hold}", parts[1].toStdWString());
        up_command = std::make_unique<MacroCommand>(widget, manager, name_ + "{up}", parts[2].toStdWString());
    }
}

std::optional<Requirement> HoldableCommand::next_requirement(MainWidget* widget) {
    // holdable commands can't have requirements
    return {};
}

bool HoldableCommand::requires_document() {
    return true;
}

void HoldableCommand::perform() {
    down_command->run();
}

void HoldableCommand::perform_up() {
    up_command->run();
}

void HoldableCommand::on_key_hold() {
    if (hold_command) {
        hold_command->run();
    }
}

bool HoldableCommand::is_holdable() {
    return true;
}

void register_base_commands(CommandManager* manager) {
    register_command<NoopCommand>(manager);
}
