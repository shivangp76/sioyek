#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <qjsondocument.h>
#include <QKeyEvent>
#include <qstring.h>
#include <qstringlist.h>
#include <qlocalsocket.h>
#include <qfileinfo.h>
#include <qclipboard.h>
#include <qdesktopservices.h>
#include <qsettings.h>

#include "utils.h"
#include "input.h"
#include "main_widget.h"
#include "ui.h"
#include "document.h"
#include "document_view.h"
#include "network_manager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

extern std::vector<MainWidget*> windows;

extern Path local_database_file_path;
extern Path global_database_file_path;

extern bool SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE;
extern bool USE_LEGACY_KEYBINDS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, CustomCommandInfo> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, CustomCommandInfo> ADDITIONAL_MACROS;
extern std::wstring SEARCH_URLS[26];
extern bool NUMERIC_TAGS;
extern std::vector<AdditionalKeymapData> ADDITIONAL_KEYMAPS;
extern int TABLE_EXTRACT_BEHAVIOUR;

extern bool USE_KEYBOARD_POINT_SELECTION;
extern bool ADD_NEWLINES_WHEN_COPYING_TEXT;
extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;
extern float TTS_RATE;
extern float TTS_RATE_INCREMENT;

extern Path default_config_path;
extern Path default_keys_path;
extern std::vector<Path> user_config_paths;
extern std::vector<Path> user_keys_paths;
extern bool TOUCH_MODE;
extern bool VERBOSE;
extern float FREETEXT_BOOKMARK_COLOR[3];
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern bool FUZZY_SEARCHING;
extern bool TOC_JUMP_ALIGN_TOP;
extern bool ALIGN_LINK_DEST_TO_TOP;
extern bool FILL_TEXTBAR_WITH_SELECTED_TEXT;
extern bool SHOW_MOST_RECENT_COMMANDS_FIRST;
extern bool INCREMENTAL_SEARCH;
extern bool SHOW_SETCONFIG_IN_STATUSBAR;

extern std::wstring EXTRACT_TABLE_PROMPT;

extern std::wstring CONTEXT_MENU_ITEMS;
extern bool GG_USES_LABELS;

extern float SMOOTH_MOVE_MAX_VELOCITY;
bool is_command_string_modal(const std::wstring& command_name) {
    return std::find(command_name.begin(), command_name.end(), '[') != command_name.end();
}

std::vector<std::string> parse_command_name(const QString& command_names) {
    QStringList parts = command_names.split(';');
    std::vector<std::string> res;
    for (int i = 0; i < parts.size(); i++) {
        res.push_back(parts.at(i).toStdString());
    }
    return res;
}

struct CommandInvocation {
    QString command_name;
    QStringList command_args;

    QString command_string() {
        if (command_name.size() > 0) {
            if (command_name[0] != '[') return command_name;
            int index = command_name.indexOf(']');
            return command_name.mid(index + 1);
        }
        return "";
    }

    QString mode_string() {
        if (command_name.size() > 0) {
            if (command_name[0] == '[') {
                int index = command_name.indexOf(']');
                return command_name.mid(1, index - 1);
            }
            else {
                return "";
            }
        }
        return "";
    }
};

struct ParseState {
    QString str;
    int index = 0;

    void skip_whitespace() {
        while (index < str.size() && str[index].isSpace()) {
            index++;
        }
    }

    void skip_whitespace_and_commas() {

        while (index < str.size() && (str[index].isSpace() || str[index] == ',')) {
            index++;
        }
    }

    std::optional<QString> next_arg() {
        skip_whitespace();
        QString arg;

        if (str[index] == '\'') {
            index++;
            bool is_prev_char_backslash = false;

            while (index < str.size()) {
                auto ch = str[index];
                if (is_prev_char_backslash) {
                    if (ch == '\'') {
                        arg.push_back('\'');
                    }
                    if (ch == '\\') {
                        arg.push_back('\\');
                    }
                    if (ch == 'n') {
                        arg.push_back('\n');
                    }
                    is_prev_char_backslash = false;
                }
                else {
                    if (ch == '\\') {
                        is_prev_char_backslash = true;
                        index++;
                        continue;
                    }
                    if (ch == '\'') {
                        index++;
                        return arg;
                    }
                    else {
                        arg.push_back(ch);
                    }
                }
                index++;
            }
        }
        else {
            while ((str[index] != ')') && (str[index] != ',')) {
                arg.push_back(str[index]);
                index++;
            }
            if (!eof() && str[index] != ')') {
                index++;
            }
            return arg;
        }
        return {};
    }

    bool is_valid_command_name_char(QChar c) {
        return c.isLetterOrNumber() || c == '_' || c == '$' || c == '[' || c == ']';
    }

    std::optional<CommandInvocation>  next_command() {
        QString command_name;
        QStringList command_args;
        skip_whitespace();
        if (index >= str.size()) return {};

        while (index < str.size() && is_valid_command_name_char(str[index])) {
            command_name.push_back(str[index]);
            index++;
        }

        if (index == str.size() || (str[index] != '(')) {
            if (command_name.size() > 0) return CommandInvocation{ command_name , {} };
            return {};
        }


        index++; // skip the (
        while (index < str.size() && str[index] != ')') {
            std::optional<QString> maybe_arg = next_arg();
            if (maybe_arg.has_value()) {
                command_args.push_back(maybe_arg.value());
            }
            else {
                break;
            }
            skip_whitespace_and_commas();
        }
        if (!eof() && str[index] == ')') {
            index++;
        }
        return CommandInvocation{ command_name, command_args };

    }

    bool eof() {
        return index >= str.size();
    }

    QChar cc() {
        return str[index];
    }

    std::vector<CommandInvocation> parse_next_macro_command() {
        std::vector<CommandInvocation> res;

        while (auto cmd = next_command()) {
            res.push_back(cmd.value());
            skip_whitespace();
            if (eof()) break;
            if (cc() == ';') {
                index++;
            }
        }

        return res;
    }


};

class NoopCommand : public Command {
public:
    static inline const std::string cname = "noop";
    static inline const std::string hname = "Do nothing";

    NoopCommand(MainWidget* widget_) : Command(cname, widget_) {}

    void perform() {
    }

    bool requires_document() { return false; }
};

class LazyCommand : public Command {
private:
    CommandManager* command_manager;
    std::string command_name;
    //std::wstring command_params;
    std::vector<std::wstring> command_params;
    std::unique_ptr<Command> actual_command = nullptr;
    NoopCommand noop;

    //void parse_command_text(std::wstring command_text) {
    //    int index = command_text.find(L"(");
    //    if (index != -1) {
    //        command_name = utf8_encode(command_text.substr(0, index));
    //        command_params = command_text.substr(index + 1, command_text.size() - index - 2);
    //    }
    //    else {
    //        command_name = utf8_encode(command_text);
    //    }
    //}

    std::optional<std::wstring> get_result() override {
        if (actual_command) {
            return actual_command->get_result();
        }
        return {};
    }

    void set_result_socket(QLocalSocket* socket) {
        noop.set_result_socket(socket);
        result_socket = socket;
        if (actual_command) {
            actual_command->set_result_socket(socket);
        }
    }

    void set_result_mutex(bool* p_is_done, std::wstring* result_location) {
        result_holder = result_location;
        is_done = p_is_done;
        if (actual_command) {
            actual_command->set_result_mutex(p_is_done, result_location);
        }
    }

    Command* get_command() {
        if (actual_command) {
            return actual_command.get();
        }
        else {
            actual_command = std::move(command_manager->get_command_with_name(widget, command_name));
            if (!actual_command) return &noop;

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


public:
    LazyCommand(std::string name, MainWidget* widget_, CommandManager* manager, CommandInvocation invocation) : Command(name, widget_), noop(widget_) {
        command_manager = manager;
        command_name = invocation.command_name.toStdString();
        for (auto arg : invocation.command_args) {
            command_params.push_back(arg.toStdWString());
        }

        //parse_command_text(command_text);
    }

    void set_text_requirement(std::wstring value) { get_command()->set_text_requirement(value); }
    void set_symbol_requirement(char value) { get_command()->set_symbol_requirement(value); }
    void set_file_requirement(std::wstring value) { get_command()->set_file_requirement(value); }
    void set_rect_requirement(AbsoluteRect value) { get_command()->set_rect_requirement(value); }
    void set_generic_requirement(QVariant value) { get_command()->set_generic_requirement(value); }
    void handle_generic_requirement() { get_command()->handle_generic_requirement(); }
    void set_point_requirement(AbsoluteDocumentPos value) { get_command()->set_point_requirement(value); }
    void set_num_repeats(int nr) { get_command()->set_num_repeats(nr); }
    std::vector<char> special_symbols() { return get_command()->special_symbols(); }
    void pre_perform() { get_command()->pre_perform(); }
    bool pushes_state() { return get_command()->pushes_state(); }
    bool requires_document() { return get_command()->requires_document(); }
    std::optional<Requirement> next_requirement(MainWidget* widget) {
        return get_command()->next_requirement(widget);
    }

    virtual void perform() {
        auto com = get_command();
        if (com) {
            com->run();
        }
    }

    std::string get_name() {
        auto com = get_command();
        if (com) {
            return com->get_name();
        }
        else {
            return "";
        }
    }


};

class MacroCommand : public Command {
    std::vector<std::unique_ptr<Command>> commands;
    std::vector<std::string> modes;
    //std::wstring commands;
    std::string name;
    std::wstring raw_commands;
    CommandManager* command_manager;
    bool is_modal = false;
    std::vector<bool> performed;
    std::vector<bool> pre_performed;

public:

    std::unique_ptr<Command> get_subcommand(CommandInvocation invocation) {
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

    void set_result_socket(QLocalSocket* rsocket) {
        result_socket = rsocket;
        for (auto& subcommand : commands) {
            subcommand->set_result_socket(result_socket);
        }
    }

    void set_result_mutex(bool* id, std::wstring* result_location) {
        is_done = id;
        result_holder = result_location;
        for (auto& subcommand : commands) {
            subcommand->set_result_mutex(id, result_location);
        }
    }


    void initialize_from_invocations(std::vector<CommandInvocation> command_invocations) {
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

    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::vector<CommandInvocation> command_invocations) : Command(name_, widget_) {
        command_manager = manager;
        name = name_;

        initialize_from_invocations(command_invocations);
    }

    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::vector<std::unique_ptr<Command>>&& cmds) : Command(name_, widget_) {
        command_manager = manager;
        name = name_;

        for (auto&& cmd : cmds) {
            commands.push_back(std::move(cmd));
            performed.push_back(false);
            pre_performed.push_back(false);
        }
    }

    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, QString command_name, QStringList args) : Command(name_, widget_) {
        command_manager = manager;
        name = name_;
        CommandInvocation invocation;
        invocation.command_name = command_name;
        invocation.command_args = args;
        initialize_from_invocations({ invocation });

    }

    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(name_, widget_) {
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


    std::optional<std::wstring> get_result() override {
        if (commands.size() > 0) {
            return commands.back()->get_result();
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
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

    bool is_menu_command() {
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

    void set_generic_requirement(QVariant value) {
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

    void handle_generic_requirement() {
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

    void set_symbol_requirement(char value) {
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

    bool requires_document() override{

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

    void set_file_requirement(std::wstring value) {
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


    int get_command_index_for_requirement_type(RequirementType reqtype) {
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

    void set_rect_requirement(AbsoluteRect value) {
        int index = get_command_index_for_requirement_type(RequirementType::Rect);
        if (index >= 0) {
            commands[index]->set_rect_requirement(value);
        }
    }

    void set_point_requirement(AbsoluteDocumentPos value) {
        int index = get_command_index_for_requirement_type(RequirementType::Point);
        if (index >= 0) {
            commands[index]->set_point_requirement(value);
        }
    }


    std::wstring get_text_default_value() {
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

    void pre_perform() {
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

    std::optional<Requirement> next_requirement(MainWidget* widget) {
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

    void perform_subcommand(int index) {
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


    bool is_enabled() {
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

    bool is_holdable() {
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

    void perform_up() {
        if (is_holdable()) {
            int mode_index = get_current_mode_index();
            if (mode_index >= 0) {
                commands[mode_index]->perform_up();
            }
        }
    }

    void on_key_hold() {
        if (is_holdable()) {
            int mode_index = get_current_mode_index();
            if (mode_index >= 0) {
                commands[mode_index]->on_key_hold();
            }
        }
    }

    void perform() {
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


    int get_current_mode_index() {
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

    int get_current_executing_command_index() {
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


    void on_text_change(const QString& new_text) {
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

    std::vector<char> special_symbols() {
        int command_index = get_current_executing_command_index();
        if (command_index != -1) {
            return commands[command_index]->special_symbols();
        }
        return {};
    }

    bool mode_matches(std::string current_mode, std::string command_mode) {
        for (auto c : command_mode) {
            if (current_mode.find(c) == std::string::npos) {
                return false;
            }
        }
        return true;
    }

    std::string get_name() {
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

    std::string get_human_readable_name() override {
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

};
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
        if (req.type == RequirementType::Text || req.type == RequirementType::Password) {
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

class GenericPathCommand : public Command {
public:
    std::optional<std::wstring> selected_path = {};
    GenericPathCommand(std::string name, MainWidget* w) : Command(name, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (selected_path) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "File path" };
        }
    }

    void set_generic_requirement(QVariant value) {
        selected_path = value.toString().toStdWString();
    }

};

class GenericGotoLocationCommand : public Command {
public:
    GenericGotoLocationCommand(std::string name, MainWidget* w) : Command(name, w) {};

    std::optional<float> target_location = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (target_location.has_value()) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "Target Location" };
        }
    }

    void set_generic_requirement(QVariant value) {
        target_location = value.toFloat();
    }

    void perform() {
        widget->main_document_view->set_offset_y(target_location.value());
        widget->validate_render();
    }

    bool pushes_state() { return true; }

};

class GenericPathAndLocationCommadn : public Command {
public:

    std::optional<QVariant> target_location;
    bool is_hash = false;
    GenericPathAndLocationCommadn(std::string name, MainWidget* w, bool is_hash_ = false) : Command(name, w) { is_hash = is_hash_; };

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (target_location) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "Location" };
        }
    }

    void set_generic_requirement(QVariant value) {
        target_location = value;
    }

    void perform() {
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

    bool pushes_state() {
        return true;
    }
};

class SymbolCommand : public Command {
public:
    char symbol = 0;
    SymbolCommand(std::string name, MainWidget* w) : Command(name, w) {}
    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (symbol == 0) {
            return Requirement{ RequirementType::Symbol, "symbol" };
        }
        else {
            return {};
        }
    }

    virtual void set_symbol_requirement(char value) {
        this->symbol = value;
    }

    virtual std::string get_human_readable_name() {
        auto res = Command::get_human_readable_name();
        if (symbol != 0) {
            res += "(";
            res.push_back(symbol);
            res += ")";
        }
        return res;
    }

};

class TextCommand : public Command {
protected:
    std::optional<std::wstring> text = {};
public:

    TextCommand(std::string name, MainWidget* w) : Command(name, w) {}

    virtual std::string text_requirement_name() {
        return "text";
    }

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (text.has_value()) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Text, text_requirement_name() };
        }
    }

    virtual void set_text_requirement(std::wstring value) {
        this->text = value;
    }

    std::wstring get_text_default_value() {
        if (FILL_TEXTBAR_WITH_SELECTED_TEXT) {
            return widget->dv()->get_selected_text();
        }
        return L"";
    }
};

class GotoMark : public SymbolCommand {
public:
    static inline const std::string cname = "goto_mark";
    static inline const std::string hname = "Go to marked location";

    GotoMark(MainWidget* w) : SymbolCommand(cname, w) {}

    bool pushes_state() {
        return true;
    }

    std::string get_name() {
        return cname;
    }

    std::vector<char> special_symbols() {
        std::vector<char> res = { '`', '\'', '/' };
        return res;
    }

    void perform() {
        assert(this->symbol != 0);
        widget->goto_mark(this->symbol);
    }

    bool requires_document() { return false; }
};

class SendSymbolCommand : public SymbolCommand {
public:
    static inline const std::string cname = "send_symbol";
    static inline const std::string hname = "Send a symbol to the previous running command";

    SendSymbolCommand(MainWidget* w) : SymbolCommand(cname, w) {}

    std::string get_name() {
        return cname;
    }

    void perform() {
        widget->send_symbol_to_last_command(this->symbol);
    }

    bool requires_document() { return false; }
};

class SetMark : public SymbolCommand {
public:
    static inline const std::string cname = "set_mark";
    static inline const std::string hname = "Set mark in current location";

    SetMark(MainWidget* w) : SymbolCommand(cname, w) {}

    std::string get_name() {
        return cname;
    }


    void perform() {
        assert(this->symbol != 0);
        widget->set_mark_in_current_location(this->symbol);
    }
};

class ToggleDrawingMask : public SymbolCommand {
public:
    static inline const std::string cname = "toggle_drawing_mask";
    static inline const std::string hname = "Toggle drawing type visibility";

    ToggleDrawingMask(MainWidget* w) : SymbolCommand(cname, w) {}

    std::string get_name() {
        return cname;
    }

    void perform() {
        widget->handle_toggle_drawing_mask(this->symbol);
    }
};

class GotoLoadedDocumentCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "goto_tab";
    static inline const std::string hname = "Open tab";
    GotoLoadedDocumentCommand(MainWidget* w) : GenericPathCommand(cname, w) {}

    void handle_generic_requirement() {
        widget->handle_goto_loaded_document();
    }

    void perform() {
        // if there is a window with that tab, raise the window
        for (auto window : windows) {
            if (window->doc()) {
                if (window->doc()->get_path() == selected_path.value()) {
                    window->raise();
                    window->activateWindow();
                    return;
                }
            }
        }

        widget->push_state();
        widget->open_document(selected_path.value());
    }

    bool requires_document() {
        return false;
    }

    std::string get_name() {
        return cname;
    }

};

class NextItemCommand : public Command {
public:
    static inline const std::string cname = "next_item";
    static inline const std::string hname = "Go to next search result";
    NextItemCommand(MainWidget* w) : Command(cname, w) {}

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(num_repeats);
    }

    std::string get_name() {
        return cname;
    }

};

class PrevItemCommand : public Command {
public:
    static inline const std::string cname = "previous_item";
    static inline const std::string hname = "Go to previous search result";
    PrevItemCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(-num_repeats);
    }

};

class ToggleTextMarkCommand : public Command {
public:
    static inline const std::string cname = "toggle_text_mark";
    static inline const std::string hname = "Move text cursor to other end of selection";
    ToggleTextMarkCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (widget->main_document_view->line_select_mode) {
            widget->main_document_view->swap_line_select_cursor();
        }
        else {
            widget->handle_toggle_text_mark();
        }
    }

};

class MoveTextMarkForwardCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_forward";
    static inline const std::string hname = "Move text cursor forward";
    MoveTextMarkForwardCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        //if (num_repeats == 0) num_repeats++;
        dv()->handle_move_text_mark_forward(false);
        //widget->invalidate_render();
    }
};

class MoveTextMarkDownCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_down";
    static inline const std::string hname = "Move text cursor down";
    MoveTextMarkDownCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->handle_move_text_mark_down();
    }
};

class MoveTextMarkUpCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_up";
    static inline const std::string hname = "Move text cursor up";
    MoveTextMarkUpCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->handle_move_text_mark_up();
    }
};

class MoveTextMarkForwardWordCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_forward_word";
    static inline const std::string hname = "Move text cursor forward to the next word";
    MoveTextMarkForwardWordCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->handle_move_text_mark_forward(true);
    }
};

class MoveTextMarkBackwardCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_backward";
    static inline const std::string hname = "Move text cursor backward";
    MoveTextMarkBackwardCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->handle_move_text_mark_backward(false);
    }
};

class MoveTextMarkBackwardWordCommand : public Command {
public:
    static inline const std::string cname = "move_text_mark_backward_word";
    static inline const std::string hname = "Move text cursor backward to the previous word";
    MoveTextMarkBackwardWordCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->handle_move_text_mark_backward(true);
    }
};


class IncreaseTtsRateCommand : public Command {
public:
    static inline const std::string cname = "increase_tts_rate";
    static inline const std::string hname = "Increase the tts speed.";
    IncreaseTtsRateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        float new_rate = TTS_RATE + TTS_RATE_INCREMENT;
        if (new_rate > 1) new_rate = 1;
        TTS_RATE = new_rate;
        if (widget->is_reading) {
            widget->handle_stop_reading();
            widget->handle_start_reading();
        }
    }
};

class DecreaseTtsRateCommand : public Command {
public:
    static inline const std::string cname = "decrease_tts_rate";
    static inline const std::string hname = "Decrease the tts speed.";
    DecreaseTtsRateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        float new_rate = TTS_RATE - TTS_RATE_INCREMENT;
        if (new_rate < -1) new_rate = -1;
        TTS_RATE = new_rate;
        if (widget->is_reading) {
            widget->handle_stop_reading();
            widget->handle_start_reading();
        }
    }
};

class StartReadingHighQualityCommand : public Command {
public:
    static inline const std::string cname = "start_reading_high_quality";
    static inline const std::string hname = "Read using sioyek servers' text to speech";
    StartReadingHighQualityCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_start_reading_high_quality(true);
    }
};

class StopReadingCommand : public Command {
public:
    static inline const std::string cname = "stop_reading";
    static inline const std::string hname = "Stop reading";
    StopReadingCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_stop_reading();
    }
};

class ToggleReadingCommand : public Command {
public:
    static inline const std::string cname = "toggle_reading";
    static inline const std::string hname = "Start/Pause reading the selected line";
    ToggleReadingCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_toggle_reading();
    }
};

class AddKeybindCommand : public TextCommand {
public:
    static inline const std::string cname = "add_keybind";
    static inline const std::string hname = "Add Keybinding";
    AddKeybindCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        QString cmd_string = QString::fromStdWString(text.value());
        int last_space_index = cmd_string.lastIndexOf(' ');
        std::wstring command_string = cmd_string.left(last_space_index).toStdWString();
        std::wstring keybind_string = cmd_string.right(cmd_string.size() - last_space_index - 1).toStdWString();
        qDebug() << command_string << " | " << keybind_string;
        widget->input_handler->add_keybind(keybind_string, command_string, L"[no file]", -1);
        //widget->input_handler->add_keybind()
    }

    std::string text_requirement_name() {
        return "Keybinding";
    }

};

class SearchCommand : public TextCommand {
public:
    static inline const std::string cname = "search";
    static inline const std::string hname = "Search";
    SearchCommand(MainWidget* w, std::string override_cname = "") : TextCommand(override_cname.size() > 0 ? override_cname : cname, w) {
    };

    void on_text_change(const QString& new_text) override {
        if (INCREMENTAL_SEARCH && widget->doc()->is_super_fast_index_ready()) {
            widget->perform_search(new_text.toStdWString(), false, true);
        }
    }

    void pre_perform() {
        if (INCREMENTAL_SEARCH) {
            widget->main_document_view->add_mark('/');
        }
    }

    virtual void on_cancel() override {
        if (INCREMENTAL_SEARCH) {
            widget->goto_mark('/');
        }
    }

    virtual void perform() {
        // this search is not incremental even if incremental search is activated
        // (for example it should update the search terms list)
        widget->perform_search(this->text.value(), false, false);
        if (TOUCH_MODE) {
            widget->show_search_buttons();
        }
    }

    bool pushes_state() {
        return true;
    }

    std::optional<std::wstring> get_text_suggestion(int index) {
        return widget->get_search_suggestion_with_index(index);
    }

    std::string text_requirement_name() {
        return "Search Term";
    }

};

class FuzzySearchCommand : public SearchCommand {
public:
    static inline const std::string cname = "fuzzy_search";
    static inline const std::string hname = "Fuzzy Search";

    FuzzySearchCommand(MainWidget* w) : SearchCommand(w, cname) {
    };

    virtual void perform() {
        dv()->perform_fuzzy_search(text.value());
    }

};

class TypeTextCommand : public TextCommand {
public:
    static inline const std::string cname = "type_text";
    static inline const std::string hname = "Type Text";
    static inline const bool developer_only = true;

    TypeTextCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        widget->handle_type_text_into_input(QString::fromStdWString(text.value()));
    }

    std::string text_requirement_name() {
        return "Text";
    }

};


class DownloadClipboardUrlCommand : public Command {
public:
    static inline const std::string cname = "download_clipboard_url";
    static inline const std::string hname = "Download clipboard URL";

    DownloadClipboardUrlCommand(MainWidget* w) : Command(cname, w) {
    };

    void perform() {
        auto clipboard = QGuiApplication::clipboard();
        std::wstring url = clipboard->text().toStdWString();
        widget->download_paper_with_url(url, false, PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string text_requirement_name() {
        return "Paper Url";
    }

};
class DownloadPaperWithUrlCommand : public TextCommand {
public:
    static inline const std::string cname = "download_paper_with_url";
    static inline const std::string hname = "";

    DownloadPaperWithUrlCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        widget->download_paper_with_url(text.value(), false, PaperDownloadFinishedAction::OpenInNewWindow);
    }

    std::string text_requirement_name() {
        return "Paper Url";
    }

};

class SemanticSearchCommand : public TextCommand {
public:
    static inline const std::string cname = "semantic_search";
    static inline const std::string hname = "";

    SemanticSearchCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        widget->handle_semantic_search(text.value());
    }

    std::string text_requirement_name() {
        return "Search text";
    }

};

class SemanticSearchExtractiveCommand : public TextCommand {
public:
    static inline const std::string cname = "semantic_search_extractive";
    static inline const std::string hname = "";

    SemanticSearchExtractiveCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        widget->handle_semantic_search_extractive(text.value());
    }

    std::string text_requirement_name() {
        return "Search text";
    }

};

class ControlMenuCommand : public TextCommand {
public:
    static inline const std::string cname = "control_menu";
    static inline const std::string hname = "";
    ControlMenuCommand(MainWidget* w) : TextCommand(cname, w) {
    };


    void perform() {
        QString res = widget->handle_action_in_menu(text.value());
        result = res.toStdWString();
    }

    std::string text_requirement_name() {
        return "Action";
    }

    bool requires_document() {
        return false;
    }

};

class ExecuteMacroCommand : public TextCommand {
public:
    static inline const std::string cname = "execute_macro";
    static inline const std::string hname = "";
    ExecuteMacroCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        widget->execute_macro_if_enabled(text.value());
    }

    std::string text_requirement_name() {
        return "Macro";
    }

};

class SetViewStateCommand : public TextCommand {
public:
    static inline const std::string cname = "set_view_state";
    static inline const std::string hname = "";
    SetViewStateCommand(MainWidget* w) : TextCommand(cname, w) {
    };

    void perform() {
        QStringList parts = QString::fromStdWString(text.value()).split(' ');
        if (parts.size() == 4) {
            float offset_x = parts[0].toFloat();
            float offset_y = parts[1].toFloat();
            float zoom_level = parts[2].toFloat();
            bool pushes_state = parts[3].toInt();

            if (pushes_state == 1) {
                widget->push_state();
            }

            widget->main_document_view->set_offsets(offset_x, offset_y);
            widget->main_document_view->set_zoom_level(zoom_level, true);
        }
    }


    std::string text_requirement_name() {
        return "State String";
    }

};

class LlmCommand : public Command {
private:
    std::optional<std::wstring> system_prompt = {};
    std::optional<std::wstring> user_prompt = {};
    std::optional<AbsoluteRect> selected_rect = {};
public:
    static inline const std::string cname = "llm_command";
    static inline const std::string hname = "";
    LlmCommand(MainWidget* w) : Command(cname, w) {};

    void set_text_requirement(std::wstring value) {
        if (!system_prompt.has_value()) {
            system_prompt = value;
        }
        else {
            user_prompt = value;
        }
    }

    bool requires_image() {
        if (selected_rect.has_value()) {
            return false;
        }

        if (system_prompt.has_value() && user_prompt.has_value()) {
            return  (system_prompt->find(L"%{selected_image}") != -1) || (user_prompt->find(L"%{selected_image}") != -1);
        }
        return false;
    }


    void set_rect_requirement(AbsoluteRect value) {
        selected_rect = value;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!system_prompt.has_value()) {
            return Requirement{ RequirementType::Text, "System Prompt" };
        }
        if (!user_prompt.has_value()) {
            return Requirement{ RequirementType::Text, "User Prompt" };
        }
        if (requires_image() && (!selected_rect.has_value())) {
            return Requirement{ RequirementType::Rect, "Selected Image" };
        }
        return {};
    }

    QString process_prompt(std::wstring prompt) {
        QString result = QString::fromStdWString(prompt);

        if (result.indexOf("%{selected_text}") != -1) {
            result = result.replace("%{selected_text}", QString::fromStdWString(dv()->get_selected_text()));
        }

        if (result.indexOf("%{document_text}") != -1) {
            result = result.replace("%{document_text}", QString::fromStdWString(widget->doc()->get_super_fast_index()));
        }


        result.replace("%{selected_image}", "the provided image");

        return result;

    }

    void perform() {
        QString processed_user_prompt = process_prompt(user_prompt.value());
        QString processed_system_prompt = process_prompt(system_prompt.value());

        QPixmap pixmap;
        if (selected_rect) {
            WindowRect window_rect = selected_rect->to_window(widget->main_document_view);
            window_rect.y0 += 1;
            QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());

            float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
            pixmap = QPixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
            pixmap.setDevicePixelRatio(ratio);

            //widget->render(&pixmap, QPoint(), QRegion(widget->rect()));
            widget->render(&pixmap, QPoint(), QRegion(window_qrect));

            widget->dv()->clear_selected_rectangle();
            widget->set_rect_select_mode(false);
            widget->invalidate_render();
        }
        widget->sioyek_network_manager->perform_generic_llm_request(
            widget,
            processed_system_prompt,
            processed_user_prompt,
            selected_rect.has_value() ? &pixmap : nullptr, [selected_rect=selected_rect, w=widget](QString data) {
                if (selected_rect.has_value()) {
                    std::wstring desc = data.toStdWString();
                    w->doc()->add_freetext_bookmark(desc, selected_rect.value());
                    w->invalidate_render();
                }
                else {
                    copy_to_clipboard(data.toStdWString());
                }
            });
    }

};

class TestCommand : public Command {
private:
    std::optional<std::wstring> text1 = {};
    std::optional<std::wstring> text2 = {};
public:
    static inline const std::string cname = "test_command";
    static inline const std::string hname = "";
    TestCommand(MainWidget* w) : Command(cname, w) {};

    void set_text_requirement(std::wstring value) {
        if (!text1.has_value()) {
            text1 = value;
        }
        else {
            text2 = value;
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!text1.has_value()) {
            return Requirement{ RequirementType::Text, "text1" };
        }
        if (!text2.has_value()) {
            return Requirement{ RequirementType::Text, "text2" };
        }
        return {};
    }

    void perform() {
        result = text1.value() + text2.value();
        //widget->set_status_message(result.value());
        show_error_message(result.value());
    }

};

class GetConfigNoDialogCommand : public Command {
    std::optional<std::wstring> command_name = {};
public:
    static inline const std::string cname = "get_config_no_dialog";
    static inline const std::string hname = "";
    GetConfigNoDialogCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!command_name.has_value()) {
            return Requirement{ RequirementType::Text, "Prompt Title" };
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        command_name = value;
    }

    void perform() {
        //widget->config_manager->get_config
        QStringList config_name_list = QString::fromStdWString(command_name.value()).split(',');

        auto configs = widget->config_manager->get_configs_ptr();
        std::wstringstream output;
        for (auto confname : config_name_list) {

            for (int i = 0; i < configs->size(); i++) {
                if ((*configs)[i]->name == confname.toStdWString()) {
                    output << (*configs)[i]->get_type_string();
                    output << L" ";
                    (*configs)[i]->serialize((*configs)[i]->value, output);
                    if (i < configs->size() - 1) {
                        output.put(L'\n');
                    }
                }
            }
        }

        result = output.str();
    }

};

class ShowTextPromptCommand : public Command {

    std::optional<std::wstring> prompt_title = {};
    std::optional<std::wstring> default_value = {};
public:
    static inline const std::string cname = "show_text_prompt";
    static inline const std::string hname = "";
    ShowTextPromptCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!prompt_title.has_value()) {
            return Requirement{ RequirementType::Text, "Prompt Title" };
        }
        if (!result.has_value()) {
            return Requirement{ RequirementType::Text, utf8_encode(prompt_title.value()) };
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        if (!prompt_title.has_value()) {
            QString value_qstring = QString::fromStdWString(value);
            int index = value_qstring.indexOf('|');
            if (index != -1) {

                //QStringList parts = value_qstring.split("|");
                prompt_title = value_qstring.left(index).toStdWString();
                default_value = value_qstring.right(value_qstring.size() - prompt_title->size() - 1).toStdWString();
                //default_value = parts.at(1).toStdWString();
            }
            else {
                prompt_title = value;
            }
        }
        else {
            result = value;
        }
    }

    void pre_perform() {
        if (default_value) {
            widget->text_command_line_edit->setText(
                QString::fromStdWString(default_value.value())
            );
        }
    }

    void perform() {
    }

};


class GetModeString : public Command {

public:
    static inline const std::string cname = "get_mode_string";
    static inline const std::string hname = "";
    GetModeString(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = QString::fromStdString(widget->get_current_mode_string()).toStdWString();
    }
};

class GetStateJsonCommand : public Command {

public:
    static inline const std::string cname = "get_state_json";
    static inline const std::string hname = "";
    GetStateJsonCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QJsonDocument doc(widget->get_all_json_states());
        std::wstring json_str = utf8_decode(doc.toJson(QJsonDocument::Compact).toStdString());
        result = json_str;
    }
};

class GetPaperNameCommand : public Command {

public:
    static inline const std::string cname = "get_paper_name";
    static inline const std::string hname = "";
    GetPaperNameCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = widget->doc()->detect_paper_name();
    }
};

class GetOverviewPaperName : public Command {

public:
    static inline const std::string cname = "get_overview_paper_name";
    static inline const std::string hname = "";
    GetOverviewPaperName(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::optional<QString> paper_name = widget->main_document_view->get_overview_paper_name();
        result = paper_name.value_or("").toStdWString();
    }
};

class ToggleRectHintsCommand : public Command {

public:
    static inline const std::string cname = "toggle_rect_hints";
    static inline const std::string hname = "";
    ToggleRectHintsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->toggle_rect_hints();
    }

};

class GetAnnotationsJsonCommand : public Command {

public:
    static inline const std::string cname = "get_annotations_json";
    static inline const std::string hname = "";
    GetAnnotationsJsonCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QJsonDocument doc(widget->get_json_annotations());
        std::wstring json_str = utf8_decode(doc.toJson(QJsonDocument::Compact).toStdString());
        result = json_str;
    }

};

class ShowOptionsCommand : public Command {

private:
    std::vector<std::wstring> options;
    std::optional<std::wstring> selected_option;

public:
    static inline const std::string cname = "show_custom_options";
    static inline const std::string hname = "";
    ShowOptionsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (options.size() == 0) {
            return Requirement{ RequirementType::Text, "options" };
        }
        if (!selected_option.has_value()) {
            return Requirement{ RequirementType::Generic, "selected" };
        }
        return {};
    }

    void set_generic_requirement(QVariant value) {
        selected_option = value.toString().toStdWString();
        result = selected_option.value();
    }

    void handle_generic_requirement() {
        widget->show_custom_option_list(options);
    }

    void set_text_requirement(std::wstring value) {
        QStringList options_ = QString::fromStdWString(value).split("|");
        for (auto option : options_) {
            options.push_back(option.toStdWString());
        }
    }

    void perform() {
    }

};

//class ShowOptionsCommand : public TextCommand {
//public:
//    SearchCommand(MainWidget* w) : TextCommand(w) {};
//
//    void perform() {
//        widget->perform_search(this->text.value(), false);
//        if (TOUCH_MODE) {
//            widget->show_search_buttons();
//        }
//    }
//
//    std::string get_name() {
//        return "search";
//    }
//
//    bool pushes_state() {
//        return true;
//    }
//
//    std::string text_requirement_name() {
//        return "Search Term";
//    }
//
//};

class GetConfigCommand : public TextCommand {
public:
    static inline const std::string cname = "get_config_value";
    static inline const std::string hname = "";
    GetConfigCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        auto configs = widget->config_manager->get_configs_ptr();
        for (int i = 0; i < configs->size(); i++) {
            if ((*configs)[i]->name == text.value()) {
                std::wstringstream ssr;
                (*configs)[i]->serialize((*configs)[i]->value, ssr);
                show_error_message(ssr.str());
            }
        }
    }

    std::string text_requirement_name() {
        return "Config Name";
    }

};

class DownloadPaperWithNameCommand : public TextCommand {
public:
    static inline const std::string cname = "download_paper_with_name";
    static inline const std::string hname = "";
    DownloadPaperWithNameCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->download_paper_with_name(text.value(), PaperDownloadFinishedAction::OpenInNewWindow);

    }

    std::string text_requirement_name() {
        return "Paper Name";
    }

};

class RenameCommand : public TextCommand {
public:
    static inline const std::string cname = "rename";
    static inline const std::string hname = "Rename the current file";
    RenameCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        //widget->add_text_annotation_to_selected_highlight(this->text.value());
        widget->handle_rename(text.value());
    }

    void pre_perform() {
        if (!widget->doc()) return;

        QString file_name = get_file_name_from_paper_name(QString::fromStdWString(widget->doc()->detect_paper_name()));
        widget->text_command_line_edit->setText(
            file_name
        );
        //QString paper_name = QString::fromStdWString(widget->doc()->detect_paper_name());

    }

    std::string text_requirement_name() {
        return "New Name";
    }
};

class SetFreehandThickness : public TextCommand {
public:
    static inline const std::string cname = "set_freehand_thickness";
    static inline const std::string hname = "Set thickness of freehand drawings";
    SetFreehandThickness(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        float thickness = QString::fromStdWString(this->text.value()).toFloat();
        widget->set_freehand_thickness(thickness);
        //widget->perform_search(this->text.value(), false);
        //if (TOUCH_MODE) {
        //	widget->show_search_buttons();
        //}
    }


    std::string text_requirement_name() {
        return "Thickness";
    }
};

class GotoPageWithLabel : public TextCommand {
public:
    static inline const std::string cname = "goto_page_with_label";
    static inline const std::string hname = "Go to page with label";
    GotoPageWithLabel(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->goto_page_with_label(text.value());
    }

    bool pushes_state() {
        return true;
    }

    std::string text_requirement_name() {
        return "Page Label";
    }
};

class ChapterSearchCommand : public TextCommand {
public:
    static inline const std::string cname = "chapter_search";
    static inline const std::string hname = "Search current chapter";
    ChapterSearchCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->perform_search(this->text.value(), false);
    }

    void pre_perform() {
        std::optional<std::pair<int, int>> chapter_range = widget->main_document_view->get_current_page_range();
        if (chapter_range) {
            std::stringstream search_range_string;
            search_range_string << "<" << chapter_range.value().first << "," << chapter_range.value().second << ">";
            widget->text_command_line_edit->setText(search_range_string.str().c_str() + widget->text_command_line_edit->text());
        }

    }

    bool pushes_state() {
        return true;
    }

    std::string text_requirement_name() {
        return "Search Term";
    }
};

class RegexSearchCommand : public TextCommand {
public:
    static inline const std::string cname = "regex_search";
    static inline const std::string hname = "Search using regular expression";
    RegexSearchCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->perform_search(this->text.value(), true);
    }

    bool pushes_state() {
        return true;
    }

    std::string text_requirement_name() {
        return "regex";
    }
};

class AddBookmarkCommand : public TextCommand {
public:
    static inline const std::string cname = "add_bookmark";
    static inline const std::string hname = "Add an invisible bookmark in the current location";
    AddBookmarkCommand(MainWidget* w) : TextCommand(cname, w) {}

    void perform() {
        std::string uuid = widget->main_document_view->add_bookmark(text.value());
        widget->on_new_bookmark_added(uuid);
        result = utf8_decode(uuid);
    }


    std::string text_requirement_name() {
        return "Bookmark Text";
    }
};


class AddBookmarkMarkedCommand : public Command {

public:
    static inline const std::string cname = "add_marked_bookmark";
    static inline const std::string hname = "Add a bookmark in the selected location";

    std::optional<std::wstring> text_;
    std::optional<AbsoluteDocumentPos> point_;
    std::string pending_uuid = "";

    AddBookmarkMarkedCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!point_.has_value()) {
            Requirement req = { RequirementType::Point, "Bookmark Location" };
            return req;
        }
        if (!text_.has_value()) {
            Requirement req = { RequirementType::Text, "Bookmark Text" };
            return req;
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        text_ = value;
    }


    void set_point_requirement(AbsoluteDocumentPos value) {
        point_ = value;

        BookMark incomplete_bookmark;

        incomplete_bookmark.begin_x = value.x;
        incomplete_bookmark.begin_y = value.y;

        pending_uuid = widget->doc()->add_incomplete_bookmark(incomplete_bookmark);
        dv()->set_selected_bookmark_uuid(pending_uuid);
        widget->invalidate_render();
    }

    void on_cancel() override {
        if (pending_uuid.size() > 0) {
            widget->doc()->undo_pending_bookmark(pending_uuid);
        }
    }

    void perform() {
        if (text_->size() > 0) {
            std::string uuid = widget->doc()->add_pending_bookmark(pending_uuid, text_.value());
            widget->on_new_bookmark_added(uuid);
            widget->invalidate_render();
            result = utf8_decode(uuid);
            dv()->set_selected_bookmark_uuid("");
        }
        else {
            widget->doc()->undo_pending_bookmark(pending_uuid);
        }
    }
};


class CreateVisiblePortalCommand : public Command {

public:
    static inline const std::string cname = "create_visible_portal";
    static inline const std::string hname = "";

    std::optional<AbsoluteDocumentPos> point_;

    CreateVisiblePortalCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!point_.has_value()) {
            Requirement req = { RequirementType::Point, "Location" };
            return req;
        }
        return {};
    }

    void set_point_requirement(AbsoluteDocumentPos value) {
        point_ = value;
    }

    void perform() {
        widget->start_creating_rect_portal(point_.value());
    }

};

class PinOverviewAsPortalCommand : public Command {

public:
    static inline const std::string cname = "pin_overview_as_portal";
    static inline const std::string hname = "";

    std::optional<AbsoluteDocumentPos> point_;

    PinOverviewAsPortalCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->pin_current_overview_as_portal();
    }

};

class CopyDrawingsFromScratchpadCommand : public Command {

public:
    static inline const std::string cname = "copy_drawings_from_scratchpad";
    static inline const std::string hname = "";

    std::optional<AbsoluteRect> rect_;

    CopyDrawingsFromScratchpadCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Screenshot rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {
        AbsoluteDocumentPos scratchpad_pos = AbsoluteDocumentPos{ widget->scratchpad->get_offset_x(), widget->scratchpad->get_offset_y() };
        AbsoluteDocumentPos main_window_pos = widget->main_document_view->get_offsets();
        AbsoluteDocumentPos click_pos = rect_->center().to_window(widget->scratchpad).to_absolute(widget->main_document_view);

        //float diff_x = main_window_pos.x - scratchpad_pos.x;
        //float diff_y = main_window_pos.y - scratchpad_pos.y;

        auto indices = widget->scratchpad->get_intersecting_objects(rect_.value());
        std::vector<FreehandDrawing> drawings;
        std::vector<PixmapDrawing> pixmaps;

        widget->scratchpad->get_selected_objects_with_indices(indices, drawings, pixmaps);

        for (auto& drawing : drawings) {
            for (int i = 0; i < drawing.points.size(); i++) {
                drawing.points[i].pos = drawing.points[i].pos.to_window(widget->scratchpad).to_absolute(widget->main_document_view);
            }
        }

        FreehandDrawingMoveData md;
        md.initial_drawings = drawings;
        md.initial_pixmaps = {};
        md.initial_mouse_position = click_pos;
        dv()->freehand_drawing_move_data = md;
        widget->toggle_scratchpad_mode();
        widget->set_rect_select_mode(false);
        widget->invalidate_render();
    }

};

class CopyScreenshotToClipboard : public Command {

public:
    static inline const std::string cname = "copy_screenshot_to_clipboard";
    static inline const std::string hname = "";

    std::optional<AbsoluteRect> rect_;

    CopyScreenshotToClipboard(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Screenshot rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {
        widget->clear_selected_rect();

        WindowRect window_rect = rect_->to_window(widget->main_document_view);
        window_rect.y0 += 1;
        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());

        float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
        QPixmap pixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
        pixmap.setDevicePixelRatio(ratio);

        //widget->render(&pixmap, QPoint(), QRegion(widget->rect()));
        widget->render(&pixmap, QPoint(), QRegion(window_qrect));
        QApplication::clipboard()->setPixmap(pixmap);

        widget->set_rect_select_mode(false);
        widget->invalidate_render();
    }

};

class CopyScreenshotToScratchpad : public Command {

public:
    static inline const std::string cname = "copy_screenshot_to_scratchpad";
    static inline const std::string hname = "Copy window screenshot to scratchpad";

    std::optional<AbsoluteRect> rect_;

    CopyScreenshotToScratchpad(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Screenshot rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {
        widget->clear_selected_rect();

        WindowRect window_rect = rect_->to_window(widget->main_document_view);
        window_rect.y0 += 1;
        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());

        float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
        QPixmap pixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
        pixmap.setDevicePixelRatio(ratio);

        //widget->render(&pixmap, QPoint(), QRegion(widget->rect()));
        widget->render(&pixmap, QPoint(), QRegion(window_qrect));
        widget->toggle_scratchpad_mode();
        widget->add_pixmap_to_scratchpad(pixmap);
        widget->set_rect_select_mode(false);
        widget->invalidate_render();
    }
};


class ExtractTableWithPromptCommand : public Command {

public:
    static inline const std::string cname = "extract_table_with_prompt";
    static inline const std::string hname = "Extract the selected table's data";

    std::optional<AbsoluteRect> rect_;
    std::optional<QString> bookmark_type_;
    std::optional<QString> prompt_;

    ExtractTableWithPromptCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!bookmark_type_.has_value()) {
            Requirement req = { RequirementType::Text, "Boomark Type" };
            return req;
        }
        if (!prompt_.has_value()) {
            Requirement req = { RequirementType::Text, "prompt" };
            return req;
        }
        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "table rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void set_text_requirement(std::wstring value) {
        if (!bookmark_type_.has_value()) {
            bookmark_type_ = QString::fromStdWString(value);
        }
        else {
            prompt_ = QString::fromStdWString(value);
        }
    }

    void perform() {
        widget->clear_selected_rect();

        WindowRect window_rect = rect_->to_window(widget->main_document_view);
        window_rect.y0 += 1;
        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());

        float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
        QPixmap pixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
        pixmap.setDevicePixelRatio(ratio);

        //widget->render(&pixmap, QPoint(), QRegion(widget->rect()));
        widget->render(&pixmap, QPoint(), QRegion(window_qrect));

        widget->set_rect_select_mode(false);
        widget->invalidate_render();

        MainWidget* w = widget;
        AbsoluteRect r = rect_.value();
        widget->sioyek_network_manager->extract_table_data(widget, pixmap, [w, r, bookmark_type_ = bookmark_type_](QString data) {
            if (TABLE_EXTRACT_BEHAVIOUR == TableExtractBehaviour::Copy) {

                copy_to_clipboard(data.toStdWString());
                show_error_message(L"The result was copied to your clipboard");
            }
            else if (TABLE_EXTRACT_BEHAVIOUR == TableExtractBehaviour::Bookmark) {
                std::wstring desc = ("#" + bookmark_type_.value() + "\n" + data).toStdWString();
                w->doc()->add_freetext_bookmark(desc, r);
                w->invalidate_render();
            }
            }, prompt_.value());

    }
};


class ConvertToLatexCommand : public Command {

public:
    static inline const std::string cname = "convert_to_latex";
    static inline const std::string hname = "Convert the selected image into latex";

    std::optional<AbsoluteRect> rect_;

    ConvertToLatexCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "table rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {

        std::unique_ptr<Command> cmd = widget->command_manager->get_command_with_name(widget, "extract_table_with_prompt");
        cmd->set_text_requirement(L"latex");
        cmd->set_text_requirement(L"Convert the image into latex. Just reply with raw latex, do not include any extra text.");
        cmd->set_rect_requirement(rect_.value());
        widget->advance_command(std::move(cmd));
    }
};

class ExtractTableCommand : public Command {

public:
    static inline const std::string cname = "extract_table";
    static inline const std::string hname = "Extract the selected table's data";

    std::optional<AbsoluteRect> rect_;

    ExtractTableCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "table rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
    }

    void perform() {

        std::unique_ptr<Command> cmd = widget->command_manager->get_command_with_name(widget, "extract_table_with_prompt");
        cmd->set_text_requirement(L"markdown");
        cmd->set_text_requirement(EXTRACT_TABLE_PROMPT);
        cmd->set_rect_requirement(rect_.value());
        widget->advance_command(std::move(cmd));
    }
};


class AddBookmarkFreetextCommand : public Command {

public:
    static inline const std::string cname = "add_freetext_bookmark";
    static inline const std::string hname = "Add a text bookmark in the selected rectangle";

    std::optional<std::wstring> text_;
    std::optional<AbsoluteRect> rect_;
    std::string pending_uuid = "";

    AddBookmarkFreetextCommand(MainWidget* w) : Command(cname, w) {};

    void on_text_change(const QString& new_text) override {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_bookmark_uuid);

        if (bookmark) {
            bookmark->description = new_text.toStdWString();
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Bookmark Location" };
            return req;
        }
        if (!text_.has_value()) {
            Requirement req = { RequirementType::Text, "Bookmark Text" };
            return req;
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        text_ = value;
    }

    void set_rect_requirement(AbsoluteRect value) {
        rect_ = value;
        BookMark incomplete_bookmark;

        incomplete_bookmark.begin_x = value.x0;
        incomplete_bookmark.begin_y = value.y0;
        incomplete_bookmark.end_x = value.x1;
        incomplete_bookmark.end_y = value.y1;
        incomplete_bookmark.color[0] = FREETEXT_BOOKMARK_COLOR[0];
        incomplete_bookmark.color[1] = FREETEXT_BOOKMARK_COLOR[1];
        incomplete_bookmark.color[2] = FREETEXT_BOOKMARK_COLOR[2];

        pending_uuid = widget->doc()->add_incomplete_bookmark(incomplete_bookmark);
        dv()->set_selected_bookmark_uuid(pending_uuid);

        widget->clear_selected_rect();
        widget->validate_render();
    }


    void on_cancel() {

        if (pending_uuid.size() > 0) {
            widget->doc()->undo_pending_bookmark(pending_uuid);
        }
    }

    void perform() {
        //widget->doc()->add_freetext_bookmark(text_.value(), rect_.value());
        result = widget->handle_freetext_bookmark_perform(text_.value(), pending_uuid);
    }
};

class AddFreetextBookmarkAutoCommand : public Command {

public:
    static inline const std::string cname = "add_freetext_bookmark_auto";
    static inline const std::string hname = "Add a text bookmark in an automatically selected rectangle.";
    AddFreetextBookmarkAutoCommand(MainWidget* w) : Command(cname, w) {};
    std::string pending_uuid = "";
    std::optional<DocumentRect> rect;
    std::vector<DocumentRect> possible_targets;


    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (rect.has_value()) {
            return {};
        }
        return Requirement { RequirementType::Symbol, "Bookmark Location" };
    }

    void set_symbol_requirement(char value) override {
        std::string tag;
        tag.push_back(value);
        int index = get_index_from_tag(tag);
        if (index < possible_targets.size() && index >= 0) {
            rect = possible_targets[index];
        }

    }

    void pre_perform() override {
        auto largest_rects = widget->get_largest_empty_rects();
        for (auto r : largest_rects) {
            possible_targets.push_back(r.to_absolute(dv()).to_document(widget->doc()));
        }

        if (possible_targets.size() > 0) {
            dv()->set_highlight_words(possible_targets);
            dv()->set_should_highlight_words(true, true);
        }
    }

    void perform() {
        dv()->set_highlight_words({});
        dv()->set_should_highlight_words(false);

        std::unique_ptr<AddBookmarkFreetextCommand> add_freetext_bookmark_command = std::make_unique<AddBookmarkFreetextCommand>(widget);
        add_freetext_bookmark_command->set_rect_requirement(rect.value().to_absolute(widget->doc()));
        widget->handle_command_types(std::move(add_freetext_bookmark_command), 1);
    }

};

class GotoBookmarkCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "goto_bookmark";
    static inline const std::string hname = "Open the bookmark list of current document";
    GotoBookmarkCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark();
    }
};

class GotoPortalListCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "goto_portal_list";
    static inline const std::string hname = "Open the portal list of current document";
    GotoPortalListCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_portal_list();
    }
};

class GotoBookmarkGlobalCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "goto_bookmark_g";
    static inline const std::string hname = "Open the bookmark list of all documents";

    GotoBookmarkGlobalCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark_global();
    }
    bool requires_document() { return false; }
};

class IncreaseFreetextBookmarkFontSizeCommand : public Command {
public:
    static inline const std::string cname = "increase_freetext_font_size";
    static inline const std::string hname = "Increase freetext bookmark font size";
    IncreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        FREETEXT_BOOKMARK_FONT_SIZE *= 1.1f;
        if (FREETEXT_BOOKMARK_FONT_SIZE > 100) {
            FREETEXT_BOOKMARK_FONT_SIZE = 100;
        }
        widget->update_selected_bookmark_font_size();

    }
};

class DecreaseFreetextBookmarkFontSizeCommand : public Command {
public:
    static inline const std::string cname = "decrease_freetext_font_size";
    static inline const std::string hname = "Decrease freetext bookmark font size";
    DecreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        FREETEXT_BOOKMARK_FONT_SIZE /= 1.1f;
        if (FREETEXT_BOOKMARK_FONT_SIZE < 1) {
            FREETEXT_BOOKMARK_FONT_SIZE = 1;
        }
        widget->update_selected_bookmark_font_size();
    }
};


class GotoHighlightCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "goto_highlight";
    static inline const std::string hname = "Open the highlight list of the current document";
    GotoHighlightCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_highlight();
    }
};


class GotoHighlightGlobalCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "goto_highlight_g";
    static inline const std::string hname = "Open the highlight list of the all documents";

    GotoHighlightGlobalCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_highlight_global();
    }
    bool requires_document() { return false; }
};

class GotoTableOfContentsCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "goto_toc";
    static inline const std::string hname = "Open table of contents";
    GotoTableOfContentsCommand(MainWidget* w) : GenericPathCommand(cname, w) {};

    std::optional<QVariant>  target_location = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (target_location) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Generic, "Location" };
        }
    }

    void set_generic_requirement(QVariant value) {
        target_location = value;
    }


    void handle_generic_requirement() {
        //widget->handle_goto_loaded_document();
        widget->handle_goto_toc();
    }


    void perform() {
        QVariant val = target_location.value();

        if (val.canConvert<int>()) {
            int page = val.toInt();

            widget->main_document_view->goto_page(page);

            if (TOC_JUMP_ALIGN_TOP || ALIGN_LINK_DEST_TO_TOP) {
                widget->main_document_view->scroll_mid_to_top();
            }
        }
        else {
            QList<QVariant> location = val.toList();
            int page = location[0].toInt();
            float x_offset = location[1].toFloat();
            float y_offset = location[2].toFloat();

            if (std::isnan(y_offset)) {
                widget->main_document_view->goto_page(page);
            }
            else {
                widget->main_document_view->goto_offset_within_page(page, y_offset);
            }
            if (TOC_JUMP_ALIGN_TOP || ALIGN_LINK_DEST_TO_TOP) {
                widget->main_document_view->scroll_mid_to_top();
            }

        }
    }

    bool pushes_state() { return true; }
};

class PortalCommand : public Command {
public:
    static inline const std::string cname = "portal";
    static inline const std::string hname = "Start creating a portal";
    PortalCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_portal();
    }
};

class ToggleWindowConfigurationCommand : public Command {
public:
    static inline const std::string cname = "toggle_window_configuration";
    static inline const std::string hname = "Toggle between one window and two window configuration";
    ToggleWindowConfigurationCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_window_configuration();
    }

    bool requires_document() { return false; }
};

class NextStateCommand : public Command {
public:
    static inline const std::string cname = "next_state";
    static inline const std::string hname = "Go forward in history";
    NextStateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->next_state();
    }

    bool requires_document() { return false; }
};

class PrevStateCommand : public Command {
public:
    static inline const std::string cname = "prev_state";
    static inline const std::string hname = "Go backward in history";
    PrevStateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->prev_state();
    }

    bool requires_document() { return false; }
};

class AddHighlightCommand : public SymbolCommand {
public:
    static inline const std::string cname = "add_highlight";
    static inline const std::string hname = "Highlight selected text";
    AddHighlightCommand(MainWidget* w) : SymbolCommand(cname, w) {};

    void perform() {
        result = widget->handle_add_highlight(symbol);
    }

    std::vector<char> special_symbols() {
        std::vector<char> res = { '_', };
        return res;
    }
};

class CommandPaletteCommand : public Command {
public:
    static inline const std::string cname = "command_palette";
    static inline const std::string hname = "";
    CommandPaletteCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_command_palette();
    }

    bool requires_document() { return false; }
};

#ifdef Q_OS_WIN
bool CreateRegistryKey(HKEY hKeyRoot, LPCSTR subKey, LPCSTR valueName, LPCSTR value) {
    HKEY hKey;
    LONG lResult;
    DWORD dwDisposition;

    // Create the key
    lResult = RegCreateKeyExA(
        hKeyRoot,      // Root key
        subKey,        // Subkey
        0,             // Reserved
        NULL,          // Class
        REG_OPTION_NON_VOLATILE, // Options
        KEY_WRITE,     // Desired access
        NULL,          // Security attributes
        &hKey,         // Handle to newly created key
        &dwDisposition // Disposition value buffer
    );

    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Error creating registry key: " << lResult << std::endl;
        return false;
    }

    // Set the value if provided
    if (value != NULL) {
        lResult = RegSetValueExA(
            hKey,        // Handle to key
            valueName,   // Value name
            0,           // Reserved
            REG_SZ,      // Value type
            (const BYTE*)value, // Value data
            strlen(value) + 1  // Value size including null terminator
        );

        if (lResult != ERROR_SUCCESS) {
            std::cerr << "Error setting registry value: " << lResult << std::endl;
        }
    }

    // Close the key handle
    RegCloseKey(hKey);
    return true;
}
#endif

class RegisterUrlHandler : public Command {
public:
    static inline const std::string cname = "register_url_handler";
    static inline const std::string hname = "";
    RegisterUrlHandler(MainWidget* w) : Command(cname, w) {};


    void perform() {
        // register the application to handle sioyek:// urls from windows
        std::string path = QApplication::applicationFilePath().toStdString();
        std::replace(path.begin(), path.end(), '/', '\\');

#ifdef Q_OS_WIN
        CreateRegistryKey(HKEY_CLASSES_ROOT, "sioyek", "URL Protocol", "");
        CreateRegistryKey(HKEY_CLASSES_ROOT, "sioyek\\shell", NULL, NULL);

        // Create [HKEY_CLASSES_ROOT\duck\shell\open]
        CreateRegistryKey(HKEY_CLASSES_ROOT, "sioyek\\shell\\open", NULL, NULL);

        std::string val = path + " %1";
        // Create [HKEY_CLASSES_ROOT\duck\shell\open\command]
        CreateRegistryKey(HKEY_CLASSES_ROOT, "sioyek\\shell\\open\\command", NULL, val.c_str());
#endif

#ifdef Q_OS_LINUX
        QFile desktop_file = QFile(":/resources/sioyek.desktop");
        desktop_file.copy(QDir::homePath() + "/.local/share/applications/sioyek.desktop");
        std::string desktop_file_path = QDir::homePath().toStdString() + "/.local/share/applications/sioyek.desktop";

        system(("chmod +x " + desktop_file_path).c_str());
        system("xdg-mime default sioyek.desktop x-scheme-handler/sioyek");

#endif

    }

    bool requires_document() { return false; }
};

class UnregisterUrlHandler : public Command {
public:
    static inline const std::string cname = "unregister_url_handler";
    static inline const std::string hname = "";
    UnregisterUrlHandler(MainWidget* w) : Command(cname, w) {};


    void perform() {
        // unregister sioyek:// handler
#ifdef Q_OS_WIN
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "sioyek\\shell\\open\\command");
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "sioyek\\shell\\open");
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "sioyek\\shell");
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "sioyek");
#endif
#ifdef Q_OS_LINUX
        QFile desktop_file = QFile(QDir::homePath() + "/.local/share/applications/sioyek.desktop");
        desktop_file.remove();
#endif
    }

    bool requires_document() { return false; }
};


class CommandCommand : public Command {
public:
    static inline const std::string cname = "command";
    static inline const std::string hname = "";
    CommandCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QStringList command_names = widget->command_manager->get_all_command_names();
        if (!TOUCH_MODE) {

            widget->show_command_menu();
        }
        else {

            TouchCommandSelector* tcs = new TouchCommandSelector(true, command_names, widget);
            widget->set_current_widget(tcs);
        }

        widget->show_current_widget();

    }

    bool requires_document() { return false; }
};

class ScreenshotCommand : public Command {
public:
    static inline const std::string cname = "screenshot";
    static inline const std::string hname = "";
    ScreenshotCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::Text, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_text_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->screenshot(file_name);
    }
    bool requires_document() { return false; }
};

class FramebufferScreenshotCommand : public Command {
public:
    static inline const std::string cname = "framebuffer_screenshot";
    static inline const std::string hname = "";
    FramebufferScreenshotCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::Text, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_text_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->framebuffer_screenshot(file_name);
    }

    bool requires_document() { return false; }
};

class ExportDefaultConfigFile : public Command {
public:
    static inline const std::string cname = "export_default_config_file";
    static inline const std::string hname = "";
    ExportDefaultConfigFile(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->export_default_config_file(file_name);
    }

    bool requires_document() { return false; }
};

class ExportCommandNamesCommand : public Command {
public:
    static inline const std::string cname = "export_command_names";
    static inline const std::string hname = "";
    ExportCommandNamesCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        //widget->framebuffer_screenshot(file_name);
        widget->export_command_names(file_name);
    }

    bool requires_document() { return false; }
};



class ExportConfigNamesCommand : public Command {
public:
    static inline const std::string cname = "export_config_names";
    static inline const std::string hname = "";
    ExportConfigNamesCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File Path" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        //widget->framebuffer_screenshot(file_name);
        widget->export_config_names(file_name);
    }

    bool requires_document() { return false; }
};

class GenericWaitCommand : public Command {
public:
    GenericWaitCommand(std::string name, MainWidget* w) : Command(name, w) {};
    bool finished = false;
    QTimer* timer = nullptr;
    QMetaObject::Connection timer_connection;

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!finished) {
            return Requirement{ RequirementType::Generic, "dummy" };
        }
        else {
            return {};
        }
    }

    virtual ~GenericWaitCommand() {

        if (timer) {
            timer->stop();
            timer->deleteLater();
        }
    }

    void set_generic_requirement(QVariant value)
    {
        finished = true;
    }

    virtual bool is_ready() = 0;

    void handle_generic_requirement() override {
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

    void perform() {
    }

};

class WaitCommand : public GenericWaitCommand {
    std::optional<int> duration = {};
    QDateTime start_time;
public:
    static inline const std::string cname = "wait";
    static inline const std::string hname = "";
    WaitCommand(MainWidget* w) : GenericWaitCommand(cname, w) {
        start_time = QDateTime::currentDateTime();
    };

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (duration.has_value()) {
            return GenericWaitCommand::next_requirement(widget);
        }
        else {
            return Requirement{ RequirementType::Text, "Duration" };
        }
    }


    void set_text_requirement(std::wstring text) {
        duration = QString::fromStdWString(text).toInt();
    }

    bool is_ready() override {
        return start_time.msecsTo(QDateTime::currentDateTime()) > duration.value();
    }
};

class WaitForIndexingToFinishCommand : public GenericWaitCommand {
public:
    static inline const std::string cname = "wait_for_indexing_to_finish";
    static inline const std::string hname = "";
    WaitForIndexingToFinishCommand(MainWidget* w) : GenericWaitCommand(cname, w) {};

    void set_generic_requirement(QVariant value)
    {
        finished = true;
    }

    bool is_ready() override {
        return widget->is_index_ready();
    }
};

class WaitForRendersToFinishCommand : public GenericWaitCommand {
public:
    static inline const std::string cname = "wait_for_renders_to_finish";
    static inline const std::string hname = "";
    WaitForRendersToFinishCommand(MainWidget* w) : GenericWaitCommand(cname, w) {};

    bool is_ready() override {
        return widget->is_render_ready();
    }
};

class WaitForSearchToFinishCommand : public GenericWaitCommand {
public:
    static inline const std::string cname = "wait_for_search_to_finish";
    static inline const std::string hname = "";
    WaitForSearchToFinishCommand(MainWidget* w) : GenericWaitCommand(cname, w) {};

    bool is_ready() override {
        return widget->is_search_ready();
    }
};

class WaitForDownloadsToFinish : public GenericWaitCommand {
public:
    static inline const std::string cname = "wait_for_downloads_to_finish";
    static inline const std::string hname = "";
    WaitForDownloadsToFinish(MainWidget* w) : GenericWaitCommand(cname, w) {};

    bool is_ready() override {
        bool is_downloading = false;
        bool is_running = widget->is_network_manager_running(&is_downloading);
        return !(is_downloading || is_running);
    }
};

class OpenDocumentCommand : public Command {
public:
    static inline const std::string cname = "open_document";
    static inline const std::string hname = "Open a document using the native file explorer";
    OpenDocumentCommand(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    bool pushes_state() {
        return true;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.size() == 0) {
            return Requirement{ RequirementType::File, "File" };
        }
        else {
            return {};
        }
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->open_document(file_name);
    }

    bool requires_document() { return false; }
};

class OpenServerOnlyFile : public Command {
public:
    static inline const std::string cname = "open_server_only_file";
    static inline const std::string hname = "Search and open files located only in sioyek servers";
    OpenServerOnlyFile(MainWidget* w) : Command(cname, w) {};

    std::wstring file_name;

    bool pushes_state() {
        return true;
    }

    void perform() {
        widget->handle_open_server_only_file();
    }

    bool requires_document() { return false; }
};


class MoveSmoothCommand : public Command {
    bool was_held = false;
    float velocity_multiplier = 1.0f;
public:
    MoveSmoothCommand(std::string name, MainWidget* w, float velocity_mult=1.0f) : Command(name, w) {
        velocity_multiplier = velocity_mult;
    };

    virtual bool is_down() = 0;

    void perform() {
        if (widget->main_document_view->is_ruler_mode() || dv()->is_pinned_portal_selected()) {
            widget->move_visual_mark(is_down() ? 1 : -1);
        }
        else {
            widget->handle_move_smooth_hold(is_down());
            widget->set_fixed_velocity(is_down() ? -SMOOTH_MOVE_MAX_VELOCITY * velocity_multiplier : SMOOTH_MOVE_MAX_VELOCITY * velocity_multiplier);
        }
    }

    void perform_up() {
        widget->set_fixed_velocity(0);
    }

    bool is_holdable() {

        if (widget->main_document_view->is_ruler_mode() || dv()->is_pinned_portal_selected()) {
            return false;
        }
        else {
            return true;
        }
    }

    bool requires_document() {
        return true;
    }

    void on_key_hold() {
        was_held = true;
        widget->handle_move_smooth_hold(is_down());
        widget->validate_render();
    }
};

class MoveUpSmoothCommand : public MoveSmoothCommand {
public:
    static inline const std::string cname = "move_up_smooth";
    static inline const std::string hname = "";
    MoveUpSmoothCommand(MainWidget* w) : MoveSmoothCommand(cname, w) {};

    bool is_down() {
        return false;
    }
};

class MoveDownSmoothCommand : public MoveSmoothCommand {
public:
    static inline const std::string cname = "move_down_smooth";
    static inline const std::string hname = "";
    MoveDownSmoothCommand(MainWidget* w) : MoveSmoothCommand(cname, w) {};

    bool is_down() {
        return true;
    }
};

class ScreenUpSmoothCommand : public MoveSmoothCommand {
public:
    static inline const std::string cname = "screen_up_smooth";
    static inline const std::string hname = "Move screen up smoothly";
    ScreenUpSmoothCommand(MainWidget* w) : MoveSmoothCommand(cname, w, 3.0f) {};

    bool is_down() {
        return false;
    }
};

class ScreenDownSmoothCommand : public MoveSmoothCommand {
public:
    static inline const std::string cname = "screen_down_smooth";
    static inline const std::string hname = "Move screen down smoothly";
    ScreenDownSmoothCommand(MainWidget* w) : MoveSmoothCommand(cname, w, 3.0f) {};

    bool is_down() {
        return true;
    }
};

class ToggleTwoPageModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_two_page_mode";
    static inline const std::string hname = "Toggle two page mode";
    ToggleTwoPageModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_toggle_two_page_mode();
    }
};

class FitEpubToWindowCommand : public Command {
public:
    static inline const std::string cname = "fit_epub_to_window";
    static inline const std::string hname = "";
    FitEpubToWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {

        EPUB_WIDTH = widget->width() / widget->dv()->get_zoom_level();
        EPUB_HEIGHT = widget->height() / widget->dv()->get_zoom_level();
        widget->on_config_changed("epub_width");
    }
};

class MoveDownCommand : public Command {
public:
    static inline const std::string cname = "move_down";
    static inline const std::string hname = "Move down";
    MoveDownCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        dv()->handle_vertical_move(rp);
    }
};

class MoveUpCommand : public Command {
public:
    static inline const std::string cname = "move_up";
    static inline const std::string hname = "Move up";
    MoveUpCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        dv()->handle_vertical_move(-rp);
    }
};

class MoveLeftInOverviewCommand : public Command {
public:
    static inline const std::string cname = "move_left_in_overview";
    static inline const std::string hname = "Move left in the overview window";
    MoveLeftInOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        dv()->scroll_overview(0, 1);
    }
};

class MoveRightInOverviewCommand : public Command {
public:
    static inline const std::string cname = "move_right_in_overview";
    static inline const std::string hname = "Move right in the overview window";
    MoveRightInOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        dv()->scroll_overview(0, -1);
    }
};

class MoveLeftCommand : public Command {
public:
    static inline const std::string cname = "move_left";
    static inline const std::string hname = "Move left";
    MoveLeftCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        dv()->handle_horizontal_move(-rp);
    }
};

class MoveRightCommand : public Command {
public:
    static inline const std::string cname = "move_right";
    static inline const std::string hname = "Move right";
    MoveRightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        dv()->handle_horizontal_move(rp);
    }
};

class JavascriptCommand : public Command {
public:
    std::string command_name;
    std::wstring code;
    std::optional<std::wstring> entry_point = {};
    bool is_async;

    JavascriptCommand(std::string command_name, std::wstring code_, std::optional<std::wstring> entry_point_, bool is_async_, MainWidget* w) : Command(command_name, w), command_name(command_name) {
        code = code_;
        entry_point = entry_point_;
        is_async = is_async_;
    };

    void perform() {
        widget->run_javascript_command(code, entry_point, is_async);
    }

    std::string get_name() {
        return command_name;
    }

};

class JsCallCommand : public Command {
public:
    std::string command_name;
    std::wstring funcall;

    JsCallCommand(std::string command_name, std::wstring func_, MainWidget* widget) : Command(command_name, widget) {
        funcall = func_ + L"()";
    };

    void perform() {
        widget->run_javascript_command(funcall, {}, false);
    }

    std::string get_name() {
        return command_name;
    }

};

class SaveScratchpadCommand : public Command {
public:
    static inline const std::string cname = "save_scratchpad";
    static inline const std::string hname = "Save scratchpad file";
    SaveScratchpadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->save_scratchpad();
    }
};

class LoadScratchpadCommand : public Command {
public:
    static inline const std::string cname = "load_scratchpad";
    static inline const std::string hname = "Load scratchpad file";
    LoadScratchpadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->load_scratchpad();
    }
};

class ClearScratchpadCommand : public Command {
public:
    static inline const std::string cname = "clear_scratchpad";
    static inline const std::string hname = "Clear all scratchpad content";
    ClearScratchpadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->clear_scratchpad();
    }
};

class ZoomInCommand : public Command {
public:
    static inline const std::string cname = "zoom_in";
    static inline const std::string hname = "Zoom in";
    ZoomInCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (dv()->selected_freehand_drawings.has_value()) {
            dv()->zoom_selected_freehand_drawings(ZOOM_INC_FACTOR);
        }
        else if (dv()->is_pinned_portal_selected()) {
            dv()->zoom_pinned_portal(true);
        }
        else {
            widget->main_document_view->zoom_in();
            widget->main_document_view->last_smart_fit_page = {};
        }
    }
};

class ZoomOutOverviewCommand : public Command {
public:
    static inline const std::string cname = "zoom_out_overview";
    static inline const std::string hname = "Zoom out the overview window";
    ZoomOutOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        dv()->zoom_out_overview();
    }
};

class ZoomInOverviewCommand : public Command {
public:
    static inline const std::string cname = "zoom_in_overview";
    static inline const std::string hname = "Zoom in the overview window";
    ZoomInOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        dv()->zoom_in_overview();
    }
};

class FitToPageWidthCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_width";
    static inline const std::string hname = "Fit the page to screen width";
    FitToPageWidthCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->handle_fit_to_page_width(false);
    }
};

class FitToOverviewWidth : public Command {
public:
    static inline const std::string cname = "fit_to_overview_width";
    static inline const std::string hname = "Fit the overview page to overview.";
    FitToOverviewWidth(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->fit_overview_width();
    }
};

class FitToPageSmartCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_smart";
    static inline const std::string hname = "";
    FitToPageSmartCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height_and_width_smart();
    }

};

class FitToPageWidthSmartCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_width_smart";
    static inline const std::string hname = "Fit the page to screen width, ignoring white page margins";
    FitToPageWidthSmartCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        dv()->handle_fit_to_page_width(true);
    }
};

class FitToPageHeightCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_height";
    static inline const std::string hname = "Fit the page to screen height";
    FitToPageHeightCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height();
        widget->main_document_view->last_smart_fit_page = {};
    }
};

class FitToPageHeightSmartCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_height_smart";
    static inline const std::string hname = "Fit the page to screen height, ignoring white page margins";
    FitToPageHeightSmartCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->fit_to_page_height(true);
    }
};

class NextPageCommand : public Command {
public:
    static inline const std::string cname = "next_page";
    static inline const std::string hname = "Go to next page";
    NextPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->move_pages(std::max(1, num_repeats));
    }
};

class PreviousPageCommand : public Command {
public:
    static inline const std::string cname = "previous_page";
    static inline const std::string hname = "Go to previous page";
    PreviousPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->move_pages(std::min(-1, -num_repeats));
    }
};

class ZoomOutCommand : public Command {
public:
    static inline const std::string cname = "zoom_out";
    static inline const std::string hname = "Zoom out";
    ZoomOutCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (dv()->selected_freehand_drawings.has_value()) {
            dv()->zoom_selected_freehand_drawings(1.0f / ZOOM_INC_FACTOR);
        }
        else if (dv()->is_pinned_portal_selected()) {
            dv()->zoom_pinned_portal(false);
        }
        else {
            widget->main_document_view->zoom_out();
            widget->main_document_view->last_smart_fit_page = {};
        }
    }
};

class GotoDefinitionCommand : public Command {
public:
    static inline const std::string cname = "goto_definition";
    static inline const std::string hname = "Go to the reference in current highlighted line";
    GotoDefinitionCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->main_document_view->goto_definition()) {
            widget->main_document_view->exit_ruler_mode();
        }
    }

    bool pushes_state() {
        return true;
    }

};

class OverviewDefinitionCommand : public Command {
public:
    static inline const std::string cname = "overview_definition";
    static inline const std::string hname = "Open an overview to the reference in current highlighted line";
    OverviewDefinitionCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->overview_to_definition();
    }
};

class PortalToDefinitionCommand : public Command {
public:
    static inline const std::string cname = "portal_to_definition";
    static inline const std::string hname = "Create a portal to the definition in current highlighted line";
    PortalToDefinitionCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->portal_to_definition();
    }

};

class MoveRulerToNextBlockCommand : public Command {
public:
    static inline const std::string cname = "move_ruler_to_next_block";
    static inline const std::string hname = "Move the ruler to the start of next text block";
    MoveRulerToNextBlockCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_next_block();
    }
};

class MoveRulerToPrevBlockCommand : public Command {
public:
    static inline const std::string cname = "move_ruler_to_prev_block";
    static inline const std::string hname = "Move the ruler to the start of previous text block";
    MoveRulerToPrevBlockCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_prev_block();
    }
};

class MoveVisualMarkDownCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_down";
    static inline const std::string hname = "Move current highlighted line down";
    MoveVisualMarkDownCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->move_visual_mark(rp);
    }
};

class MoveVisualMarkUpCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_up";
    static inline const std::string hname = "Move current highlighted line up";
    MoveVisualMarkUpCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->move_visual_mark(-rp);
    }
};

class MoveVisualMarkNextCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_next";
    static inline const std::string hname = "Move the current highlighted line to the next unread text";
    MoveVisualMarkNextCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        dv()->move_visual_mark_next();
    }
};

class MoveVisualMarkPrevCommand : public Command {
public:
    static inline const std::string cname = "move_visual_mark_prev";
    static inline const std::string hname = "Move the current highlighted line to the previous";
    MoveVisualMarkPrevCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        dv()->move_visual_mark_prev();
    }

};


class GotoPageWithPageNumberCommand : public TextCommand {
public:
    static inline const std::string cname = "goto_page_with_page_number";
    static inline const std::string hname = "Go to page with page number";
    GotoPageWithPageNumberCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        std::wstring text_ = text.value();
        if (is_string_numeric(text_.c_str()) && text_.size() < 6) { // make sure the page number is valid
            int dest = std::stoi(text_.c_str()) - 1;
            widget->main_document_view->goto_page(dest + widget->main_document_view->get_page_offset());
        }
    }

    std::string text_requirement_name() {
        return "Page Number";
    }

    bool pushes_state() {
        return true;
    }
};

class ResizePendingBookmark : public TextCommand {
public:
    static inline const std::string cname = "resize_pending_bookmark";
    static inline const std::string hname = "Resize the current freetext bookmark that is being edited.";
    ResizePendingBookmark(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        std::wstring text_ = text.value();
        QString qtext = QString::fromStdWString(text_);
        QStringList parts = qtext.split(' ');
        if (parts.size() == 4) {
            float d_top = parts[0].toFloat();
            float d_bottom = parts[1].toFloat();
            float d_left = parts[2].toFloat();
            float d_right = parts[3].toFloat();
            std::string bookmark_uuid = dv()->get_selected_bookmark_uuid();
            if (bookmark_uuid.size() > 0) {
                auto bookmark = widget->doc()->get_bookmark_with_uuid(bookmark_uuid);
                if (bookmark && bookmark->is_freetext()) {
                    bookmark->begin_x += d_left;
                    bookmark->end_x += d_right;
                    bookmark->begin_y += d_top;
                    bookmark->end_y += d_bottom;
                }
            }
        }
    }

    std::string text_requirement_name() {
        return "Top Bottom Left Right";
    }

    bool pushes_state() {
        return true;
    }
};

class MoveSelectedBookmarkCommand : public Command {
public:
    static inline const std::string cname = "move_selected_bookmark";
    static inline const std::string hname = "Move selected bookmark";

    bool use_keyboard = false;

    MoveSelectedBookmarkCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        if (selected_bookmark_uuid.size() > 0) {

            AbsoluteDocumentPos mouse_abspos = widget->get_mouse_abspos();
            dv()->move_selected_bookmark_to_pos(mouse_abspos);
            dv()->begin_bookmark_move(selected_bookmark_uuid, mouse_abspos);
        }

    }

};

class EditSelectedBookmarkCommand : public TextCommand {
public:
    static inline const std::string cname = "edit_selected_bookmark";
    static inline const std::string hname = "Edit selected bookmark";
    std::wstring initial_text;
    float initial_font_size;
    std::string uuid = "";

    EditSelectedBookmarkCommand(MainWidget* w) : TextCommand(cname, w) {};

    void on_text_change(const QString& new_text) override {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        //widget->doc()->get_bookmarks()[selected_bookmark_index].description = new_text.toStdWString();
        BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_bookmark_uuid);
        if (bookmark) {
            bookmark->description = new_text.toStdWString();
        }
    }

    void pre_perform() {

        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_bookmark_uuid);
        if (bookmark) {
            initial_text = bookmark->description;
            initial_font_size = bookmark->font_size;
            uuid = selected_bookmark_uuid;

            if (TOUCH_MODE) {
                if (widget->current_widget_stack.size() > 0) {
                    TouchTextEdit* stack_top = dynamic_cast<TouchTextEdit*>(widget->current_widget_stack.back());
                    if (stack_top) {
                        stack_top->set_text(bookmark->description);
                    }
                }
            }
            else {
                widget->text_command_line_edit->setText(
                    QString::fromStdWString(bookmark->description)
                );
            }
        }
    }

    void on_cancel() {
        if (uuid.size() > 0) {
            BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(uuid);
            bookmark->description = initial_text;
            bookmark->font_size = initial_font_size;
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        if (selected_bookmark_uuid.size() == 0) return {};
        return TextCommand::next_requirement(widget);
    }

    void perform() {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        if (selected_bookmark_uuid.size() > 0) {
            std::wstring text_ = text.value();
            widget->change_selected_bookmark_text(text_);
            widget->invalidate_render();
        }
        else {
            show_error_message(L"No bookmark is selected");
        }
    }

    std::string text_requirement_name() {
        return "Bookmark Text";
    }

};

class EditSelectedHighlightCommand : public TextCommand {
public:
    static inline const std::string cname = "edit_selected_highlight";
    static inline const std::string hname = "Edit the text comment of current selected highlight";
    std::string uuid = "";

    EditSelectedHighlightCommand(MainWidget* w) : TextCommand(cname, w) {};

    void pre_perform() {
        uuid = dv()->get_selected_highlight_uuid();

        if (uuid.size() > 0) {
            Highlight* hl = widget->doc()->get_highlight_with_uuid(uuid);
            if (hl) {
                widget->set_text_prompt_text(
                    QString::fromStdWString(hl->text_annot));
            }
        }
        //widget->text_command_line_edit->setText(
        //    QString::fromStdWString(widget->doc()->get_highlights()[widget->selected_highlight_index].text_annot)
        //);
    }

    void perform() {
        if (uuid.size() > 0) {
            std::wstring text_ = text.value();
            widget->change_selected_highlight_text_annot(text_);
        }
    }

    std::string text_requirement_name() {
        return "Highlight Annotation";
    }

};

class DeletePortalCommand : public Command {
public:
    static inline const std::string cname = "delete_portal";
    static inline const std::string hname = "Delete the closest portal";
    DeletePortalCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        std::optional<Portal> deleted_portal = widget->main_document_view->delete_closest_portal();
        if (deleted_portal) {
            widget->on_portal_deleted(deleted_portal.value(), widget->doc()->get_checksum());
        }
        widget->validate_render();
    }
};

class DeleteBookmarkCommand : public Command {
public:
    static inline const std::string cname = "delete_bookmark";
    static inline const std::string hname = "Delete the closest bookmark";
    DeleteBookmarkCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        std::optional<BookMark> deleted_bookmark = widget->main_document_view->delete_closest_bookmark();
        if (deleted_bookmark) {
            widget->on_bookmark_deleted(deleted_bookmark.value(), widget->doc()->get_checksum());
        }
        widget->validate_render();
    }
};

class GenericVisibleSelectCommand : public Command {
protected:
    std::vector<std::string> visible_item_uuids;
    std::string tag;
    int n_required_tags = 0;
    bool already_pre_performed = false;

public:
    GenericVisibleSelectCommand(std::string name, MainWidget* w) : Command(name, w) {};

    virtual std::vector<std::string> get_visible_item_uuids() = 0;
    virtual std::string get_selected_item_uuid() = 0;
    virtual void handle_indices_pre_perform() = 0;

    void pre_perform() override {
        if (!already_pre_performed) {
            if (get_selected_item_uuid().size() == 0) {
                visible_item_uuids = get_visible_item_uuids();
                n_required_tags = get_num_tag_digits(visible_item_uuids.size());

                handle_indices_pre_perform();
                widget->invalidate_render();
                //widget->handle_highlight_tags_pre_perform(visible_item_indices);
            }
            already_pre_performed = true;
        }
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        // since pre_perform must be executed in order to determine the requirements, we manually run it here
        pre_perform();

        if (tag.size() < n_required_tags) {
            return Requirement{ RequirementType::Symbol, "tag" };
        }
        else {
            return {};
        }
    }


    virtual void set_symbol_requirement(char value) {
        tag.push_back(value);
    }

    virtual void perform_with_selected_index(std::optional<int> index) = 0;

    void perform() {
        bool should_clear_labels = false;
        if (tag.size() > 0) {
            int index = get_index_from_tag(tag);
            perform_with_selected_index(index);
            //if (index < visible_item_indices.size()) {
            //    widget->set_selected_highlight_index(visible_highlight_indices[index]);
            //}
            should_clear_labels = true;
        }
        else {
            perform_with_selected_index({});
        }

        if (should_clear_labels) {
            dv()->clear_keyboard_select_highlights();
        }
    }
};


class SelectVisibleItem : public GenericVisibleSelectCommand {

    std::vector<VisibleObjectIndex> visible_objects;
public:
    static inline const std::string cname = "generic_select";
    static inline const std::string hname = "Select visible items";
    SelectVisibleItem(MainWidget* w) : GenericVisibleSelectCommand(cname, w) {
        visible_objects = widget->dv()->get_generic_visible_item_indices();
    };

    std::string get_selected_item_uuid() override {
        return "";
        //return widget->selected_highlight_index;
    }

    //void pre_perform() {
    //    widget->clear_tag_prefix();
    //}

    std::vector<std::string> get_visible_item_uuids() override {
        std::vector<std::string> res;
        for (int i = 0; i < visible_objects.size(); i++) {
            res.push_back(visible_objects[i].uuid);
        }
        return res;
        //return widget->main_document_view->get_visible_highlight_indices();
    }

    void handle_indices_pre_perform() override {
        widget->clear_tag_prefix();
        dv()->handle_generic_tags_pre_perform(visible_objects);

    }

    void perform_with_selected_index(std::optional<int> index) override {
        if (index && index.value() < visible_objects.size()) {
            VisibleObjectIndex object_index = visible_objects[index.value()];
            if (object_index.object_type == VisibleObjectType::Highlight) {
                dv()->set_selected_highlight_uuid(object_index.uuid);
            }
            if (object_index.object_type == VisibleObjectType::Bookmark) {
                dv()->set_selected_bookmark_uuid(object_index.uuid);
            }
            if (object_index.object_type == VisibleObjectType::Portal) {
                dv()->set_selected_portal_uuid(object_index.uuid);
            }
            if (object_index.object_type == VisibleObjectType::PinnedPortal) {
                dv()->set_selected_portal_uuid(object_index.uuid, true);
            }

        }

    }

};

class DeteteVisibleItem : public GenericVisibleSelectCommand {

    std::vector<VisibleObjectIndex> visible_objects;
public:
    static inline const std::string cname = "generic_delete";
    static inline const std::string hname = "Delete visible items";
    DeteteVisibleItem(MainWidget* w) : GenericVisibleSelectCommand(cname, w) {
        visible_objects = widget->dv()->get_generic_visible_item_indices();
    };

    std::string get_selected_item_uuid() override {
        return "";
        //return widget->selected_highlight_index;
    }

    //void pre_perform() {
    //    widget->clear_tag_prefix();
    //}

    std::vector<std::string> get_visible_item_uuids() override {
        std::vector<std::string> res;
        for (int i = 0; i < visible_objects.size(); i++) {
            res.push_back(visible_objects[i].uuid);
        }
        return res;
        //return widget->main_document_view->get_visible_highlight_indices();
    }

    void handle_indices_pre_perform() override {
        widget->clear_tag_prefix();
        dv()->handle_generic_tags_pre_perform(visible_objects);

    }

    void perform_with_selected_index(std::optional<int> index) override {
        if (index && index.value() < visible_objects.size()) {
            VisibleObjectIndex object_index = visible_objects[index.value()];
            if (object_index.object_type == VisibleObjectType::Highlight) {
                dv()->set_selected_highlight_uuid(object_index.uuid);
                widget->handle_delete_selected_highlight();
            }
            if (object_index.object_type == VisibleObjectType::Bookmark) {
                dv()->set_selected_bookmark_uuid(object_index.uuid);
                widget->handle_delete_selected_bookmark();
            }
            if ((object_index.object_type == VisibleObjectType::Portal) || (object_index.object_type == VisibleObjectType::PinnedPortal)) {
                dv()->set_selected_portal_uuid(object_index.uuid);
                widget->handle_delete_selected_portal();
            }

        }

    }

};

class GenericHighlightCommand : public GenericVisibleSelectCommand {

public:
    GenericHighlightCommand(std::string name, MainWidget* w) : GenericVisibleSelectCommand(name, w) {};

    std::string get_selected_item_uuid() override {
        return dv()->get_selected_highlight_uuid();
    }

    std::vector<std::string> get_visible_item_uuids() override {
        return widget->main_document_view->get_visible_highlight_uuids();
    }

    void handle_indices_pre_perform() override {
        dv()->handle_highlight_tags_pre_perform(visible_item_uuids);

    }

    virtual void perform_with_highlight_selected() = 0;

    void perform_with_selected_index(std::optional<int> index) override {
        if (index) {
            if (index < visible_item_uuids.size()) {
                dv()->set_selected_highlight_uuid(visible_item_uuids[index.value()]);
            }
        }

        perform_with_highlight_selected();
    }

};

class GenericVisibleBookmarkCommand : public GenericVisibleSelectCommand {

public:
    GenericVisibleBookmarkCommand(std::string name, MainWidget* w) : GenericVisibleSelectCommand(name, w) {};

    std::string get_selected_item_uuid() override {
        return dv()->get_selected_bookmark_uuid();
    }

    std::vector<std::string> get_visible_item_uuids() override {
        return widget->main_document_view->get_visible_bookmark_uuids();
    }


    void handle_indices_pre_perform() override {
        dv()->handle_visible_bookmark_tags_pre_perform(visible_item_uuids);
    }

    virtual void perform_with_bookmark_selected() = 0;

    void perform_with_selected_index(std::optional<int> index) override {
        if (index) {
            if (index < visible_item_uuids.size()) {
                dv()->set_selected_bookmark_uuid(visible_item_uuids[index.value()]);
            }
        }

        perform_with_bookmark_selected();
    }

};

class DeleteVisibleBookmarkCommand : public GenericVisibleBookmarkCommand {

public:
    static inline const std::string cname = "delete_visible_bookmark";
    static inline const std::string hname = "Delete the selected bookmark";
    DeleteVisibleBookmarkCommand(MainWidget* w) : GenericVisibleBookmarkCommand(cname, w) {};

    void perform_with_bookmark_selected() override {
        widget->handle_delete_selected_bookmark();
    }
};

class EditVisibleBookmarkCommand : public GenericVisibleBookmarkCommand {

public:
    static inline const std::string cname = "edit_visible_bookmark";
    static inline const std::string hname = "";
    EditVisibleBookmarkCommand(MainWidget* w) : GenericVisibleBookmarkCommand(cname, w) {};

    void perform_with_bookmark_selected() override {
        widget->execute_macro_if_enabled(L"edit_selected_bookmark");
    }
};

class DeleteHighlightCommand : public GenericHighlightCommand {

public:
    static inline const std::string cname = "delete_highlight";
    static inline const std::string hname = "Delete the selected highlight";
    DeleteHighlightCommand(MainWidget* w) : GenericHighlightCommand(cname, w) {};

    void perform_with_highlight_selected() override {
        std::optional<Highlight> deleted_highlight = widget->handle_delete_selected_highlight();
        //if (deleted_highlight) {
        //    widget->push_deleted_object({ widget->doc()->get_checksum(), deleted_highlight.value() });
        //}
    }
};


class ChangeHighlightTypeCommand : public GenericHighlightCommand {

public:
    static inline const std::string cname = "change_highlight";
    static inline const std::string hname = "";
    ChangeHighlightTypeCommand(MainWidget* w) : GenericHighlightCommand(cname, w) {};

    void perform_with_highlight_selected() {
        widget->execute_macro_if_enabled(L"add_highlight");
    }

};

class AddAnnotationToHighlightCommand : public GenericHighlightCommand {

public:
    static inline const std::string cname = "add_annot_to_highlight";
    static inline const std::string hname = "";
    AddAnnotationToHighlightCommand(MainWidget* w) : GenericHighlightCommand(cname, w) {};

    void perform_with_highlight_selected() {
        widget->execute_macro_if_enabled(L"edit_selected_highlight");
    }

};

class GotoPortalCommand : public Command {
public:
    static inline const std::string cname = "goto_portal";
    static inline const std::string hname = "Goto closest portal destination";
    GotoPortalCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        std::optional<Portal> link = widget->main_document_view->find_closest_portal();
        if (link) {
            widget->open_document(link->dst);
        }
    }

    bool pushes_state() {
        return true;
    }
};

class EditPortalCommand : public Command {
public:
    static inline const std::string cname = "edit_portal";
    static inline const std::string hname = "Edit portal";
    EditPortalCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        std::optional<Portal> link = widget->main_document_view->find_closest_portal();
        if (link) {
            widget->portal_to_edit = link;
            widget->open_document(link->dst);
        }
    }

    bool pushes_state() {
        return true;
    }
};

class OpenPrevDocCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "open_prev_doc";
    static inline const std::string hname = "Open the list of previously opened documents";
    OpenPrevDocCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w, true) {};

    void handle_generic_requirement() {
        widget->handle_open_prev_doc();
    }

    bool requires_document() { return false; }
};

class OpenAllDocsCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "open_all_docs";
    static inline const std::string hname = "";
    OpenAllDocsCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w, true) {};

    void handle_generic_requirement() {
        widget->handle_open_all_docs();
    }

    bool requires_document() { return false; }
};


class OpenDocumentEmbeddedCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "open_document_embedded";
    static inline const std::string hname = "Open an embedded file explorer";
    OpenDocumentEmbeddedCommand(MainWidget* w) : GenericPathCommand(cname, w) {};

    void handle_generic_requirement() {

        widget->set_current_widget(new FileSelector(
            FUZZY_SEARCHING,
            [widget = widget, this](std::wstring doc_path) {
                set_generic_requirement(QString::fromStdWString(doc_path));
                widget->advance_command(std::move(widget->pending_command_instance));
            }, widget, ""));
        widget->show_current_widget();
    }


    void perform() {
        widget->validate_render();
        widget->open_document(selected_path.value());
    }

    bool pushes_state() {
        return true;
    }

    bool requires_document() { return false; }
};

class OpenDocumentEmbeddedFromCurrentPathCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "open_document_embedded_from_current_path";
    static inline const std::string hname = "Open an embedded file explorer, starting in the directory of current document";
    OpenDocumentEmbeddedFromCurrentPathCommand(MainWidget* w) : GenericPathCommand(cname, w) {};

    void handle_generic_requirement() {
        std::wstring last_file_name = widget->get_current_file_name().value_or(L"");

        widget->set_current_widget(new FileSelector(
            FUZZY_SEARCHING,
            [widget = widget, this](std::wstring doc_path) {
                set_generic_requirement(QString::fromStdWString(doc_path));
                widget->advance_command(std::move(widget->pending_command_instance));
            }, widget, QString::fromStdWString(last_file_name)));
        widget->show_current_widget();

    }

    void perform() {
        widget->validate_render();
        widget->open_document(selected_path.value());
    }

    bool pushes_state() {
        return true;
    }

    bool requires_document() { return false; }
};

class CopyCommand : public Command {
public:
    static inline const std::string cname = "copy";
    static inline const std::string hname = "Copy";
    CopyCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        auto selected_text = dv()->get_selected_text(ADD_NEWLINES_WHEN_COPYING_TEXT);
        if (selected_text.size() == 0) {
            if (widget->main_document_view->get_overview_page()) {
                std::optional<QString> overview_paper_name = widget->main_document_view->get_overview_paper_name();
                if (overview_paper_name) {
                    copy_to_clipboard(overview_paper_name->toStdWString());
                }
            }
            else if (dv()->selected_object_index.has_value()) {
                VisibleObjectIndex selected_object = dv()->selected_object_index.value();
                if (selected_object.object_type == VisibleObjectType::Bookmark) {
                    BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_object.uuid);
                    if (bookmark) {
                        copy_to_clipboard(bookmark->description);
                    }
                }

            }
        }
        else {
            copy_to_clipboard(selected_text);
        }
    }

};

class GotoBeginningCommand : public Command {
public:
    static inline const std::string cname = "goto_begining";
    static inline const std::string hname = "Go to the beginning of the document";
    GotoBeginningCommand(MainWidget* w) : Command(cname, w) {};
public:
    void perform() {
        if (num_repeats) {
            if (GG_USES_LABELS){
                widget->goto_page_with_label(QString::number(num_repeats).toStdWString());
            }
            else{
                widget->main_document_view->goto_page(num_repeats - 1 + widget->main_document_view->get_page_offset());
            }
        }
        else {
            float y_offset = dv()->get_view_height() / 2 / dv()->get_zoom_level();
            widget->main_document_view->set_offset_y(y_offset);
        }
    }

    bool pushes_state() {
        return true;
    }

};

class GotoEndCommand : public Command {
public:
    static inline const std::string cname = "goto_end";
    static inline const std::string hname = "Go to the end of the document";
    GotoEndCommand(MainWidget* w) : Command(cname, w) {};
public:
    void perform() {
        if (num_repeats == 0) {
            widget->main_document_view->goto_end();
        }
        else {
            widget->main_document_view->goto_page(num_repeats - 1);
        }
    }

    bool pushes_state() {
        return true;
    }
};

class OverviewRulerPortalCommand : public Command {
public:
    static inline const std::string cname = "overview_to_ruler_portal";
    static inline const std::string hname = "";
    OverviewRulerPortalCommand(MainWidget* w) : Command(cname, w) {};
public:
    void perform() {
        widget->handle_overview_to_ruler_portal();
    }
};

class GotoRulerPortalCommand : public Command {
public:
    static inline const std::string cname = "goto_ruler_portal";
    static inline const std::string hname = "";
    GotoRulerPortalCommand(MainWidget* w) : Command(cname, w) {};
    std::optional<char> mark = {};

public:

    void perform() {

        std::string mark_str = "";
        if (mark) {
            mark_str = std::string(1, mark.value());
        }

        widget->handle_goto_ruler_portal(mark_str);
        widget->set_should_highlight_words(false);
    }

    void on_cancel() override {
        widget->set_should_highlight_words(false);
    }

    void pre_perform() override {
        if (dv()->get_ruler_portals().size() > 1) {
            dv()->highlight_ruler_portals();
        }
    }

    void set_symbol_requirement(char value) override {
        mark = value;
    }

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (mark) return {};

        if (dv()->get_ruler_portals().size() > 1) {
            return Requirement{ RequirementType::Symbol, "Mark" };
        }

        return {};
    }

    bool pushes_state() override {
        return true;
    }

};

class PrintNonDefaultConfigs : public Command {
public:
    static inline const std::string cname = "print_non_default_configs";
    static inline const std::string hname = "";
    PrintNonDefaultConfigs(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->print_non_default_configs();
    }
    bool requires_document() { return false; }
};

class PrintUndocumentedCommandsCommand : public Command {
public:
    static inline const std::string cname = "print_undocumented_commands";
    static inline const std::string hname = "";
    PrintUndocumentedCommandsCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->print_undocumented_commands();
    }

    bool requires_document() { return false; }
};

class PrintUndocumentedConfigsCommand : public Command {
public:
    static inline const std::string cname = "print_undocumented_configs";
    static inline const std::string hname = "";
    PrintUndocumentedConfigsCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->print_undocumented_configs();
    }

    bool requires_document() { return false; }
};

class ToggleFullscreenCommand : public Command {
public:
    static inline const std::string cname = "toggle_fullscreen";
    static inline const std::string hname = "Toggle fullscreen mode";
    ToggleFullscreenCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_fullscreen();
    }

    bool requires_document() { return false; }
};

class MaximizeCommand : public Command {
public:
    static inline const std::string cname = "maximize";
    static inline const std::string hname = "Maximize window";
    MaximizeCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->maximize_window();
    }

    bool requires_document() { return false; }
};

class ToggleOneWindowCommand : public Command {
public:
    static inline const std::string cname = "toggle_one_window";
    static inline const std::string hname = "Open/close helper window";
    ToggleOneWindowCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_two_window_mode();
    }

    bool requires_document() { return false; }
};

class ToggleHighlightCommand : public Command {
public:
    static inline const std::string cname = "toggle_highlight_links";
    static inline const std::string hname = "Toggle whether PDF links are highlighted";
    ToggleHighlightCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_highlight_links();
    }

    bool requires_document() { return false; }
};

class ToggleSynctexCommand : public Command {
public:
    static inline const std::string cname = "toggle_synctex";
    static inline const std::string hname = "Toggle synctex mode";
    ToggleSynctexCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_synctex_mode();
    }

    bool requires_document() { return false; }
};

class TurnOnSynctexCommand : public Command {
public:
    static inline const std::string cname = "turn_on_synctex";
    static inline const std::string hname = "Turn synxtex mode on";
    TurnOnSynctexCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->set_synctex_mode(true);
    }

    bool requires_document() { return false; }
};

class ToggleShowLastCommand : public Command {
public:
    static inline const std::string cname = "toggle_show_last_command";
    static inline const std::string hname = "Toggle whether the last command is shown in statusbar";
    ToggleShowLastCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->should_show_last_command = !widget->should_show_last_command;
    }
};


class ForwardSearchCommand : public Command {
public:
    static inline const std::string cname = "synctex_forward_search";
    static inline const std::string hname = "";
    ForwardSearchCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<std::wstring> file_path = {};
    std::optional<int> line = {};
    std::optional<int> column = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!file_path.has_value()) {
            return Requirement{ RequirementType::File, "File Path" };
        }
        if (!line.has_value()) {
            return Requirement{ RequirementType::Text, "Line number" };
        }
        return {};
    }

    void set_file_requirement(std::wstring value) {
        file_path = value;
    }

    void set_text_requirement(std::wstring text) {
        QStringList parts = QString::fromStdWString(text).split(" ");
        if (parts.size() == 1) {
            line = parts[0].toInt();
        }
        else {
            line = parts[0].toInt();
            column = parts[1].toInt();
        }

    }

    void perform() {
        widget->do_synctex_forward_search(widget->doc()->get_path(), file_path.value(), line.value(), column.value_or(0));
    }

};

class ExternalSearchCommand : public SymbolCommand {
public:
    static inline const std::string cname = "external_search";
    static inline const std::string hname = "Search using external search engines";
    ExternalSearchCommand(MainWidget* w) : SymbolCommand(cname, w) {};
    void perform() {
        std::wstring selected_text = dv()->get_selected_text();

        if ((symbol >= 'a') && (symbol <= 'z')) {
            if (SEARCH_URLS[symbol - 'a'].size() > 0) {
                if (selected_text.size() > 0) {
                    search_custom_engine(selected_text, SEARCH_URLS[symbol - 'a']);
                }
                else {
                    std::optional<QString> overview_paper_name = widget->main_document_view->get_overview_paper_name();
                    if (overview_paper_name.has_value()) {
                        search_custom_engine(overview_paper_name->toStdWString(), SEARCH_URLS[symbol - 'a']);
                    }
                    else {
                        search_custom_engine(widget->doc()->detect_paper_name(), SEARCH_URLS[symbol - 'a']);
                    }
                }
            }
            else {
                std::wcerr << L"No search engine defined for symbol " << symbol << std::endl;
            }
        }
    }

    std::string get_symbol_hint_name() override {
        std::vector<std::string> available_chars;
        char str[2] = { 0 };
        for (int i = 0; i < 26; i++) {
            if (SEARCH_URLS[i].size() > 0) {
                str[0] = 'a' + i;
                available_chars.push_back(str);
            }
        }

        std::string res = get_name() + "(";
        for (int i = 0; i < available_chars.size(); i++) {
            res += available_chars[i];
            if (i < available_chars.size() - 1) {
                res += "/";
            }
        }
        res += ")";
        return res;

    }

};

class OpenSelectedUrlCommand : public Command {
public:
    static inline const std::string cname = "open_selected_url";
    static inline const std::string hname = "";
    OpenSelectedUrlCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        open_web_url((dv()->get_selected_text()).c_str());
    }
};

class ScreenDownCommand : public Command {
public:
    static inline const std::string cname = "screen_down";
    static inline const std::string hname = "Move screen down";
    ScreenDownCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        dv()->handle_move_screen(rp);
    }

};

class ScreenUpCommand : public Command {
public:
    static inline const std::string cname = "screen_up";
    static inline const std::string hname = "Move screen up";
    ScreenUpCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        dv()->handle_move_screen(-rp);
    }

};

class NextChapterCommand : public Command {
public:
    static inline const std::string cname = "next_chapter";
    static inline const std::string hname = "Go to next chapter";
    NextChapterCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->main_document_view->goto_chapter(rp);
    }

    bool pushes_state() {
        return true;
    }

};

class PrevChapterCommand : public Command {
public:
    static inline const std::string cname = "prev_chapter";
    static inline const std::string hname = "Go to previous chapter";
    PrevChapterCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int rp = num_repeats == 0 ? 1 : num_repeats;
        widget->main_document_view->goto_chapter(-rp);
    }

    bool pushes_state() {
        return true;
    }

};

class ShowContextMenuCommand : public Command {
public:
    static inline const std::string cname = "show_context_menu";
    static inline const std::string hname = "";
    ShowContextMenuCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_contextual_context_menu(QString::fromStdWString(CONTEXT_MENU_ITEMS));
        //widget->show_contextual_context_menu();
    }

    bool requires_document() { return false; }
};


std::unique_ptr<MenuItems> parse_menu_string(MainWidget* widget, QString name, QString menu_string) {
    ParseState parser;
    parser.index = 0;
    parser.str = menu_string;
    std::unique_ptr<MenuItems> res = std::make_unique<MenuItems>();
    res->name = name.toStdWString();
    while (true) {
        std::vector<CommandInvocation> invocations = parser.parse_next_macro_command();

        if (invocations.size() == 1 && invocations[0].command_name == "show_custom_context_menu") {

            if (invocations[0].command_args.size() == 2) {
                res->items.push_back(parse_menu_string(widget, invocations[0].command_args[0], invocations[0].command_args[1]));
            }
        }
        else {
            res->items.push_back(std::make_unique<MacroCommand>(widget, widget->command_manager, "", invocations));
        }
        if (parser.eof()) break;

        if (parser.cc() == '|') {
            parser.index++;
        }
    }
    return res;
}

class ShowCustomContextMenuCommand : public Command {
public:
    static inline const std::string cname = "show_custom_context_menu";
    static inline const std::string hname = "";
    ShowCustomContextMenuCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<std::wstring> menu_name = {};
    std::optional<std::wstring> commands = {};

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!menu_name.has_value()) {
            return Requirement{ RequirementType::Text, "Menu Name" };
        }
        if (!commands.has_value()) {
            return Requirement{ RequirementType::Text, "Commands" };
        }
        return {};
    }

    virtual void set_text_requirement(std::wstring value) {
        if (!menu_name.has_value()) {
            menu_name = value;
            return;
        }
        if (!commands.has_value()) {
            commands = value;
            return;
        }
    }


    void perform() {

        std::unique_ptr<MenuItems> menu = parse_menu_string(widget, QString::fromStdWString(menu_name.value()), QString::fromStdWString(commands.value()));
        widget->show_recursive_context_menu(std::move(menu));
    }

    bool requires_document() { return false; }
};

class OpenExternalTextEditorCommand : public Command {
public:
    static inline const std::string cname = "open_external_text_editor";
    static inline const std::string hname = "Edit the current text input in the configured external text editor";

    OpenExternalTextEditorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->open_external_text_editor();
    }

    bool requires_document() { return false; }
};

class ToggleDarkModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_dark_mode";
    static inline const std::string hname = "Toggle dark mode";

    ToggleDarkModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_dark_mode();
    }


    bool requires_document() { return false; }
};

class ToggleCustomColorMode : public Command {
public:
    static inline const std::string cname = "toggle_custom_color";
    static inline const std::string hname = "Toggle custom color mode";
    ToggleCustomColorMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_custom_color_mode();
    }

    bool requires_document() { return false; }
};

class TogglePresentationModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_presentation_mode";
    static inline const std::string hname = "Toggle presentation mode";
    TogglePresentationModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->toggle_presentation_mode();
    }

    bool requires_document() { return false; }
};

class TurnOnPresentationModeCommand : public Command {
public:
    static inline const std::string cname = "turn_on_presentation_mode";
    static inline const std::string hname = "Turn on presentation mode";
    TurnOnPresentationModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        dv()->set_presentation_mode(true);
    }

    bool requires_document() { return false; }
};

class ToggleMouseDragMode : public Command {
public:
    static inline const std::string cname = "toggle_mouse_drag_mode";
    static inline const std::string hname = "Toggle mouse drag mode";
    ToggleMouseDragMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_mouse_drag_mode();
    }

    bool requires_document() { return false; }
};

class ToggleFreehandDrawingMode : public Command {
public:
    static inline const std::string cname = "toggle_freehand_drawing_mode";
    static inline const std::string hname = "Toggle freehand drawing mode";
    ToggleFreehandDrawingMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_freehand_drawing_mode();
    }

    bool requires_document() { return false; }
};

class TogglePenDrawingMode : public Command {
public:
    static inline const std::string cname = "toggle_pen_drawing_mode";
    static inline const std::string hname = "Toggle pen drawing mode";
    TogglePenDrawingMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_pen_drawing_mode();
    }

    bool requires_document() { return false; }
};

class ToggleScratchpadMode : public Command {
public:
    static inline const std::string cname = "toggle_scratchpad_mode";
    static inline const std::string hname = "Toggle scratchpad";
    ToggleScratchpadMode(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_scratchpad_mode();
    }

    bool requires_document() { return false; }
};

class CloseWindowCommand : public Command {
public:
    static inline const std::string cname = "close_window";
    static inline const std::string hname = "Close window";
    CloseWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->close();
    }

    bool requires_document() { return false; }
};

class NewWindowCommand : public Command {
public:
    static inline const std::string cname = "new_window";
    static inline const std::string hname = "Open a new window";
    NewWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int new_id = widget->handle_new_window()->window_id;
        result = QString::number(new_id).toStdWString();
    }

    bool requires_document() { return false; }
};

class QuitCommand : public Command {
public:
    static inline const std::string cname = "quit";
    static inline const std::string hname = "Quit";
    QuitCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_close_event(true);
        QApplication::quit();
    }


    bool requires_document() { return false; }
};

class EscapeCommand : public Command {
public:
    static inline const std::string cname = "escape";
    static inline const std::string hname = "Escape";
    EscapeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_escape();
    }

    bool requires_document() { return false; }
};

class TogglePDFAnnotationsCommand : public Command {
public:
    static inline const std::string cname = "toggle_pdf_annotations";
    static inline const std::string hname = "Toggle whether PDF annotations should be rendered";
    TogglePDFAnnotationsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_pdf_annotations();
    }

    bool requires_document() { return true; }
};


class OpenLinkCommand : public Command {
public:
    static inline const std::string cname = "open_link";
    static inline const std::string hname = "Go to PDF links using keyboard";
    OpenLinkCommand(MainWidget* w) : Command(cname, w) {};
protected:
    std::optional<std::wstring> text = {};
    bool already_pre_performed = false;
public:

    virtual std::string text_requirement_name() {
        return "Label";
    }

    bool is_done() {
        if ((NUMERIC_TAGS && text.has_value())) return true;
        if ((!NUMERIC_TAGS) && text.has_value() && (text.value().size() == get_num_tag_digits(widget->num_visible_links()))) return true;
        return false;
    }

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
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

    virtual void perform() {
        widget->handle_open_link(text.value());
    }

    void pre_perform() {
        if (already_pre_performed) return;

        widget->clear_tag_prefix();
        widget->set_highlight_links(true, true);
        widget->invalidate_render();
        already_pre_performed = true;
    }

    virtual void set_text_requirement(std::wstring value) {
        this->text = value;
    }

    virtual void set_symbol_requirement(char value) {
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
};

class FindReferencesCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "find_references";
    static inline const std::string hname = "Find links to the selected text/link.";
    FindReferencesCommand(MainWidget* w) : OpenLinkCommand(w) {};
public:


    virtual std::optional<Requirement> next_requirement(MainWidget* widget) override{
        if (dv()->selected_character_rects.size() > 0) {
            return {};
        }
        return OpenLinkCommand::next_requirement(widget);
    }

    virtual void perform() {
        widget->push_state();
        if (dv()->selected_character_rects.size() > 0) {
            widget->handle_find_references_to_selected_text();
        }
        else {
            if (text.has_value()) {
                widget->handle_find_references_to_link(text.value());
            }
        }
        widget->reset_highlight_links();
    }

};

class KeyboardSelectLineCommand : public Command {
public:
    static inline const std::string cname = "keyboard_select_line";
    static inline const std::string hname = "Select a line using keyboard";
    std::vector<DocumentRect> highlight_rects;
    std::vector<int> index_in_page;
    int rects_size = 0;
    //int page;

    KeyboardSelectLineCommand(MainWidget* w) : Command(cname, w) {
        //const std::vector<AbsoluteRect> rects = widget->doc()->get_page_lines(widget->get_current_page_number()).merged_line_rects;
        const std::vector<AbsoluteRect> rects = widget->main_document_view->get_visible_line_rects(index_in_page);
        //page = widget->get_current_page_number();
        for (auto& rect : rects) {
            highlight_rects.push_back(rect.to_document(widget->doc()));
        }
        rects_size = highlight_rects.size();
    };
protected:
    std::optional<std::wstring> text = {};
    bool already_pre_performed = false;
public:

    virtual std::string text_requirement_name() {
        return "Label";
    }

    bool is_done() {
        int num_tag_digits = get_num_tag_digits(rects_size);
        if (text.has_value() && (text->size() == num_tag_digits)) {
            return true;
        }
        return false;
    }

    virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
        bool done = is_done();

        if (done) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Symbol, "Label" };
        }
    }

    virtual void perform() {
        int index = get_index_from_tag(utf8_encode(text.value()));
        if (index < highlight_rects.size() && index >= 0) {
            int page = highlight_rects[index].page;
            widget->main_document_view->set_line_index(index_in_page[index], page);
            //widget->handle_open_link(text.value());
            widget->set_highlighted_tags({});
            widget->main_document_view->set_should_highlight_words(false);
        }
    }

    std::wstring get_text_default_value() override{
        return L"";
    }

    void pre_perform() {
        if (already_pre_performed) return;

        widget->clear_tag_prefix();
        widget->main_document_view->set_highlight_words(highlight_rects);
        widget->main_document_view->set_should_highlight_words(true);
        //widget->set_highlight_links(true, true);
        widget->invalidate_render();
        already_pre_performed = true;
    }

    virtual void set_text_requirement(std::wstring value) {
        this->text = value;
    }

    virtual void set_symbol_requirement(char value) {
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
};

class StartReadingCommand : public Command {
public:
    static inline const std::string cname = "start_reading";
    static inline const std::string hname = "Read using local text to speech";
    StartReadingCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (dv()->is_ruler_mode()) {
            widget->handle_start_reading();
        }
        else {
            std::vector<std::unique_ptr<Command>> cmds;
            cmds.push_back(std::move(std::make_unique<KeyboardSelectLineCommand>(widget)));
            cmds.push_back(std::move(std::make_unique<StartReadingCommand>(widget)));

            widget->handle_command_types(
                std::make_unique<MacroCommand>(widget, widget->command_manager, cname, std::move(cmds)), 1);
        }
    }
};


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

class OverviewLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "overview_link";
    static inline const std::string hname = "Overview to PDF links using keyboard";
    OverviewLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform() {
        widget->handle_overview_link(text.value());
    }

    std::string get_name() {
        return cname;
    }
};

class DownloadLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "download_link";
    static inline const std::string hname = "Download the destination reference of a PDF link";
    DownloadLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform() {
        std::optional<PdfLink> link = widget->get_selected_link(text.value());

        if (link) {
            ParsedUri uri = widget->doc()->parse_link(link.value());
            PdfLinkTextInfo link_info = widget->doc()->get_pdf_link_text(link.value());
            AbsoluteRect link_source_rect = DocumentRect{ link->rects[0], link->source_page }.to_absolute(widget->doc());

            std::optional<std::pair<QString, std::vector<PagelessDocumentRect>>> reftext = widget->doc()->get_page_bib_with_reference(uri.page - 1, link_info.link_text);
            if (reftext.has_value()) {
                QString paper_name = get_paper_name_from_reference_text(reftext->first);
                widget->download_and_portal(paper_name.toStdWString(), link_source_rect.center());
            }
        }
        widget->reset_highlight_links();
        widget->clear_tag_prefix();
    }

    std::string get_name() {
        return cname;
    }
};

class PortalToLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "portal_to_link";
    static inline const std::string hname = "Create a portal to PDF links using keyboard";
    PortalToLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform() {
        widget->handle_portal_to_link(text.value());
    }

    std::string get_name() {
        return cname;
    }
};

class CopyLinkCommand : public TextCommand {
public:
    static inline const std::string cname = "copy_link";
    static inline const std::string hname = "Copy URL of PDF links using keyboard";
    CopyLinkCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->handle_open_link(text.value(), true);
    }

    void pre_perform() {
        widget->set_highlight_links(true, true);
        widget->invalidate_render();

    }


    std::string text_requirement_name() {
        return "Label";
    }
};

class KeyboardSelectCommand : public TextCommand {
public:
    static inline const std::string cname = "keyboard_select";
    static inline const std::string hname = "Select text using keyboard";
    KeyboardSelectCommand(MainWidget* w) : TextCommand(cname, w) {};

    std::vector<DocumentRect> tag_rects;
    bool has_pre_performed = false;

    void on_text_change(const QString& new_text) override {
        std::vector<std::string> selected_tags;
        QStringList tags = new_text.split(" ");
        for (auto& tag : tags) {
            selected_tags.push_back(tag.toStdString());
        }
        widget->set_highlighted_tags(selected_tags);
    }

    void on_cancel() override {
        widget->set_highlighted_tags({});

    }

    std::wstring get_text_default_value() override{
        return L"";
    }

    void perform() {

        if (!has_pre_performed) {
            // when running the command from the command line or e.g. python api, the pre_perform
            // might not be called, since this command relies the the pre-perform for its logic
            // we need to call it if it is not already called
            pre_perform();
        }

        dv()->handle_keyboard_select(text.value(), tag_rects);
        widget->set_highlighted_tags({});
    }

    void pre_perform() {
        has_pre_performed = true;
        tag_rects = dv()->highlight_words();
        widget->clear_tag_prefix();

    }

    std::string text_requirement_name() {
        return "Labels";
    }
};


class TagCommand : public Command {
public:
    std::vector<DocumentRect> tags;
    std::string selected_tag;
    bool pre_performed = false;

    TagCommand(std::string cname, MainWidget* w) : Command(cname, w) {

    }


    bool is_done() {
        return get_num_tag_digits(tags.size()) == selected_tag.size();
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (is_done()) {
            return {};
        }
        else {
            return Requirement{ RequirementType::Symbol, "tag" };
        }
    }

    void set_symbol_requirement(char value) override {
        selected_tag.push_back(value);
        if (!is_done()) {
            widget->set_tag_prefix(utf8_decode(selected_tag));
        }
    }


    void pre_perform() override {
        if (!pre_performed) {
            widget->clear_tag_prefix();
            tags = dv()->highlight_words();
            pre_performed = true;
        }
    }

};

class KeyboardOverviewCommand : public TagCommand {
public:
    static inline const std::string cname = "keyboard_overview";
    static inline const std::string hname = "Open an overview using keyboard";
    KeyboardOverviewCommand(MainWidget* w) : TagCommand(cname, w) {};

    void perform() {
        std::optional<fz_irect> rect_ = dv()->get_tag_window_rect(tags, selected_tag);
        if (rect_) {
            fz_irect rect = rect_.value();
            widget->overview_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
            widget->set_should_highlight_words(false);
            widget->invalidate_render();
        }
    }
};

class KeyboardSmartjumpCommand : public TagCommand {
public:
    static inline const std::string cname = "keyboard_smart_jump";
    static inline const std::string hname = "Smart jump using keyboard";
    KeyboardSmartjumpCommand(MainWidget* w) : TagCommand(cname, w) {};

    void perform() {
        std::optional<fz_irect> rect_ = dv()->get_tag_window_rect(tags, selected_tag);
        if (rect_) {
            fz_irect rect = rect_.value();
            widget->smart_jump_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
            widget->set_should_highlight_words(false);
        }
    }

    bool pushes_state() {
        return true;
    }
};

class KeysCommand : public Command {
public:
    static inline const std::string cname = "keys";
    static inline const std::string hname = "Open the default keys config file";
    KeysCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        open_file(default_keys_path.get_path(), true);
    }

    bool requires_document() { return false; }
};

class KeysUserCommand : public Command {
public:
    static inline const std::string cname = "keys_user";
    static inline const std::string hname = "Open the default keys_user config file";
    KeysUserCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::optional<Path> key_file_path = widget->input_handler->get_or_create_user_keys_path();
        if (key_file_path) {
            open_file(key_file_path.value().get_path(), true);
        }
    }

    bool requires_document() { return false; }
};

class KeysUserAllCommand : public Command {
public:
    static inline const std::string cname = "keys_user_all";
    static inline const std::string hname = "List all user keys config files";
    KeysUserAllCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_keys_user_all();
    }

    bool requires_document() { return false; }
};

class PrefsCommand : public Command {
public:
    static inline const std::string cname = "prefs";
    static inline const std::string hname = "Open the default prefs config file";
    PrefsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        open_file(default_config_path.get_path(), true);
    }

    bool requires_document() { return false; }
};

class PrefsUserCommand : public Command {
public:
    static inline const std::string cname = "prefs_user";
    static inline const std::string hname = "Open the default prefs_user config file";
    PrefsUserCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::optional<Path> pref_file_path = widget->config_manager->get_or_create_user_config_file();
        if (pref_file_path) {
            open_file(pref_file_path.value().get_path(), true);
        }
    }

    bool requires_document() { return false; }
};

class PrefsUserAllCommand : public Command {
public:
    static inline const std::string cname = "prefs_user_all";
    static inline const std::string hname = "List all user keys config files";
    PrefsUserAllCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_prefs_user_all();
    }

    bool requires_document() { return false; }
};

class FitToPageWidthRatioCommand : public Command {
public:
    static inline const std::string cname = "fit_to_page_width_ratio";
    static inline const std::string hname = "Fit page to a percentage of window width";
    FitToPageWidthRatioCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->fit_to_page_width(false, true);
        widget->main_document_view->last_smart_fit_page = {};
    }

};

class SmartJumpUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "smart_jump_under_cursor";
    static inline const std::string hname = "Perform a smart jump to the reference under cursor";
    SmartJumpUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->smart_jump_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }
};

class OpenContainingFolderCommand : public Command {
public:
    static inline const std::string cname = "open_containing_folder";
    static inline const std::string hname = "Open the folder that contains the current file in the default file explorer";
    OpenContainingFolderCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::wstring path = widget->doc()->get_path();
        QFileInfo file_info(QString::fromStdWString(path));
        QDir directory = file_info.dir();
        QDesktopServices::openUrl(directory.absolutePath());
    }
};

class DownloadPaperUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "download_paper_under_cursor";
    static inline const std::string hname = "Try to download the paper name under cursor";
    DownloadPaperUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->download_paper_under_cursor();
    }

};


class OverviewUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "overview_under_cursor";
    static inline const std::string hname = "Open an overview to the reference under cursor";
    OverviewUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->overview_under_pos({ mouse_pos.x(), mouse_pos.y() });
        widget->invalidate_render();
    }

};

class SynctexUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "synctex_under_cursor";
    static inline const std::string hname = "Perform a synctex search to the tex file location corresponding to cursor location";
    SynctexUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->synctex_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

};

class SynctexUnderRulerCommand : public Command {
public:
    static inline const std::string cname = "synctex_under_ruler";
    static inline const std::string hname = "";
    SynctexUnderRulerCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = widget->handle_synctex_to_ruler();
    }

};

class VisualMarkUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "visual_mark_under_cursor";
    static inline const std::string hname = "Highlight the line under cursor";
    VisualMarkUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->visual_mark_under_pos({ mouse_pos.x(), mouse_pos.y() });
    }

};

class RulerUnderSelectedPointCommand : public Command {
public:
    static inline const std::string cname = "ruler_under_selected_point";
    static inline const std::string hname = "Select a point to set a ruler";

    std::optional<AbsoluteDocumentPos> point_;

    RulerUnderSelectedPointCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!point_.has_value()) {
            Requirement req = { RequirementType::Point, "Ruler line location" };
            return req;
        }
        return {};
    }

    virtual void set_point_requirement(AbsoluteDocumentPos value) {
        point_ = value;
    }

    void perform() {
        // QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        WindowPos pos = point_->to_window(widget->main_document_view);
        widget->visual_mark_under_pos({ pos.x, pos.y });
    }


};

class StartMobileTextSelectionAtPointCommand : public Command {
public:
    static inline const std::string cname = "start_mobile_text_selection_at_point";
    static inline const std::string hname = "Start mobile text selection at selected point";

    std::optional<AbsoluteDocumentPos> point_;

    StartMobileTextSelectionAtPointCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!point_.has_value()) {
            Requirement req = { RequirementType::Point, "Text selection location" };
            return req;
        }
        return {};
    }

    virtual void set_point_requirement(AbsoluteDocumentPos value) {
        point_ = value;
    }

    void perform() {
        widget->start_mobile_selection_under_point(point_.value());
    }


};

class CloseOverviewCommand : public Command {
public:
    static inline const std::string cname = "close_overview";
    static inline const std::string hname = "Close overview window";
    CloseOverviewCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->set_overview_page({}, false);
    }

};

class CloseVisualMarkCommand : public Command {
public:
    static inline const std::string cname = "close_visual_mark";
    static inline const std::string hname = "Stop ruler mode";
    CloseVisualMarkCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->exit_ruler_mode();
    }

};

class ZoomInCursorCommand : public Command {
public:
    static inline const std::string cname = "zoom_in_cursor";
    static inline const std::string hname = "Zoom in centered on mouse cursor";
    ZoomInCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->main_document_view->zoom_in_cursor({ mouse_pos.x(), mouse_pos.y() });
        widget->main_document_view->last_smart_fit_page = {};
    }

};

class ZoomOutCursorCommand : public Command {
public:
    static inline const std::string cname = "zoom_out_cursor";
    static inline const std::string hname = "Zoom out centered on mouse cursor";
    ZoomOutCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(widget->cursor_pos());
        widget->main_document_view->zoom_out_cursor({ mouse_pos.x(), mouse_pos.y() });
        widget->main_document_view->last_smart_fit_page = {};
    }

};

class GotoLeftCommand : public Command {
public:
    static inline const std::string cname = "goto_left";
    static inline const std::string hname = "Go to far left side of the page";
    GotoLeftCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_left();
    }

};

class GotoLeftSmartCommand : public Command {
public:
    static inline const std::string cname = "goto_left_smart";
    static inline const std::string hname = "Go to far left side of the page, ignoring white page margins";
    GotoLeftSmartCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_left_smart();
    }

};

class GotoRightCommand : public Command {
public:
    static inline const std::string cname = "goto_right";
    static inline const std::string hname = "Go to far right side of the page";
    GotoRightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_right();
    }

};

class GotoRightSmartCommand : public Command {
public:
    static inline const std::string cname = "goto_right_smart";
    static inline const std::string hname = "Go to far right side of the page, ignoring white page margins";
    GotoRightSmartCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->goto_right_smart();
    }

};

class RotateClockwiseCommand : public Command {
public:
    static inline const std::string cname = "rotate_clockwise";
    static inline const std::string hname = "Rotate clockwise";
    RotateClockwiseCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->rotate();
        widget->rotate_clockwise();
    }

};

class RotateCounterClockwiseCommand : public Command {
public:
    static inline const std::string cname = "rotate_counterclockwise";
    static inline const std::string hname = "Rotate counter clockwise";
    RotateCounterClockwiseCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->rotate();
        widget->rotate_counterclockwise();
    }

};

class GotoNextHighlightCommand : public Command {
public:
    static inline const std::string cname = "goto_next_highlight";
    static inline const std::string hname = "Go to the next highlight";
    GotoNextHighlightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y());
        if (next_highlight.has_value()) {
            widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
    }
};

class GotoPrevHighlightCommand : public Command {
public:
    static inline const std::string cname = "goto_prev_highlight";
    static inline const std::string hname = "Go to the previous highlight";
    GotoPrevHighlightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {

        auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y());
        if (prev_highlight.has_value()) {
            widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
    }
};

class GotoNextHighlightOfTypeCommand : public Command {
public:
    static inline const std::string cname = "goto_next_highlight_of_type";
    static inline const std::string hname = "Go to the next highlight with the current highlight type";
    GotoNextHighlightOfTypeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (next_highlight.has_value()) {
            widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
    }

};

class GotoPrevHighlightOfTypeCommand : public Command {
public:
    static inline const std::string cname = "goto_prev_highlight_of_type";
    static inline const std::string hname = "Go to the previous highlight with the current highlight type";
    GotoPrevHighlightOfTypeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (prev_highlight.has_value()) {
            widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
    }
};

class SetSelectHighlightTypeCommand : public SymbolCommand {
public:
    static inline const std::string cname = "set_select_highlight_type";
    static inline const std::string hname = "Set the selected highlight type";
    SetSelectHighlightTypeCommand(MainWidget* w) : SymbolCommand(cname, w) {};
    void perform() {
        widget->select_highlight_type = symbol;
    }

    bool requires_document() { return false; }

};

class SetFreehandType : public SymbolCommand {
public:
    static inline const std::string cname = "set_freehand_type";
    static inline const std::string hname = "Set the freehand drawing color type";
    SetFreehandType(MainWidget* w) : SymbolCommand(cname, w) {};
    void perform() {
        dv()->current_freehand_type = symbol;
    }

    bool requires_document() { return false; }
};

class SetFreehandAlphaCommand : public TextCommand {
public:
    static inline const std::string cname = "set_freehand_alpha";
    static inline const std::string hname = "Set the freehand drawing alpha";
    SetFreehandAlphaCommand(MainWidget* w) : TextCommand(cname, w) {};
    void perform() {
        if (text.has_value() && text->size() > 0) {
            bool was_number = false;
            float alpha = QString::fromStdWString(text.value()).toFloat(&was_number);
            if (!was_number || alpha > 1 || alpha < 0) {
                alpha = 1.0f;
            }
            dv()->set_current_freehand_alpha(alpha);

        }
    }

    bool requires_document() { return false; }
};

class AddHighlightWithCurrentTypeCommand : public Command {
public:
    static inline const std::string cname = "add_highlight_with_current_type";
    static inline const std::string hname = "Highlight selected text with current selected highlight type";
    AddHighlightWithCurrentTypeCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->main_document_view->selected_character_rects.size() > 0) {
            widget->add_highlight_to_current_document(widget->dv()->selection_begin, widget->dv()->selection_end, widget->select_highlight_type);
            dv()->clear_selected_text();
        }
    }

};

class UndoDrawingCommand : public Command {
public:
    static inline const std::string cname = "undo_drawing";
    static inline const std::string hname = "Undo freehand drawing";
    UndoDrawingCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->handle_undo_drawing();
    }

    bool requires_document() { return true; }
};


class EnterPasswordCommand : public TextCommand {
public:
    static inline const std::string cname = "enter_password";
    static inline const std::string hname = "Enter password";
    EnterPasswordCommand(MainWidget* w) : TextCommand(cname, w) {};
    void perform() {
        std::string password = utf8_encode(text.value());
        widget->add_password(widget->main_document_view->get_document()->get_path(), password);
    }

    std::string text_requirement_name() {
        return "Password";
    }
};

class ToggleFastreadCommand : public Command {
public:
    static inline const std::string cname = "toggle_fastread";
    static inline const std::string hname = "Go to top of current page";
    ToggleFastreadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_fastread();
    }
};

class GotoTopOfPageCommand : public Command {
public:
    static inline const std::string cname = "goto_top_of_page";
    static inline const std::string hname = "Go to top of current page";
    GotoTopOfPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->goto_top_of_page();
    }

};

class GotoBottomOfPageCommand : public Command {
public:
    static inline const std::string cname = "goto_bottom_of_page";
    static inline const std::string hname = "Go to bottom of current page";
    GotoBottomOfPageCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->main_document_view->goto_bottom_of_page();
    }

};

class ReloadCommand : public Command {
public:
    static inline const std::string cname = "reload";
    static inline const std::string hname = "Reload document";
    ReloadCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->reload();
    }

};

class ReloadNoFlickerCommand : public Command {
public:
    static inline const std::string cname = "reload_no_flicker";
    static inline const std::string hname = "Reload document with no screen flickering";
    ReloadNoFlickerCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->reload(false);
    }

};

class ReloadConfigCommand : public Command {
public:
    static inline const std::string cname = "reload_config";
    static inline const std::string hname = "Reload configs";
    ReloadConfigCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->on_config_file_changed(widget->config_manager);
    }

    bool requires_document() { return false; }
};

class TurnOnAllDrawings : public Command {
public:
    static inline const std::string cname = "turn_on_all_drawings";
    static inline const std::string hname = "Make all freehand drawings visible";
    TurnOnAllDrawings(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->hande_turn_on_all_drawings();
    }

    bool requires_document() { return false; }
};

class TurnOffAllDrawings : public Command {
public:
    static inline const std::string cname = "turn_off_all_drawings";
    static inline const std::string hname = "Make all freehand drawings invisible";
    TurnOffAllDrawings(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->hande_turn_off_all_drawings();
    }

    bool requires_document() { return false; }
};

class SetStatusStringCommand : public TextCommand {
public:
    static inline const std::string cname = "set_status_string";
    static inline const std::string hname = "Set custom message to be shown in statusbar";
    SetStatusStringCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->set_status_message(text.value());
    }

    std::string text_requirement_name() {
        return "Status String";
    }


    bool requires_document() { return false; }
};

class ClearStatusStringCommand : public Command {
public:
    static inline const std::string cname = "clear_status_string";
    static inline const std::string hname = "Clear custom statusbar message";
    ClearStatusStringCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->set_status_message(L"");
    }

    bool requires_document() { return false; }
};

class ToggleTittlebarCommand : public Command {
public:
    static inline const std::string cname = "toggle_titlebar";
    static inline const std::string hname = "Toggle window titlebar";
    ToggleTittlebarCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->toggle_titlebar();
    }

    bool requires_document() { return false; }
};

class NextPreviewCommand : public Command {
public:
    static inline const std::string cname = "next_overview";
    static inline const std::string hname = "Go to the next candidate in overview window";
    NextPreviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->dv()->smart_view_candidates.size() > 0) {
            //widget->index_into_candidates = (widget->index_into_candidates + 1) % widget->smart_view_candidates.size();
            //widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
            widget->goto_ith_next_overview(1);
        }
        else if (dv()->search_results.size() > 0) {
            dv()->goto_search_result(1, true);
        }
    }

};

class PreviousPreviewCommand : public Command {
public:
    static inline const std::string cname = "previous_overview";
    static inline const std::string hname = "Go to the previous candidate in overview window";
    PreviousPreviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        if (widget->dv()->smart_view_candidates.size() > 0) {
            //widget->index_into_candidates = mod(widget->index_into_candidates - 1, widget->smart_view_candidates.size());
            //widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
            widget->goto_ith_next_overview(-1);
        }
        else if (dv()->search_results.size() > 0) {
            dv()->goto_search_result(-1, true);
        }
    }
};

class GotoOverviewCommand : public Command {
public:
    static inline const std::string cname = "goto_overview";
    static inline const std::string hname = "Go to the current overview location";
    GotoOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->goto_overview();
    }

};

class PortalToOverviewCommand : public Command {
public:
    static inline const std::string cname = "portal_to_overview";
    static inline const std::string hname = "Create a portal to the current overview location";
    PortalToOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->handle_portal_to_overview();
    }

};

class GotoSelectedTextCommand : public Command {
public:
    static inline const std::string cname = "goto_selected_text";
    static inline const std::string hname = "Go to the location of current selected text";
    GotoSelectedTextCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->long_jump_to_destination(widget->dv()->selection_begin.y);
    }

};

class SetWindowRectCommand : public TextCommand {
public:
    static inline const std::string cname = "set_window_rect";
    static inline const std::string hname = "Move and resize the window to the given coordinates (in pixels).";
    SetWindowRectCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        if (!text.has_value()) return;

        QStringList parts = QString::fromStdWString(text.value()).split(' ');
        if (parts.size() < 4) {
            show_error_message(L"Invalid format. Expected: x y width height");
            return;
        }

        bool ok1, ok2, ok3, ok4;
        int x0 = parts[0].toInt(&ok1);
        int y0 = parts[1].toInt(&ok2);
        int w = parts[2].toInt(&ok3);
        int h = parts[3].toInt(&ok4);

        if (!ok1 || !ok2 || !ok3 || !ok4) {
            show_error_message(L"Invalid numbers in input");
            return;
        }

        if (w <= 0 || h <= 0) {
            show_error_message(L"Width and height must be positive");
            return;
        }

        widget->resize(w, h);
        widget->move(x0, y0);
    }

};

class LoginWithGoogleCommand : public Command {
public:
    static inline const std::string cname = "login_with_google";
    static inline const std::string hname = "Login to sioyek using your google account";
    std::optional<std::wstring> token;

    LoginWithGoogleCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!token.has_value()) {
            return Requirement{ RequirementType::Password, "Token" };
        }
        return {};
    }


    void set_text_requirement(std::wstring value) {
        token = value;
    }

    void pre_perform() {
        // open web browser to get token
        QDesktopServices::openUrl(QUrl("https://127.0.0.1:8081/app/login_token"));
    }

    void perform() {
        widget->sioyek_network_manager->ACCESS_TOKEN = utf8_encode(token.value());
        widget->sioyek_network_manager->persist_access_token(utf8_encode(token.value()));

        widget->sioyek_network_manager->status = ServerStatus::LoggedIn;
        widget->sioyek_network_manager->handle_one_time_network_operations();
    }
};

class LoginCommand : public Command {
public:
    static inline const std::string cname = "login";
    static inline const std::string hname = "Login to sioyek";
    std::optional<std::wstring> email;
    std::optional<std::wstring> password;

    LoginCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (email.has_value() && password.has_value()) return {};
        if (email.has_value()) {
            return Requirement{ RequirementType::Password, "password" };
        }
        return Requirement{ RequirementType::Text, "email" };
    }


    void set_text_requirement(std::wstring value) {
        if (email.has_value()) {
            password = value;
        }
        else {
            email = value;
        }
    }

    void perform() {
        widget->handle_login(email.value(), password.value());
    }
};

class LogoutCommand : public Command {
public:
    static inline const std::string cname = "logout";
    static inline const std::string hname = "Logout from sioyek servers";

    LogoutCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->sioyek_network_manager->handle_logout();
    }
};

class CancelAllDownloadsCommand : public Command {
public:
    static inline const std::string cname = "cancel_all_downloads";
    static inline const std::string hname = "Cancel all pending downloads";

    CancelAllDownloadsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->sioyek_network_manager->cancel_all_downlods();
    }
};

class CitersCommand : public Command {
public:
    static inline const std::string cname = "citers";
    static inline const std::string hname = "Show a list of citers of this paper";

    CitersCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_citers_of_current_paper();
    }
};

class ResumeToServerLocationCommand : public Command {
public:
    static inline const std::string cname = "resume_to_server";
    static inline const std::string hname = "Jump to the location of current document in sioyek servers";

    ResumeToServerLocationCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_resume_to_server_location();
    }
};

class LoginUsingAccessTokenCommand : public Command {
public:
    static inline const std::string cname = "login_using_access_token";
    static inline const std::string hname = "Login using the saved access token from previous login";
    LoginUsingAccessTokenCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->sioyek_network_manager->load_access_token();
    }

};

class SynchronizeCommand : public Command {
public:
    static inline const std::string cname = "synchronize";
    static inline const std::string hname = "Synchronize a desyncronized file with server";
    SynchronizeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->synchronize_if_desynchronized();
    }

};

class ForceDownloadAnnotations : public Command {
public:
    static inline const std::string cname = "force_download_annotations";
    static inline const std::string hname = "Download all annotations from the server into local database";
    ForceDownloadAnnotations(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->sioyek_network_manager->download_annotations_since_last_sync(true);
    }

};

class UploadCurrentFileCommand : public Command {
public:
    static inline const std::string cname = "upload_current_file";
    static inline const std::string hname = "Upload the current file to sioyek servers";
    UploadCurrentFileCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->upload_current_file();
    }

};

class DeleteCurrentFileFromServer : public Command {
public:
    static inline const std::string cname = "delete_current_file_from_server";
    static inline const std::string hname = "Delete the current file from sioyek servers";
    DeleteCurrentFileFromServer(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->delete_current_file_from_server();
    }

};

class ResyncDocumentCommand : public Command {
public:
    static inline const std::string cname = "resync_document";

    static inline const std::string hname = "Re-sync current document's annotations with sioyek servers";
    ResyncDocumentCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_sync_open_document();
    }

};

class DownloadUnsyncedFilesCommand : public Command {
public:
    static inline const std::string cname = "download_unsynced_files";
    static inline const std::string hname = "Download unsynced files from sioyek servers";
    DownloadUnsyncedFilesCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->sioyek_network_manager->download_unsynced_files(widget, widget->db_manager);
    }

};

class SyncCurrentFileLocation : public Command {
public:
    static inline const std::string cname = "sync_current_file_location";
    static inline const std::string hname = "Sync the current file location to sioyek servers";
    SyncCurrentFileLocation(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->sync_current_file_location_to_servers();
    }
};

class FocusTextCommand : public TextCommand {
public:
    static inline const std::string cname = "focus_text";
    static inline const std::string hname = "Focus on the given text";
    FocusTextCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        std::wstring text_ = text.value();
        widget->main_document_view->focus_on_current_page_text(text_);
    }

    std::string text_requirement_name() {
        return "Text to focus";
    }
};

class DownloadOverviewPaperCommand : public TextCommand {
public:
    static inline const std::string cname = "download_overview_paper";
    static inline const std::string hname = "Download the referenced paper overview window";
    std::optional<AbsoluteRect> source_rect = {};
    std::wstring src_doc_path;

    DownloadOverviewPaperCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {

        std::wstring text_ = text.value();

        if (source_rect) {
            widget->download_and_portal(text_, source_rect->center());
        }
        else {
            widget->download_paper_with_name(text_);
        }

    }

    void pre_perform() {
        std::optional<QString> paper_name = widget->main_document_view->get_overview_paper_name();
        src_doc_path = widget->doc()->get_path();

        if (paper_name) {
            source_rect = dv()->get_overview_source_rect();

            if (TOUCH_MODE) {
                TouchTextEdit* paper_name_editor = dynamic_cast<TouchTextEdit*>(widget->current_widget_stack.back());
                if (paper_name_editor) {
                    paper_name_editor->set_text(paper_name.value().toStdWString());
                    widget->close_overview();
                }
                //widget->close_overview();
            }
            else {
                widget->text_command_line_edit->setText(
                    paper_name.value()
                );
                widget->close_overview();
            }
        }
    }

    std::string text_requirement_name() {
        return "Paper Name";
    }
};

class DownloadOverviewPaperNoPrompt : public Command {
public:
    static inline const std::string cname = "download_overview_paper_no_prompt";
    static inline const std::string hname = "Download and portal to the current highlighted overview paper";
    DownloadOverviewPaperNoPrompt(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->download_and_portal_to_highlighted_overview_paper();
        widget->close_overview();
    }
};

class GotoWindowCommand : public Command {
public:
    static inline const std::string cname = "goto_window";
    static inline const std::string hname = "Open a list of all sioyek windows";
    GotoWindowCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_goto_window();
    }

    bool requires_document() { return false; }
};

class ToggleSmoothScrollModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_smooth_scroll_mode";
    static inline const std::string hname = "Toggle smooth scroll mode";
    ToggleSmoothScrollModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_toggle_smooth_scroll_mode();
    }

    bool requires_document() { return false; }
};

class ToggleScrollbarCommand : public Command {
public:
    static inline const std::string cname = "toggle_scrollbar";
    static inline const std::string hname = "Toggle scrollbar";
    ToggleScrollbarCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_scrollbar();
    }

    bool requires_document() { return false; }

};

class OverviewToPortalCommand : public Command {
public:
    static inline const std::string cname = "overview_to_portal";
    static inline const std::string hname = "Open an overview to the closest portal";
    OverviewToPortalCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_overview_to_portal();
    }

};

class ScanNewFilesFromScanDirCommand : public Command {
public:
    static inline const std::string cname = "scan_new_files";
    static inline const std::string hname = "";
    ScanNewFilesFromScanDirCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->document_manager->scan_new_files_from_scan_directory();
    }

};

class DebugCommand : public Command {
public:
    inline static const std::string cname = "debug";
    inline static const std::string hname = "[internal]";
    static inline const bool developer_only = true;

    DebugCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_debug_command();
    }

};

class SetConfigCommand : public Command {
public:
    inline static const std::string cname = "set_config";
    inline static const std::string hname = "Set the value of a configuration";

    std::optional<std::wstring> config_name = {};
    std::optional<std::wstring> config_value = {};

    SetConfigCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        //widget->handle_debug_command();
        if (SHOW_SETCONFIG_IN_STATUSBAR) {
            widget->set_status_message(config_name.value() + L" = '" + config_value.value() + L"'");
        }
        if (widget->config_manager->deserialize_config(utf8_encode(config_name.value()), config_value.value())) {
            widget->on_config_changed(utf8_encode(config_name.value()), false);
        }
    }

    bool requires_document() override {
        return false;
    }

    std::wstring get_text_default_value() override{
        if (config_name.has_value()) {
            return widget->config_manager->get_config_value_string(config_name.value());
        }
        return L"";
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (!config_name.has_value()){
            return Requirement{ RequirementType::Text, "Config Name" };
        }

        if (!config_value.has_value()){
            return Requirement{ RequirementType::Text, "Config Value" };
        }
        return {};
    }

    void set_text_requirement(std::wstring value) override {
        if (!config_name.has_value()) {
            config_name = value;
        }
        else {
            config_value = value;
        }
    }


};

class ToggleConfigWithNameCommand : public Command {
public:
    inline static const std::string cname = "toggle_config";
    inline static const std::string hname = "Toggle the value of a boolean configuration.";

    std::optional<std::wstring> config_name = {};

    ToggleConfigWithNameCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto conf = widget->config_manager->get_mut_config_with_name(config_name.value());
        *(bool*)conf->value = !*(bool*)conf->value;
        widget->on_config_changed(utf8_encode(config_name.value()));
    }

    bool requires_document() override {
        return false;
    }


    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (!config_name.has_value()){
            return Requirement{ RequirementType::Text, "Config Name" };
        }

        return {};
    }

    void set_text_requirement(std::wstring value) override {
        if (!config_name.has_value()) {
            config_name = value;
        }
    }


};

class SaveConfigWithNameCommand : public Command {
public:
    inline static const std::string cname = "save_config";
    inline static const std::string hname = "Save the current value of a config to the auto config file.";

    std::optional<std::wstring> config_name = {};

    SaveConfigWithNameCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto conf = widget->config_manager->get_mut_config_with_name(config_name.value());
        conf->is_auto = true;
        widget->save_auto_config();
    }

    bool requires_document() override {
        return false;
    }


    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (!config_name.has_value()){
            return Requirement{ RequirementType::Text, "Config Name" };
        }

        return {};
    }

    void set_text_requirement(std::wstring value) override {
        if (!config_name.has_value()) {
            config_name = value;
        }
    }


};

class UndoDeleteCommand : public Command {
public:
    inline static const std::string cname = "undo_delete";
    inline static const std::string hname = "Undo deleted object";

    UndoDeleteCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->undo_delete();
    }

};

class FulltextSearchCommand : public Command {
public:
    static inline const std::string cname = "search_all_indexed_documents";
    static inline const std::string hname = "Fulltext search all indexed documents";

    FulltextSearchCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_fulltext_search();
    }

};

class ShowCurrentDocumentFulltextTags : public Command {
public:
    static inline const std::string cname = "show_current_document_fulltext_tags";
    static inline const std::string hname = "Show the tags with which the current document has been indexed";

    ShowCurrentDocumentFulltextTags(MainWidget* w) : Command(cname, w) {};

    void perform() {
        auto tags = widget->doc()->get_fulltext_tags();
        widget->show_items(tags, {}, [d=widget->doc()](std::wstring tag) {
            d->delete_fulltext_tag(tag);
            });
        //widget->set_curr
    }

};

class FulltextSearchCommandWithTag : public Command {
public:
    static inline const std::string cname = "search_all_indexed_documents_with_tag";
    static inline const std::string hname = "Fulltext search all indexed documents";

    std::optional<std::wstring> tag = {};

    FulltextSearchCommandWithTag(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!tag.has_value()) {
            return Requirement{ RequirementType::Generic, "Tag" };
        }
        return {};
    }

    void handle_generic_requirement() override{
        std::vector<std::wstring> all_tags = widget->db_manager->get_all_tags();
        widget->show_custom_option_list(all_tags);
    }

    void set_generic_requirement(QVariant value) {
        QString tag_string = value.toString();
        tag = tag_string.toStdWString();
    }

    void perform() {
        widget->handle_fulltext_search(L"", tag.value());
    }

};

class DocumentationSearchCommand : public Command {
public:
    static inline const std::string cname = "documentation_search";
    static inline const std::string hname = "Search sioyek's documentation";

    DocumentationSearchCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_documentation_search();
    }

};

class FulltextSearchCurrentDocumentCommand : public Command {
public:
    static inline const std::string cname = "fulltext_search_current_document";
    static inline const std::string hname = "Perform a fulltext search on current document";

    FulltextSearchCurrentDocumentCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (!widget->is_current_document_fulltext_indexed()) {
            show_error_message(
                L"Current document is not indexed, first index using create_fulltext_index_for_current_document"
            );
            return;
        }

        std::wstring file_checksum = utf8_decode(widget->doc()->get_checksum());
        widget->handle_fulltext_search(file_checksum);
    }


};

class DeleteDocumentFromFulltextSearchIndex : public Command {
public:
    static inline const std::string cname = "delete_document_from_fulltext_search_index";
    static inline const std::string hname = "Delete a document from fulltext search index";

    DeleteDocumentFromFulltextSearchIndex(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_delete_document_from_fulltext_search_index();
    }


};

class CreateFulltextIndexForCurrentDocumentCommand : public Command {
public:
    static inline const std::string cname = "create_fulltext_index_for_current_document";
    static inline const std::string hname = "Add current document to fulltext search index";

    CreateFulltextIndexForCurrentDocumentCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->index_current_document_for_fulltext_search();
    }

};

class CreateFulltextIndexForCurrentDocumentCommandWithTag : public TextCommand {
public:
    static inline const std::string cname = "create_fulltext_index_for_current_document_with_tag";
    static inline const std::string hname = "Add current document to fulltext search index";

    CreateFulltextIndexForCurrentDocumentCommandWithTag(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->index_current_document_for_fulltext_search(false, text.value());
    }

};

class CollapseMenuCommand : public Command {
public:
    inline static const std::string cname = "toggle_menu_collapse";
    inline static const std::string hname = "Toggle collapse of tree menus.";

    CollapseMenuCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_menu_collapse();
    }
};

class ShowTouchMainMenu : public Command {
public:
    static inline const std::string cname = "show_touch_main_menu";
    static inline const std::string hname = "";
    ShowTouchMainMenu(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_touch_main_menu();
    }

};

class ShowTouchPageSelectCommand : public Command {
public:
    static inline const std::string cname = "show_touch_page_select";
    static inline const std::string hname = "";
    ShowTouchPageSelectCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_touch_page_select();
    }
};

class ShowTouchHighlightTypeSelectCommand : public Command {
public:
    static inline const std::string cname = "show_touch_highlight_type_select";
    static inline const std::string hname = "";
    ShowTouchHighlightTypeSelectCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_touch_highlight_type_select();
    }
};

class ShowTouchDrawingMenu : public Command {
public:
    static inline const std::string cname = "show_touch_draw_controls";
    static inline const std::string hname = "";
    ShowTouchDrawingMenu(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_draw_controls();
    }

};

class ShowTouchSettingsMenu : public Command {
public:
    static inline const std::string cname = "show_touch_settings_menu";
    static inline const std::string hname = "";
    ShowTouchSettingsMenu(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_touch_settings_menu();
    }

};


class ExportPythonApiCommand : public Command {
public:
    static inline const std::string cname = "export_python_api";
    static inline const std::string hname = "";
    ExportPythonApiCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->export_python_api();
    }

};

class ScrollSelectedBookmarkDown : public Command {
public:
    static inline const std::string cname = "scroll_selected_bookmark_down";
    static inline const std::string hname = "";
    ScrollSelectedBookmarkDown(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->scroll_selected_bookmark(1);
    }
};

class ScrollSelectedBookmarkUp : public Command {
public:
    static inline const std::string cname = "scroll_selected_bookmark_up";
    static inline const std::string hname = "";
    ScrollSelectedBookmarkUp(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->scroll_selected_bookmark(-1);
    }
};

class SelectCurrentSearchMatchCommand : public Command {
public:
    static inline const std::string cname = "select_current_search_match";
    static inline const std::string hname = "";
    SelectCurrentSearchMatchCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_select_current_search_match();
    }

};

class SelectRulerTextCommand : public Command {
public:
    static inline const std::string cname = "select_ruler_text";
    static inline const std::string hname = "Select the text of current highlighted ruler line";
    SelectRulerTextCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->toggle_line_select_mode();
        //widget->handle_select_ruler_text();
    }

};

//class ToggleLineSelectCursor : public Command {
//public:
//    static inline const std::string cname = "toggle_line_select_cursor";
//    static inline const std::string hname = "Swap between begin/end of line selection.";
//    ToggleLineSelectCursor(MainWidget* w) : Command(cname, w) {};
//
//    void perform() {
//        widget->main_document_view->swap_line_select_cursor();
//    }
//
//};

class SelectRectCommand : public Command {
public:
    static inline const std::string cname = "select_rect";
    static inline const std::string hname = "Select a rectangle.";
    SelectRectCommand (MainWidget* w) : Command(cname, w) {};
    std::optional<AbsoluteRect> absrect = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!absrect.has_value()) {
            Requirement req = { RequirementType::Rect, "Command Rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect rect) {
        absrect = rect;
    }

    void perform() {
        QJsonDocument doc;
        QJsonObject rect_json;
        rect_json["x0"] = absrect->x0;
        rect_json["y0"] = absrect->y0;
        rect_json["x1"] = absrect->x1;
        rect_json["y1"] = absrect->y1;
        doc.setObject(rect_json);
        result = utf8_decode(doc.toJson().toStdString());
        widget->clear_selected_rect();
        widget->set_rect_select_mode(false);
    }

};

class ToggleTypingModeCommand : public Command {
public:
    static inline const std::string cname = "toggle_typing_mode";
    static inline const std::string hname = "Toggle typing minigame";
    ToggleTypingModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_toggle_typing_mode();
    }

};

class DonateCommand : public Command {
public:
    static inline const std::string cname = "donate";
    static inline const std::string hname = "Donate to support sioyek's development";
    DonateCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        open_web_url(L"https://www.buymeacoffee.com/ahrm");
    }

    bool requires_document() { return false; }
};

class OverviewNextItemCommand : public Command {
public:
    static inline const std::string cname = "overview_next_item";
    static inline const std::string hname = "Open an overview to the next search result";
    OverviewNextItemCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(num_repeats, true);
    }

};

class OverviewPrevItemCommand : public Command {
public:
    static inline const std::string cname = "overview_prev_item";
    static inline const std::string hname = "Open an overview to the previous search result";
    OverviewPrevItemCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->goto_search_result(-num_repeats, true);
    }

};

class DeleteHighlightUnderCursorCommand : public Command {
public:
    static inline const std::string cname = "delete_highlight_under_cursor";
    static inline const std::string hname = "Delete highlight under mouse cursor";
    DeleteHighlightUnderCursorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_delete_highlight_under_cursor();
    }

};

class ImportLocalDatabaseCommand : public Command {
public:
    static inline const std::string cname = "import_local_database";
    static inline const std::string hname = "Import the data from a local.db file";
    ImportLocalDatabaseCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::wstring file_path = select_any_existing_file_name();
        bool success = widget->import_local_database(file_path);
        if (success) {
            show_error_message(L"Import successful");
        }
    }

    bool requires_document() { return false; }
};

class ImportSharedDatabaseCommand : public Command {
public:
    static inline const std::string cname = "import_shared_database";
    static inline const std::string hname = "Import the data from a shared.db file";
    ImportSharedDatabaseCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::wstring file_path = select_any_existing_file_name();
        bool success = widget->import_shared_database(file_path);
        if (success) {
            show_error_message(L"Import successful");
        }
    }

    bool requires_document() { return false; }
};

class ImportCommand : public Command {
public:
    static inline const std::string cname = "import";
    static inline const std::string hname = "Import annotation data from a json file";
    ImportCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        std::wstring import_file_name = select_json_file_name();
        widget->import_json(import_file_name);
    }

    bool requires_document() { return false; }
};

class ExportCommand : public Command {
public:
    static inline const std::string cname = "export";
    static inline const std::string hname = "Export annotation data to a json file";
    std::optional<std::wstring> file_name = {};
    ExportCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (file_name.has_value()) return {};
        return Requirement{ RequirementType::File, "Json file" };
    }

    void set_file_requirement(std::wstring value) {
        file_name = value;
    }

    void perform() {
        widget->export_json(file_name.value());
    }

    bool requires_document() { return false; }
};

class WriteAnnotationsFileCommand : public Command {
public:
    static inline const std::string cname = "write_annotations_file";
    static inline const std::string hname = "";
    WriteAnnotationsFileCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->persist_annotations(true);
    }

};

class LoadAnnotationsFileCommand : public Command {
public:
    static inline const std::string cname = "load_annotations_file";
    static inline const std::string hname = "";
    LoadAnnotationsFileCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->load_annotations();
    }

};

class OpenLocalDatabaseContainingFolder : public Command {
public:
    static inline const std::string cname = "open_local_database_containing_folder";
    static inline const std::string hname = "";
    OpenLocalDatabaseContainingFolder(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QFileInfo file_info(QString::fromStdWString(local_database_file_path.get_path()));
        QDir directory = file_info.dir();
        QDesktopServices::openUrl(directory.absolutePath());
    }

};

class OpenSharedDatabaseContainingFolder : public Command {
public:
    static inline const std::string cname = "open_shared_database_containing_folder";
    static inline const std::string hname = "";
    OpenSharedDatabaseContainingFolder(MainWidget* w) : Command(cname, w) {};

    void perform() {
        QFileInfo file_info(QString::fromStdWString(global_database_file_path.get_path()));
        QDir directory = file_info.dir();
        QDesktopServices::openUrl(directory.absolutePath());
    }

};

class LoadAnnotationsFileSyncDeletedCommand : public Command {
public:
    static inline const std::string cname = "import_annotations_file_sync_deleted";
    static inline const std::string hname = "";
    LoadAnnotationsFileSyncDeletedCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->load_annotations(true);
    }

};

class EnterVisualMarkModeCommand : public Command {
public:
    static inline const std::string cname = "enter_visual_mark_mode";
    static inline const std::string hname = "Enter ruler mode using keyboard";
    EnterVisualMarkModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->visual_mark_under_pos({ widget->width() / 2, widget->height() / 2 });
    }

};

class SetPageOffsetCommand : public TextCommand {
public:
    static inline const std::string cname = "set_page_offset";
    static inline const std::string hname = "Toggle visual scroll mode";
    SetPageOffsetCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        if (is_string_numeric(text.value().c_str()) && text.value().size() < 6) { // make sure the page number is valid
            widget->main_document_view->set_page_offset(std::stoi(text.value().c_str()));
        }
    }
};

class ToggleVisualScrollCommand : public Command {
public:
    static inline const std::string cname = "toggle_visual_scroll";
    static inline const std::string hname = "Toggle visual scroll mode";
    ToggleVisualScrollCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_visual_scroll_mode();
    }
};


class ToggleHorizontalLockCommand : public Command {
public:
    static inline const std::string cname = "toggle_horizontal_scroll_lock";
    static inline const std::string hname = "Toggle horizontal lock";
    ToggleHorizontalLockCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->horizontal_scroll_locked = !widget->horizontal_scroll_locked;
    }

};

class ExecuteCommand : public TextCommand {
public:
    static inline const std::string cname = "execute";
    static inline const std::string hname = "Execute shell command";
    ExecuteCommand(MainWidget* w) : TextCommand(cname, w) {};

    void perform() {
        widget->execute_command(text.value());
    }

    bool requires_document() { return false; }
};

class ImportAnnotationsCommand : public Command {
public:
    static inline const std::string cname = "import_annotations";
    static inline const std::string hname = "Import PDF annotations into sioyek";
    ImportAnnotationsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->doc()->import_annotations();
    }

};

class EmbedAnnotationsCommand : public Command {
public:
    static inline const std::string cname = "embed_annotations";
    static inline const std::string hname = "Embed the annotations into a new PDF file";
    EmbedAnnotationsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<std::wstring> file_path = {};

    virtual void set_file_requirement(std::wstring value) {
        file_path = value;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!file_path.has_value()) {
            Requirement req = { RequirementType::File, "File Path" };
            return req;
        }
        return {};
    }

    void perform() {
        //std::wstring embedded_pdf_file_name = select_new_pdf_file_name();
        if (file_path->size() > 0) {
            widget->main_document_view->get_document()->embed_annotations(file_path.value());
        }
    }
};

class EmbedSelectedAnnotation : public Command {
public:
    static inline const std::string cname = "embed_selected_annotation";
    static inline const std::string hname = "Embed the selected annotation into a PDF annotation.";
    EmbedSelectedAnnotation(MainWidget* w) : Command(cname, w) {};

    void perform() {
        if (dv()->selected_object_index.has_value()) {
            std::string uuid = dv()->selected_object_index->uuid;
            widget->free_renderer_resources_for_current_document();
            widget->doc()->embed_single_annot(uuid);
        }
    }
};

class DeleteAllPDFAnnotations : public Command {
public:
    static inline const std::string cname = "delete_all_pdf_annotations";
    static inline const std::string hname = "Delete all PDF annotations.";
    DeleteAllPDFAnnotations(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int btn_index = show_option_buttons(L"This will irreversibly delete all PDF annotations. Are you sure you want to continue?", { L"No", L"Yes" });

        if (btn_index == 3) {
            widget->free_renderer_resources_for_current_document();
            widget->doc()->delete_pdf_annotations();
        }
    }
};

class DeleteIntersectingPDFAnnotations : public Command {
public:
    static inline const std::string cname = "delete_intersecting_pdf_annotations";
    static inline const std::string hname = "Delete all PDF annotations intersecting with the selected rectangle.";
    DeleteIntersectingPDFAnnotations(MainWidget* w) : Command(cname, w) {};
    std::optional<AbsoluteRect> rect_;

    std::optional<Requirement> next_requirement(MainWidget* widget) override {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Command Rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect rect) override {
        if (rect.x0 > rect.x1) {
            std::swap(rect.x0, rect.x1);
        }
        if (rect.y0 > rect.y1) {
            std::swap(rect.y0, rect.y1);
        }

        rect_ = rect;
    }

    void on_cancel() override {
        widget->set_rect_select_mode(false);
    }

    void perform() override {

        widget->free_renderer_resources_for_current_document();
        widget->doc()->delete_intersecting_annotations(rect_.value());
        widget->set_rect_select_mode(false);
    }
};

class CopyWindowSizeConfigCommand : public Command {
public:
    static inline const std::string cname = "copy_window_size_config";
    static inline const std::string hname = "Copy current window size configuration";
    CopyWindowSizeConfigCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        copy_to_clipboard(widget->get_window_configuration_string());
    }

    bool requires_document() { return false; }
};

class ToggleSelectHighlightCommand : public Command {
public:
    static inline const std::string cname = "toggle_select_highlight";
    static inline const std::string hname = "Toggle select highlight mode";
    ToggleSelectHighlightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->is_select_highlight_mode = !widget->is_select_highlight_mode;
    }

};

class OpenLastDocumentCommand : public Command {
public:
    static inline const std::string cname = "open_last_document";
    static inline const std::string hname = "Switch to previous opened document";
    OpenLastDocumentCommand(MainWidget* w) : Command(cname, w) {};

    bool pushes_state() {
        return true;
    }

    void perform() {
        auto last_opened_file = widget->get_last_opened_file_checksum();
        if (last_opened_file) {
            widget->open_document_with_hash(last_opened_file.value());
        }
    }


    bool requires_document() { return false; }
};

class AddMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "add_marked_data";
    static inline const std::string hname = "";
    AddMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_add_marked_data();
    }

    bool requires_document() { return true; }
};

class UndoMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "undo_marked_data";
    static inline const std::string hname = "[internal]";
    UndoMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_undo_marked_data();
    }

    bool requires_document() { return true; }
};

class GotoRandomPageCommand : public Command {
public:
    static inline const std::string cname = "goto_random_page";
    static inline const std::string hname = "[internal]";
    GotoRandomPageCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_goto_random_page();
    }

    bool requires_document() { return true; }
};

class RemoveMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "remove_marked_data";
    static inline const std::string hname = "[internal]";
    RemoveMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_remove_marked_data();
    }

    bool requires_document() { return true; }
};

class ExportMarkedDataCommand : public Command {
public:
    static inline const std::string cname = "export_marked_data";
    static inline const std::string hname = "[internal]";
    ExportMarkedDataCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_export_marked_data();
    }

    bool requires_document() { return true; }
};

class ToggleStatusbarCommand : public Command {
public:
    static inline const std::string cname = "toggle_statusbar";
    static inline const std::string hname = "Toggle statusbar";
    ToggleStatusbarCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_statusbar();
    }

    bool requires_document() { return false; }
};



class ClearCurrentPageDrawingsCommand : public Command {
public:
    static inline const std::string cname = "clear_current_page_drawings";
    static inline const std::string hname = "";
    ClearCurrentPageDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->clear_current_page_drawings();
    }


    bool requires_document() { return false; }
};

class ClearCurrentDocumentDrawingsCommand : public Command {
public:
    static inline const std::string cname = "clear_current_document_drawings";
    static inline const std::string hname = "";
    ClearCurrentDocumentDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->clear_current_document_drawings();
    }


    bool requires_document() { return false; }
};

class DeleteFreehandDrawingsCommand : public Command {
public:
    static inline const std::string cname = "delete_freehand_drawings";
    static inline const std::string hname = "Delete freehand drawings in selected rectangle";
    DeleteFreehandDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<AbsoluteRect> rect_;
    DrawingMode original_drawing_mode = DrawingMode::None;


    void pre_perform() {
        original_drawing_mode = widget->freehand_drawing_mode;
        widget->freehand_drawing_mode = DrawingMode::None;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Command Rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect rect) {
        if (rect.x0 > rect.x1) {
            std::swap(rect.x0, rect.x1);
        }
        if (rect.y0 > rect.y1) {
            std::swap(rect.y0, rect.y1);
        }

        rect_ = rect;
    }

    void perform() {
        widget->delete_freehand_drawings(rect_.value());
        widget->freehand_drawing_mode = original_drawing_mode;
    }
};

class SelectFreehandDrawingsCommand : public Command {
public:
    static inline const std::string cname = "select_freehand_drawings";
    static inline const std::string hname = "Select freehand drawings";
    SelectFreehandDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<AbsoluteRect> rect_;
    DrawingMode original_drawing_mode = DrawingMode::None;


    void pre_perform() {
        original_drawing_mode = widget->freehand_drawing_mode;
        widget->freehand_drawing_mode = DrawingMode::None;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {

        if (!rect_.has_value()) {
            Requirement req = { RequirementType::Rect, "Command Rect" };
            return req;
        }
        return {};
    }

    void set_rect_requirement(AbsoluteRect rect) {
        if (rect.x0 > rect.x1) {
            std::swap(rect.x0, rect.x1);
        }
        if (rect.y0 > rect.y1) {
            std::swap(rect.y0, rect.y1);
        }

        rect_ = rect;
    }

    void perform() {
        widget->select_freehand_drawings(rect_.value());
        widget->freehand_drawing_mode = original_drawing_mode;
    }
};

class CustomCommand : public Command {

    std::wstring raw_command;
    std::string name;
    std::optional<AbsoluteRect> command_rect;
    std::optional<AbsoluteDocumentPos> command_point;
    std::optional<std::wstring> command_text;

public:

    CustomCommand(MainWidget* widget_, std::string name_, std::wstring command_) : Command(name_, widget_) {
        raw_command = command_;
        name = name_;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
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

    void set_rect_requirement(AbsoluteRect rect) {
        command_rect = rect;
    }

    void set_point_requirement(AbsoluteDocumentPos point) {
        command_point = point;
    }

    void set_text_requirement(std::wstring txt) {
        command_text = txt;
    }

    void perform() {
        widget->execute_command(raw_command, command_text.value_or(L""));
    }

    std::string get_name() {
        return name;
    }


};


//class ToggleConfigCommand : public Command {
//    std::string config_name;
//public:
//    ToggleConfigCommand(MainWidget* widget, std::string config_name_) : Command("toggleconfig_" + config_name_, widget) {
//        config_name = config_name_;
//    }
//
//    void perform() {
//        //widget->config_manager->deserialize_config(config_name, text.value());
//        auto conf = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));
//        *(bool*)conf->value = !*(bool*)conf->value;
//        widget->on_config_changed(config_name);
//    }
//
//    bool requires_document() { return false; }
//};
//
//class SaveConfigCommand : public Command {
//    std::string config_name;
//public:
//    SaveConfigCommand(MainWidget* widget, std::string config_name_) : Command("saveconfig_" + config_name_, widget) {
//        config_name = config_name_;
//    }
//
//    void perform() {
//        auto conf = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));
//        conf->is_auto = true;
//        widget->save_auto_config();
//    }
//
//    bool requires_document() { return false; }
//};
//
//
//class DeleteConfigCommand : public Command {
//    std::string config_name;
//public:
//    DeleteConfigCommand(MainWidget* widget, std::string config_name_) : Command("deleteconfig_" + config_name_, widget) {
//        config_name = config_name_;
//    }
//
//    void perform() {
//        auto conf = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));
//        conf->is_auto = false;
//        widget->save_auto_config();
//    }
//
//    bool requires_document() { return false; }
//};


//class ConfigCommand : public Command {
//    std::string config_name;
//    std::optional<std::wstring> text = {};
//    ConfigManager* config_manager;
//    bool save_after_set = false;
//    bool force_touch = false;
//
//public:
//    ConfigCommand(
//        MainWidget* widget_,
//        std::string config_name_,
//        ConfigManager* config_manager_,
//        bool save_after_set_ = false,
//        bool force_touch_=false) :
//        Command((save_after_set_ ? "setsaveconfig_" : "setconfig_") + config_name_, widget_), config_manager(config_manager_) {
//
//        save_after_set = save_after_set_;
//        config_name = config_name_;
//        force_touch = force_touch_;
//    }
//
//    void set_text_requirement(std::wstring value) {
//        text = value;
//    }
//
//    void set_file_requirement(std::wstring value) {
//        text = value;
//    }
//
//    std::wstring get_text_default_value() {
//        return config_manager->get_config_value_string(utf8_decode(config_name));
//    }
//
//    std::optional<Requirement> next_requirement(MainWidget* widget) {
//        if (TOUCH_MODE || force_touch) {
//            Config* config = config_manager->get_mut_config_with_name(utf8_decode(config_name));
//            if (config == nullptr) return {};
//            if ((!text.has_value()) && config->config_type == ConfigType::String) {
//                Requirement res;
//                res.type = RequirementType::Text;
//                res.name = "Config Value";
//                return res;
//            }
//            else if ((!text.has_value()) && config->config_type == ConfigType::FilePath) {
//                Requirement res;
//                res.type = RequirementType::File;
//                res.name = "Config Value";
//                return res;
//            }
//            else if ((!text.has_value()) && config->config_type == ConfigType::FolderPath) {
//                Requirement res;
//                res.type = RequirementType::Folder;
//                res.name = "Config Value";
//                return res;
//            }
//            return {};
//        }
//        else {
//            if (text.has_value()) {
//                return {};
//            }
//            else {
//
//                Requirement res;
//                res.type = RequirementType::Text;
//                res.name = "Config Value";
//                return res;
//            }
//        }
//    }
//
//    std::optional<std::wstring> get_text_suggestion(int index) {
//        const Config* config = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));
//        if (config->config_type == ConfigType::Enum) {
//            const EnumExtras& extras = std::get<EnumExtras>(config->extras);
//            if (extras.possible_values.size() > 0) {
//                index = index % extras.possible_values.size();
//                return extras.possible_values[index];
//            }
//        }
//
//        if (config->name == L"tts_voice") {
//            auto voices = widget->get_tts()->get_available_voices();
//            if (voices.size() > 0) {
//                index = index % voices.size();
//                return voices[index];
//            }
//        }
//
//        return {};
//    }
//
//    void perform() {
//        if (SHOW_SETCONFIG_IN_STATUSBAR) {
//            widget->set_status_message(utf8_decode(config_name) + L" = '" + text.value() + L"'");
//        }
//
//        if (TOUCH_MODE || force_touch) {
//            Config* config = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));
//            if (config == nullptr) return;
//
//
//            if (config->config_type == ConfigType::String || config->config_type == ConfigType::FilePath || config->config_type == ConfigType::FolderPath) {
//                if (widget->config_manager->deserialize_config(config_name, text.value())) {
//                    widget->on_config_changed(config_name);
//                }
//            }
//            if (config->config_type == ConfigType::Macro) {
//                widget->push_current_widget(new MacroConfigUI(config_name, widget, (std::wstring*)config->value, *(std::wstring*)config->value));
//                widget->show_current_widget();
//            }
//            if (config->config_type == ConfigType::Color3) {
//                widget->push_current_widget(new Color3ConfigUI(config_name, widget, (float*)config->value), false);
//                widget->show_current_widget();
//            }
//            if (config->config_type == ConfigType::Enum) {
//                int current_value = *(int*)config->value;
//                std::vector<std::wstring> possible_values = std::get<EnumExtras>(config->extras).possible_values;
//
//                widget->push_current_widget(new EnumConfigUI(config_name, widget, possible_values, current_value));
//                widget->show_current_widget();
//            }
//
//            if (config->config_type == ConfigType::Color4) {
//                widget->push_current_widget(new Color4ConfigUI(config_name, widget, (float*)config->value));
//                widget->show_current_widget();
//            }
//            if (config->config_type == ConfigType::Bool) {
//                widget->push_current_widget(new BoolConfigUI(config_name, widget, (bool*)config->value, QString::fromStdWString(config->name)));
//                widget->show_current_widget();
//            }
//            if (config->config_type == ConfigType::EnableRectangle) {
//                widget->push_current_widget(new RectangleConfigUI(config_name, widget, (UIRect*)config->value));
//                widget->show_current_widget();
//            }
//            if (config->config_type == ConfigType::Float) {
//                FloatExtras extras = std::get<FloatExtras>(config->extras);
//                widget->push_current_widget(new FloatConfigUI(config_name, widget, (float*)config->value, extras.min_val, extras.max_val));
//                widget->show_current_widget();
//
//            }
//            if (config->config_type == ConfigType::Int) {
//                IntExtras extras = std::get<IntExtras>(config->extras);
//                widget->push_current_widget(new IntConfigUI(config_name, widget, (int*)config->value, extras.min_val, extras.max_val));
//                widget->show_current_widget();
//            }
//
//            //        config->serialize
//        }
//        else {
//
//
//            if (text.value().size() > 1) {
//                if (text.value().substr(0, 2) == L"+=" || text.value().substr(0, 2) == L"-=") {
//                    std::wstring config_name_encoded = utf8_decode(config_name);
//                    Config* config_mut = widget->config_manager->get_mut_config_with_name(config_name_encoded);
//                    ConfigType config_type = config_mut->config_type;
//
//                    if (config_type == ConfigType::Int) {
//                        int mult = text.value()[0] == '+' ? 1 : -1;
//                        int* config_ptr = (int*)config_mut->value;
//                        int prev_value = *config_ptr;
//                        int new_value = QString::fromStdWString(text.value()).right(text.value().size() - 2).toInt();
//                        *config_ptr += mult * new_value;
//                        widget->on_config_changed(config_name, save_after_set);
//                        return;
//                    }
//
//                    if (config_type == ConfigType::Float) {
//                        float mult = text.value()[0] == '+' ? 1 : -1;
//                        float* config_ptr = (float*)config_mut->value;
//                        float prev_value = *config_ptr;
//                        float new_value = QString::fromStdWString(text.value()).right(text.value().size() - 2).toFloat();
//                        *config_ptr += mult * new_value;
//                        widget->on_config_changed(config_name, save_after_set);
//                        return;
//                    }
//                }
//            }
//            if (widget->config_manager->deserialize_config(config_name, text.value())) {
//                widget->on_config_changed(config_name, save_after_set);
//            }
//        }
//    }
//
//    bool requires_document() { return false; }
//};

class ShowTouchConfigCommand : public Command {
    std::wstring config_name;
    std::optional<std::wstring> text = {};
    //bool save_after_set = false;
    //bool force_touch = false;

public:
    static inline const std::string cname = "show_touch_ui_for_config";
    static inline const std::string hname = "Show the touch UI for the given configuration.";
    ShowTouchConfigCommand(MainWidget* widget_) :Command(cname, widget_) {
    }

    void set_text_requirement(std::wstring value) {
        if (config_name.size() == 0) {
            config_name = value;
        }
        else {
            text = value;
        }
    }

    void set_file_requirement(std::wstring value) {
        text = value;
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (config_name.size() == 0) {
            Requirement res;
            res.type = RequirementType::Text;
            res.name = "Config Name";
            return res;
        }

        Config* config = widget->config_manager->get_mut_config_with_name(config_name);
        if (config == nullptr) return {};
        if ((!text.has_value()) && config->config_type == ConfigType::String) {
            Requirement res;
            res.type = RequirementType::Text;
            res.name = "Config Value";
            return res;
        }
        else if ((!text.has_value()) && config->config_type == ConfigType::FilePath) {
            Requirement res;
            res.type = RequirementType::File;
            res.name = "Config Value";
            return res;
        }
        else if ((!text.has_value()) && config->config_type == ConfigType::FolderPath) {
            Requirement res;
            res.type = RequirementType::Folder;
            res.name = "Config Value";
            return res;
        }
        return {};
    }

    void perform() {

        Config* config = widget->config_manager->get_mut_config_with_name(config_name);
        if (config == nullptr) return;


        std::string confname = utf8_encode(config_name);
        if (config->config_type == ConfigType::String || config->config_type == ConfigType::FilePath || config->config_type == ConfigType::FolderPath) {
            if (widget->config_manager->deserialize_config(confname, text.value())) {
                widget->on_config_changed(confname);
            }
        }
        if (config->config_type == ConfigType::Macro) {
            widget->push_current_widget(new MacroConfigUI(confname, widget, (std::wstring*)config->value, *(std::wstring*)config->value));
            widget->show_current_widget();
        }
        if (config->config_type == ConfigType::Color3) {
            widget->push_current_widget(new Color3ConfigUI(confname, widget, (float*)config->value), false);
            widget->show_current_widget();
        }
        if (config->config_type == ConfigType::Enum) {
            int current_value = *(int*)config->value;
            std::vector<std::wstring> possible_values = std::get<EnumExtras>(config->extras).possible_values;

            widget->push_current_widget(new EnumConfigUI(confname, widget, possible_values, current_value));
            widget->show_current_widget();
        }

        if (config->config_type == ConfigType::Color4) {
            widget->push_current_widget(new Color4ConfigUI(confname, widget, (float*)config->value));
            widget->show_current_widget();
        }
        if (config->config_type == ConfigType::Bool) {
            widget->push_current_widget(new BoolConfigUI(confname, widget, (bool*)config->value, QString::fromStdWString(config->name)));
            widget->show_current_widget();
        }
        if (config->config_type == ConfigType::EnableRectangle) {
            widget->push_current_widget(new RectangleConfigUI(confname, widget, (UIRect*)config->value));
            widget->show_current_widget();
        }
        if (config->config_type == ConfigType::Float) {
            FloatExtras extras = std::get<FloatExtras>(config->extras);
            widget->push_current_widget(new FloatConfigUI(confname, widget, (float*)config->value, extras.min_val, extras.max_val));
            widget->show_current_widget();

        }
        if (config->config_type == ConfigType::Int) {
            IntExtras extras = std::get<IntExtras>(config->extras);
            widget->push_current_widget(new IntConfigUI(confname, widget, (int*)config->value, extras.min_val, extras.max_val));
            widget->show_current_widget();
        }

        //        config->serialize
    }

    bool requires_document() { return false; }
};

//class ShowTouchConfigCommand : public TextCommand {
//public:
//    static inline const std::string cname = "show_touch_ui_for_config";
//    static inline const std::string hname = "Show the touch config UI for the given config name.";
//    static inline const bool developer_only = true;
//
//    ShowTouchConfigCommand(MainWidget* w) : TextCommand(cname, w) {
//    };
//
//    void perform() {
//        //auto cmd = std::make_unique<ConfigCommand>(widget, utf8_encode(text.value()), widget->config_manager, false, true);
//        //widget->handle_command_types(std::move(cmd), 0);
//    }
//
//    std::string text_requirement_name() {
//        return "Config Name";
//    }
//
//};


class HoldableCommand : public Command {
    std::unique_ptr<Command> down_command = {};
    std::unique_ptr<Command> up_command = {};
    std::unique_ptr<Command> hold_command = {};
    std::string name;
    CommandManager* command_manager;

public:
    HoldableCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(name_, widget_) {
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

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        // holdable commands can't have requirements
        return {};
    }

    bool requires_document() {
        return true;
    }

    void perform() {
        down_command->run();
    }

    void perform_up() {
        up_command->run();
    }

    void on_key_hold() {
        if (hold_command) {
            hold_command->run();
        }
    }


    bool is_holdable() {
        return true;
    }


};

template<typename T>
void register_command(CommandManager* manager, std::string alias_name="") {
    bool is_developer_mode = false;

#ifdef SIOYEK_DEVELOPER
    is_developer_mode = true;
#endif

    if (is_developer_mode || !T::developer_only) {
        std::string name = alias_name.size() > 0 ? alias_name : T::cname;
        bool is_alias = alias_name.size() > 0;
        manager->new_commands[T::cname] = [](MainWidget* widget) {return std::make_unique<T>(widget); };
        if (is_alias) {
            manager->new_commands[name] = [](MainWidget* widget) {return std::make_unique<T>(widget); };
            manager->command_aliases[T::cname] = name;
        }
        manager->command_human_readable_names[name] = is_alias ? "alias for " + T::cname : T::hname;
        manager->command_required_prefixes[QString::fromStdString(name)] = "";
    }
}

CommandManager::CommandManager(ConfigManager* config_manager) {

    register_command<CloseWindowCommand>(this, "q");
    register_command<GotoBeginningCommand>(this, "goto_beginning");
    register_command<GotoEndCommand>(this);
    register_command<GotoDefinitionCommand>(this);
    register_command<OverviewDefinitionCommand>(this);
    register_command<PortalToDefinitionCommand>(this);
    register_command<GotoLoadedDocumentCommand>(this);
    register_command<NextItemCommand>(this);
    register_command<PrevItemCommand>(this);
    register_command<ToggleTextMarkCommand>(this, "toggle_line_select_cursor");
    register_command<MoveTextMarkForwardCommand>(this);
    register_command<MoveTextMarkBackwardCommand>(this);
    register_command<MoveTextMarkForwardWordCommand>(this);
    register_command<MoveTextMarkBackwardWordCommand>(this);
    register_command<MoveTextMarkDownCommand>(this);
    register_command<MoveTextMarkUpCommand>(this);
    register_command<SetMark>(this);
    register_command<SendSymbolCommand>(this);
    register_command<ToggleDrawingMask>(this);
    register_command<TurnOnAllDrawings>(this);
    register_command<TurnOffAllDrawings>(this);
    register_command<GotoMark>(this);
    register_command<GotoPageWithPageNumberCommand>(this);
    register_command<EditSelectedBookmarkCommand>(this);
    register_command<EditSelectedHighlightCommand>(this);
    register_command<SearchCommand>(this);
    register_command<FuzzySearchCommand>(this);
    register_command<SemanticSearchCommand>(this);
    register_command<SemanticSearchExtractiveCommand>(this);
    register_command<DownloadPaperWithUrlCommand>(this);
    register_command<DownloadClipboardUrlCommand>(this);
    register_command<TypeTextCommand>(this);
    register_command<ExecuteMacroCommand>(this);
    register_command<ControlMenuCommand>(this);
    register_command<SetViewStateCommand>(this);
    register_command<LlmCommand>(this);
    register_command<GetConfigCommand>(this);
    register_command<GetConfigNoDialogCommand>(this);
    register_command<ShowOptionsCommand>(this);
    register_command<ShowTextPromptCommand>(this);
    register_command<GetStateJsonCommand>(this);
    register_command<GetModeString>(this);
    register_command<GetPaperNameCommand>(this);
    register_command<GetOverviewPaperName>(this);
    register_command<GetAnnotationsJsonCommand>(this);
    register_command<ToggleRectHintsCommand>(this);
    register_command<AddAnnotationToHighlightCommand>(this);
    register_command<ChangeHighlightTypeCommand>(this);
    register_command<DeteteVisibleItem>(this);
    register_command<SelectVisibleItem>(this);
    register_command<RenameCommand>(this);
    register_command<SetFreehandThickness>(this);
    register_command<GotoPageWithLabel>(this);
    register_command<RegexSearchCommand>(this);
    register_command<ChapterSearchCommand>(this);
    register_command<ToggleTwoPageModeCommand>(this);
    register_command<FitEpubToWindowCommand>(this);
    register_command<MoveDownCommand>(this);
    register_command<MoveUpCommand>(this);
    register_command<MoveLeftCommand>(this);
    register_command<MoveRightCommand>(this);
    register_command<MoveDownSmoothCommand>(this);
    register_command<MoveUpSmoothCommand>(this);
    register_command<MoveLeftInOverviewCommand>(this);
    register_command<MoveRightInOverviewCommand>(this);
    register_command<SaveScratchpadCommand>(this);
    register_command<LoadScratchpadCommand>(this);
    register_command<ClearScratchpadCommand>(this);
    register_command<ZoomInCommand>(this);
    register_command<ZoomOutCommand>(this);
    register_command<ZoomInOverviewCommand>(this);
    register_command<ZoomOutOverviewCommand>(this);
    register_command<FitToPageWidthCommand>(this);
    register_command<FitToOverviewWidth>(this);
    register_command<FitToPageHeightCommand>(this);
    register_command<FitToPageSmartCommand>(this);
    register_command<FitToPageHeightSmartCommand>(this);
    register_command<FitToPageWidthSmartCommand>(this);
    register_command<NextPageCommand>(this);
    register_command<PreviousPageCommand>(this);
    register_command<OpenDocumentCommand>(this);
    register_command<OpenServerOnlyFile>(this);
    register_command<ScreenshotCommand>(this);
    register_command<FramebufferScreenshotCommand>(this);
    register_command<WaitCommand>(this);
    register_command<WaitForRendersToFinishCommand>(this);
    register_command<WaitForSearchToFinishCommand>(this);
    register_command<WaitForDownloadsToFinish>(this);
    register_command<WaitForIndexingToFinishCommand>(this);
    register_command<AddBookmarkCommand>(this);
    register_command<AddBookmarkMarkedCommand>(this);
    register_command<AddBookmarkFreetextCommand>(this);
    register_command<AddFreetextBookmarkAutoCommand>(this);
    register_command<CopyDrawingsFromScratchpadCommand>(this);
    register_command<CopyScreenshotToScratchpad>(this);
    register_command<ConvertToLatexCommand>(this);
    register_command<ExtractTableWithPromptCommand>(this);
    register_command<ExtractTableCommand>(this);
    register_command<CopyScreenshotToClipboard>(this);
    register_command<AddHighlightCommand>(this);
    register_command<GotoTableOfContentsCommand>(this);
    register_command<GotoHighlightCommand>(this);
    register_command<IncreaseFreetextBookmarkFontSizeCommand>(this);
    register_command<DecreaseFreetextBookmarkFontSizeCommand>(this);
    register_command<GotoPortalListCommand>(this);
    register_command<GotoBookmarkCommand>(this);
    register_command<GotoBookmarkGlobalCommand>(this);
    register_command<GotoHighlightGlobalCommand>(this);
    register_command<PortalCommand>(this);
    register_command<CreateVisiblePortalCommand>(this);
    register_command<PinOverviewAsPortalCommand>(this);
    register_command<NextStateCommand>(this);
    register_command<PrevStateCommand>(this);
    register_command<DeletePortalCommand>(this);
    register_command<DeleteBookmarkCommand>(this);
    register_command<DeleteHighlightCommand>(this);
    register_command<DeleteVisibleBookmarkCommand>(this);
    register_command<EditVisibleBookmarkCommand>(this);
    register_command<GotoPortalCommand>(this);
    register_command<EditPortalCommand>(this);
    register_command<OpenPrevDocCommand>(this);
    register_command<OpenAllDocsCommand>(this);
    register_command<OpenDocumentEmbeddedCommand>(this);
    register_command<OpenDocumentEmbeddedFromCurrentPathCommand>(this);
    register_command<CopyCommand>(this);
    register_command<CollapseMenuCommand>(this);
    register_command<ToggleFullscreenCommand>(this);
    register_command<MaximizeCommand>(this);
    register_command<ToggleOneWindowCommand>(this);
    register_command<ToggleHighlightCommand>(this);
    register_command<ToggleSynctexCommand>(this);
    register_command<TurnOnSynctexCommand>(this);
    register_command<ToggleShowLastCommand>(this);
    register_command<ForwardSearchCommand>(this);
    register_command<CommandCommand>(this);
    register_command<CommandPaletteCommand>(this);
    register_command<RegisterUrlHandler>(this);
    register_command<UnregisterUrlHandler>(this);
    register_command<ExternalSearchCommand>(this);
    register_command<OpenSelectedUrlCommand>(this);
    register_command<ScreenDownCommand>(this);
    register_command<ScreenUpCommand>(this);
    register_command<NextChapterCommand>(this);
    register_command<PrevChapterCommand>(this);
    register_command<ShowContextMenuCommand>(this);
    register_command<ShowCustomContextMenuCommand>(this);
    register_command<ToggleDarkModeCommand>(this);
    register_command<OpenExternalTextEditorCommand>(this);
    register_command<TogglePresentationModeCommand>(this);
    register_command<TurnOnPresentationModeCommand>(this);
    register_command<ToggleMouseDragMode>(this);
    register_command<ToggleFreehandDrawingMode>(this);
    register_command<TogglePenDrawingMode>(this);
    register_command<ToggleScratchpadMode>(this);
    register_command<CloseWindowCommand>(this);
    register_command<QuitCommand>(this);
    register_command<EscapeCommand>(this);
    register_command<TogglePDFAnnotationsCommand>(this);
    register_command<CloseWindowCommand>(this);
    register_command<OpenLinkCommand>(this);
    register_command<OverviewLinkCommand>(this);
    register_command<DownloadLinkCommand>(this);
    register_command<KeyboardSelectLineCommand>(this);
    register_command<FindReferencesCommand>(this);
    register_command<PortalToLinkCommand>(this);
    register_command<ShowTouchConfigCommand>(this);
    register_command<CopyLinkCommand>(this);
    register_command<KeyboardSelectCommand>(this);
    register_command<KeyboardSmartjumpCommand>(this);
    register_command<KeyboardOverviewCommand>(this);
    register_command<KeysCommand>(this);
    register_command<KeysUserCommand>(this);
    register_command<PrefsCommand>(this);
    register_command<PrefsUserCommand>(this);
    register_command<MoveVisualMarkDownCommand>(this, "move_ruler_down");
    register_command<MoveVisualMarkUpCommand>(this, "move_ruler_up");
    //register_command<MoveVisualMarkDownCommand>(this);
    //register_command<MoveVisualMarkUpCommand>(this);
    register_command<MoveRulerToNextBlockCommand>(this);
    register_command<MoveRulerToPrevBlockCommand>(this);
    register_command<MoveVisualMarkNextCommand>(this, "move_ruler_next");
    register_command<MoveVisualMarkPrevCommand>(this, "move_ruler_prev");
    register_command<ToggleCustomColorMode>(this);
    register_command<SetSelectHighlightTypeCommand>(this);
    register_command<SetFreehandType>(this);
    register_command<SetFreehandAlphaCommand>(this);
    register_command<ToggleWindowConfigurationCommand>(this);
    register_command<PrefsUserAllCommand>(this);
    register_command<KeysUserAllCommand>(this);
    register_command<FitToPageWidthRatioCommand>(this);
    register_command<SmartJumpUnderCursorCommand>(this);
    register_command<DownloadPaperUnderCursorCommand>(this);
    register_command<OpenContainingFolderCommand>(this);
    register_command<DownloadPaperWithNameCommand>(this);
    register_command<OverviewUnderCursorCommand>(this);
    register_command<CloseOverviewCommand>(this);
    register_command<VisualMarkUnderCursorCommand>(this, "ruler_under_cursor");
    register_command<RulerUnderSelectedPointCommand>(this);
    register_command<StartMobileTextSelectionAtPointCommand>(this);
    register_command<CloseVisualMarkCommand>(this, "exit_ruler_mode");
    register_command<ZoomInCursorCommand>(this);
    register_command<ZoomOutCursorCommand>(this);
    //register_command<MoveWindowCommand>(this);
    register_command<GotoLeftCommand>(this);
    register_command<GotoLeftSmartCommand>(this);
    register_command<GotoRightCommand>(this);
    register_command<GotoRightSmartCommand>(this);
    register_command<RotateClockwiseCommand>(this);
    register_command<RotateCounterClockwiseCommand>(this);
    register_command<GotoNextHighlightCommand>(this);
    register_command<GotoPrevHighlightCommand>(this);
    register_command<GotoNextHighlightOfTypeCommand>(this);
    register_command<GotoPrevHighlightOfTypeCommand>(this);
    register_command<AddHighlightWithCurrentTypeCommand>(this);
    register_command<UndoDrawingCommand>(this);
    register_command<EnterPasswordCommand>(this);
    register_command<ToggleFastreadCommand>(this);
    register_command<GotoTopOfPageCommand>(this);
    register_command<GotoBottomOfPageCommand>(this);
    register_command<NewWindowCommand>(this);
    register_command<ReloadCommand>(this);
    register_command<ReloadNoFlickerCommand>(this);
    register_command<ReloadConfigCommand>(this);
    register_command<SynctexUnderCursorCommand>(this);
    register_command<SynctexUnderRulerCommand>(this);
    register_command<SetStatusStringCommand>(this);
    register_command<ClearStatusStringCommand>(this);
    register_command<ToggleTittlebarCommand>(this);
    register_command<NextPreviewCommand>(this);
    register_command<PreviousPreviewCommand>(this);
    register_command<GotoOverviewCommand>(this);
    register_command<PortalToOverviewCommand>(this);
    register_command<GotoSelectedTextCommand>(this);
    register_command<FocusTextCommand>(this);
    register_command<DownloadOverviewPaperCommand>(this);
    register_command<DownloadOverviewPaperNoPrompt>(this);
    register_command<GotoWindowCommand>(this);
    register_command<ToggleSmoothScrollModeCommand>(this); // todo: this probably should be a config?
    register_command<ToggleScrollbarCommand>(this);
    register_command<OverviewToPortalCommand>(this);
    register_command<OverviewRulerPortalCommand>(this);
    register_command<GotoRulerPortalCommand>(this);
    register_command<SelectRectCommand>(this);
    register_command<ToggleTypingModeCommand>(this);
    register_command<DonateCommand>(this);
    register_command<OverviewNextItemCommand>(this);
    register_command<OverviewPrevItemCommand>(this);
    register_command<DeleteHighlightUnderCursorCommand>(this); // todo: delete generic item instead of highlight?
    register_command<NoopCommand>(this);
    register_command<ImportCommand>(this);
    register_command<ImportLocalDatabaseCommand>(this);
    register_command<ImportSharedDatabaseCommand>(this);
    register_command<ExportCommand>(this);
    register_command<WriteAnnotationsFileCommand>(this);
    register_command<LoadAnnotationsFileCommand>(this);
    register_command<OpenLocalDatabaseContainingFolder>(this);
    register_command<OpenSharedDatabaseContainingFolder>(this);
    register_command<LoadAnnotationsFileSyncDeletedCommand>(this);
    register_command<EnterVisualMarkModeCommand>(this);
    register_command<SetPageOffsetCommand>(this);
    register_command<ToggleVisualScrollCommand>(this, "toggle_ruler_scroll_mode");
    register_command<ToggleHorizontalLockCommand>(this);
    register_command<ExecuteCommand>(this, "execute_shell_command");
    register_command<EmbedAnnotationsCommand>(this);
    register_command<EmbedSelectedAnnotation>(this);
    register_command<DeleteAllPDFAnnotations>(this);
    register_command<DeleteIntersectingPDFAnnotations>(this);
    register_command<ImportAnnotationsCommand>(this);
    register_command<CopyWindowSizeConfigCommand>(this);
    register_command<ToggleSelectHighlightCommand>(this);
    register_command<OpenLastDocumentCommand>(this);
    register_command<ToggleStatusbarCommand>(this);
    register_command<StartReadingCommand>(this);
    register_command<StartReadingHighQualityCommand>(this);
    register_command<StopReadingCommand>(this);
    register_command<IncreaseTtsRateCommand>(this);
    register_command<DecreaseTtsRateCommand>(this);
    register_command<ToggleReadingCommand>(this);
    register_command<AddKeybindCommand>(this);
    register_command<ScanNewFilesFromScanDirCommand>(this);
    register_command<AddMarkedDataCommand>(this);
    register_command<RemoveMarkedDataCommand>(this);
    register_command<ExportMarkedDataCommand>(this);
    register_command<UndoMarkedDataCommand>(this);
    register_command<GotoRandomPageCommand>(this);
    register_command<ClearCurrentPageDrawingsCommand>(this);
    register_command<ClearCurrentDocumentDrawingsCommand>(this);
    register_command<DeleteFreehandDrawingsCommand>(this);
    register_command<SelectFreehandDrawingsCommand>(this);
    register_command<SelectCurrentSearchMatchCommand>(this);
    register_command<SelectRulerTextCommand>(this);
    register_command<ShowTouchMainMenu>(this);
    register_command<ShowTouchPageSelectCommand>(this);
    register_command<ShowTouchHighlightTypeSelectCommand>(this);
    register_command<ShowTouchSettingsMenu>(this);
    register_command<ShowTouchDrawingMenu>(this);
    register_command<DebugCommand>(this);
    register_command<UndoDeleteCommand>(this);
    register_command<SetConfigCommand>(this);
    register_command<ToggleConfigWithNameCommand>(this);
    register_command<SaveConfigWithNameCommand>(this);
    register_command<FulltextSearchCommand>(this);
    register_command<ShowCurrentDocumentFulltextTags>(this);
    register_command<FulltextSearchCommandWithTag>(this);
    register_command<DocumentationSearchCommand>(this);
    register_command<FulltextSearchCurrentDocumentCommand>(this);
    register_command<DeleteDocumentFromFulltextSearchIndex>(this);
    register_command<CreateFulltextIndexForCurrentDocumentCommand>(this);
    register_command<CreateFulltextIndexForCurrentDocumentCommandWithTag>(this);
    register_command<ExportPythonApiCommand>(this);
    register_command<ScrollSelectedBookmarkDown>(this);
    register_command<ScrollSelectedBookmarkUp>(this);
    register_command<ExportDefaultConfigFile>(this);
    register_command<ExportCommandNamesCommand>(this);
    register_command<ExportConfigNamesCommand>(this);
    register_command<TestCommand>(this);
    register_command<PrintUndocumentedCommandsCommand>(this);
    register_command<PrintUndocumentedConfigsCommand>(this);
    register_command<PrintNonDefaultConfigs>(this);
    register_command<SetWindowRectCommand>(this);
    register_command<MoveSelectedBookmarkCommand>(this);
    register_command<ResizePendingBookmark>(this);
    register_command<LoginCommand>(this);
    register_command<LoginWithGoogleCommand>(this);
    register_command<LogoutCommand>(this);
    register_command<CancelAllDownloadsCommand>(this);
    register_command<CitersCommand>(this);
    register_command<ResumeToServerLocationCommand>(this);
    register_command<LoginUsingAccessTokenCommand>(this);
    register_command<SynchronizeCommand>(this);
    register_command<ForceDownloadAnnotations>(this);
    register_command<UploadCurrentFileCommand>(this);
    register_command<DeleteCurrentFileFromServer>(this);
    register_command<ResyncDocumentCommand>(this);
    register_command<DownloadUnsyncedFilesCommand>(this);
    register_command<SyncCurrentFileLocation>(this);
    register_command<ScreenUpSmoothCommand>(this);
    register_command<ScreenDownSmoothCommand>(this);

    for (auto [command_name_, command_value] : ADDITIONAL_COMMANDS) {
        std::string command_name = utf8_encode(command_name_);
        std::wstring local_command_value = command_value.text;
        new_commands[command_name] = [command_name, local_command_value, this](MainWidget* w) {return  std::make_unique<CustomCommand>(w, command_name, local_command_value); };
        command_required_prefixes[QString::fromStdString(command_name)] = "_";
    }


    for (auto [command_name_, js_command_info] : ADDITIONAL_JAVASCRIPT_COMMANDS) {
        handle_new_javascript_command(command_name_, js_command_info, false);
        command_required_prefixes[QString::fromStdWString(command_name_)] = "_";
    }

    for (auto [command_name_, js_command_info] : ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS) {
        handle_new_javascript_command(command_name_, js_command_info, true);
        command_required_prefixes[QString::fromStdWString(command_name_)] = "_";
    }

    for (auto [command_name_, macro_value] : ADDITIONAL_MACROS) {
        std::string command_name = utf8_encode(command_name_);
        std::wstring local_macro_value = macro_value.text;
        new_commands[command_name] = [command_name, local_macro_value, this](MainWidget* w) {return std::make_unique<MacroCommand>(w, this, command_name, local_macro_value); };
        command_required_prefixes[QString::fromStdString(command_name)] = "_";
    }

    std::vector<Config*> configs = config_manager->get_configs();

    //for (auto conf : configs) {

    //    std::string confname = utf8_encode(conf->name);
    //    QString qconfname = QString::fromStdString(confname);
    //    QString extra_prefix = "";
    //    if (qconfname.startsWith("DARK")) {
    //        extra_prefix = "DARK";
    //    }
    //    if (qconfname.startsWith("CUSTOM")) {
    //        extra_prefix = "CUSTOM";
    //    }

    //    std::string config_set_command_name = "setconfig_" + confname;
    //    new_commands[config_set_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ConfigCommand>(w, confname, config_manager); };
    //    command_required_prefixes[QString::fromStdString(config_set_command_name)] = "setconfig_" + extra_prefix;

    //    std::string config_setsave_command_name = "setsaveconfig_" + confname;
    //    new_commands[config_setsave_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ConfigCommand>(w, confname, config_manager, true); };
    //    command_required_prefixes[QString::fromStdString(config_setsave_command_name)] = "setsaveconfig_" + extra_prefix;

    //    std::string config_save_command_name = "saveconfig_" + confname;
    //    new_commands[config_save_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<SaveConfigCommand>(w, confname); };
    //    command_required_prefixes[QString::fromStdString(config_save_command_name)] = "saveconfig_" + extra_prefix;

    //    std::string config_delete_command_name = "deleteconfig_" + confname;
    //    new_commands[config_delete_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<DeleteConfigCommand>(w, confname); };
    //    command_required_prefixes[QString::fromStdString(config_delete_command_name)] = "deleteconfig_" + extra_prefix;

    //    if (conf->config_type == ConfigType::Bool) {
    //        std::string config_toggle_command_name = "toggleconfig_" + confname;
    //        new_commands[config_toggle_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ToggleConfigCommand>(w, confname); };
    //        command_required_prefixes[QString::fromStdString(config_toggle_command_name)] = "toggleconfig_";
    //    }

    //}
    //command_required_prefixes["set_config"] = "setconfig_";
    //command_required_prefixes["toggle_config"] = "toggleconfig_";

    QDateTime current_time = QDateTime::currentDateTime();
    QDateTime older_time = current_time.addSecs(-1);
    for (auto [command_name, _] : new_commands) {
        if (command_name.size() > 0 && command_name[0] == '_') {
            command_last_uses[command_name] = older_time;
        }
        else {
            command_last_uses[command_name] = current_time;
        }
    }

}

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

    if (javascript_file_info.isRelative()) {
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

void print_tree_node(InputParseTreeNode node) {
    if (node.requires_text) {
        std::wcerr << "text node" << std::endl;
        return;
    }
    if (node.requires_symbol) {
        std::wcerr << "symbol node" << std::endl;
        return;
    }

    if (node.control_modifier) {
        std::wcerr << "Ctrl+";
    }

    if (node.command_modifier) {
        std::wcerr << "Cmd+";
    }

    if (node.shift_modifier) {
        std::wcerr << "Shift+";
    }

    if (node.alt_modifier) {
        std::wcerr << "Alt+";
    }
    std::wcerr << node.command << std::endl;
}

InputParseTreeNode parse_token(std::wstring token) {
    InputParseTreeNode res;

    if (token == L"sym") {
        res.requires_symbol = true;
        return res;
    }
    if (token == L"txt") {
        res.requires_text = true;
        return res;
    }

    std::vector<std::wstring> subcommands;
    split_key_string(token, L"-", subcommands);

    for (int i = 0; i < static_cast<int>(subcommands.size()) - 1; i++) {
        if (subcommands[i] == L"C") {
            res.control_modifier = true;
        }

        if (subcommands[i] == L"D") {
            res.command_modifier = true;
        }

        if (subcommands[i] == L"S") {
            res.shift_modifier = true;
        }

        if (subcommands[i] == L"A") {
            res.alt_modifier = true;
        }
    }

    std::wstring command_string = subcommands[subcommands.size() - 1];
    if (command_string.size() == 1) {
        res.command = subcommands[subcommands.size() - 1][0];
    }
    else {

        if (int f_key = get_f_key(command_string)) {
            res.command = Qt::Key::Key_F1 - 1 + f_key;
        }
        else {

            std::map<std::wstring, Qt::Key> keymap_temp = {
                {L"up", Qt::Key::Key_Up},
                {L"down", Qt::Key::Key_Down},
                {L"left", Qt::Key::Key_Left},
                {L"right", Qt::Key::Key_Right},
                {L"backspace", Qt::Key::Key_Backspace},
                {L"space", Qt::Key::Key_Space},
                {L"pageup", Qt::Key::Key_PageUp},
                {L"pagedown", Qt::Key::Key_PageDown},
                {L"home", Qt::Key::Key_Home},
                {L"end", Qt::Key::Key_End},
                {L"pagedown", Qt::Key::Key_End},
                {L"tab", Qt::Key::Key_Tab},
                {L"return", Qt::Key::Key_Return},
            };
            std::map<std::wstring, Qt::Key> keymap;

            for (auto item : keymap_temp) {
                keymap[item.first] = item.second;
                keymap[L"<" + item.first + L">"] = item.second;
            }

            res.command = keymap[command_string];
        }

    }

    return res;
}
void get_tokens(std::wstring line, std::vector<std::wstring>& tokens) {
    std::wstring stack;

    int stack_depth = 0;

    for (wchar_t c : line) {
        if (stack_depth && (c != '>') && (c != '<')) {
            stack.push_back(c);
        }
        else if ((c == '>')) {
            stack_depth--;
            if (stack_depth == 0) {
                tokens.push_back(stack);
                stack.clear();
            }
            else {
                stack.push_back(c);
            }
        }
        else if (c == '<') {
            if (stack_depth) {
                stack.push_back(c);
            }
            stack_depth++;
        }
        else {
            tokens.push_back(std::wstring(1, c));
        }

    }
}

bool is_command_incomplete_macro(const std::vector<std::string>& commands) {

    for (auto com : commands) {
        if (com.find("[") == -1) {
            return false;
        }
        if (com.find("[]") != -1) {
            return false;
        }
    }
    return true;
}

bool parse_line(
    InputParseTreeNode* root,
    CommandManager* command_manager,
    const std::wstring& line,
    const std::wstring& command_string,
    const std::wstring& command_file_name,
    const int& command_line_number) {

    // for example convert "<a-<space>> to ["a", "space"]
    bool has_warning = false;
    std::vector<std::wstring> tokens;
    get_tokens(line, tokens);

    InputParseTreeNode* parent_node = root;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].size() == 0) continue;

        InputParseTreeNode node = parse_token(tokens[i]);
        bool existing_node = false;
        for (InputParseTreeNode* child : parent_node->children) {
            if (child->is_same(&node)) {
                parent_node = child;
                existing_node = true;
                break;
            }
        }
        if (!existing_node) {
            if ((tokens[i] != L"sym") && (tokens[i] != L"txt")) {
                if (parent_node->is_final) {
                    LOG(std::wcerr
                        << L"Warning: key defined in " << command_file_name
                        << L":" << command_line_number
                        << L" for " << command_string
                        << L" is unreachable, shadowed by final key sequence defined in "
                        << parent_node->defining_file_path
                        << L":" << parent_node->defining_file_line << L"\n");
                }
                auto new_node = new InputParseTreeNode(node);
                new_node->defining_file_line = command_line_number;
                new_node->defining_file_path = command_file_name;
                parent_node->children.push_back(new_node);
                parent_node = parent_node->children[parent_node->children.size() - 1];
            }
            else {
                if (tokens[i] == L"sym") {
                    parent_node->requires_symbol = true;
                    parent_node->is_final = true;
                }

                if (tokens[i] == L"txt") {
                    parent_node->requires_text = true;
                    parent_node->is_final = true;
                }
            }
        }
        else if (((size_t)i == (tokens.size() - 1)) &&
            (SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE ||
                (command_file_name.compare(parent_node->defining_file_path)) == 0)) {
            if ((parent_node->name_.size() == 0) || parent_node->name_[0].compare(utf8_encode(command_string)) != 0) {
                if (!is_command_string_modal(command_string)) {

                    has_warning = true;
                    std::wcerr << L"Warning: key defined in " << parent_node->defining_file_path
                        << L":" << parent_node->defining_file_line
                        << L" overwritten by " << command_file_name
                        << L":" << command_line_number;
                    if (parent_node->name_.size() > 0) {
                        std::wcerr << L". Overriding command: " << line
                            << L": replacing " << utf8_decode(parent_node->name_[0])
                            << L" with " << command_string;
                    }
                    std::wcerr << L"\n";
                }
            }
        }
        if ((size_t)i == (tokens.size() - 1)) {
            parent_node->is_final = true;

            QString command_name_qstr = QString::fromStdWString(command_string);
            std::vector<std::string> command_names = parse_command_name(command_name_qstr);
            std::vector<std::string> previous_names = std::move(parent_node->name_);
            parent_node->name_ = {};
            parent_node->defining_file_line = command_line_number;
            parent_node->defining_file_path = command_file_name;
            for (size_t k = 0; k < command_names.size(); k++) {
                parent_node->name_.push_back(command_names[k]);
            }
            if (command_name_qstr.startsWith("{holdable}")) {
                if (command_name_qstr.indexOf("|") == -1) {
                    qDebug() << "Error in " << command_file_name << ":" << command_line_number << ": holdable command " << command_name_qstr << " does not contain a | character";
                }
                else {
                    std::wstring actual_command = command_name_qstr.mid(10).toStdWString();
                    parent_node->generator = [command_manager, actual_command](MainWidget* w) {return std::make_unique<HoldableCommand>(
                        w, command_manager, "", actual_command); };
                }
            }
            else if (command_name_qstr.startsWith("{jscall}")) {
                std::wstring actual_command = command_name_qstr.mid(8).toStdWString();
                parent_node->generator = [command_manager, actual_command](MainWidget* w) {return std::make_unique<JsCallCommand>("", actual_command, w); };

            }
            else if (command_name_qstr.startsWith("{jsasync}")) {
                std::wstring code = L"(" + command_name_qstr.mid(9).toStdWString() + L")()";
                std::optional<std::wstring> entry = {};
                parent_node->generator = [command_manager, entry, code](MainWidget* w) {return std::make_unique<JavascriptCommand>("", code, entry, true, w); };
                //JavascriptCommand(std::string command_name, std::wstring code_, std::optional<std::wstring> entry_point_, bool is_async_, MainWidget* w) :  Command(command_name, w), command_name(command_name) {

            }
            else if (command_names.size() == 1 && (command_names[0].find("[") == -1) && (command_names[0].find("(") == -1)) {
                if (command_manager->new_commands.find(command_names[0]) != command_manager->new_commands.end()) {
                    parent_node->generator = command_manager->new_commands[command_names[0]];
                }
                else if (command_name_qstr.startsWith("_")) {
                    // it is possible that the macro will be defined later (e.g. maybe in .sioyek.js file) so we create
                    // a lazy macro command here
                    parent_node->generator = [single_command=command_names[0], command_manager](MainWidget* w) {return std::make_unique<MacroCommand>(w, command_manager, "", utf8_decode(single_command)); };
                }
                else {
                    has_warning = true;
                    std::wcerr << L"Warning: command " << utf8_decode(command_names[0]) << L" used in " << parent_node->defining_file_path
                        << L":" << parent_node->defining_file_line << L" not found.\n";
                }
            }
            else {
                QStringList command_parts;
                for (int k = 0; k < command_names.size(); k++) {
                    command_parts.append(QString::fromStdString(command_names[k]));
                }

                // is the command incomplete and should be appended to previous command instead of replacing it?
                if (is_command_incomplete_macro(command_names)) {
                    for (int k = 0; k < previous_names.size(); k++) {
                        command_parts.append(QString::fromStdString(previous_names[k]));
                        parent_node->name_.push_back(previous_names[k]);
                    }
                }

                std::wstring joined_command = command_parts.join(";").toStdWString();
                parent_node->generator = [joined_command, command_manager](MainWidget* w) {return std::make_unique<MacroCommand>(w, command_manager, "", joined_command); };
            }
            //if (command_names[j].size())
        }
        else {
            if (SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE && parent_node->is_final && (parent_node->name_.size() > 0)) {
                has_warning = true;
                std::wcerr << L"Warning: unmapping " << utf8_decode(parent_node->name_[0]) << L" because of " << command_string << L" which uses " << line << L"\n";
            }
            parent_node->is_final = false;
        }

    }
    return has_warning;
}

InputParseTreeNode* parse_lines(
    InputParseTreeNode* root,
    CommandManager* command_manager,
    const std::vector<std::wstring>& lines,
    const std::vector<std::wstring>& command_strings,
    const std::vector<std::wstring>& command_file_names,
    const std::vector<int>& command_line_numbers
) {

    for (size_t j = 0; j < lines.size(); j++) {
        parse_line(
            root,
            command_manager,
            lines[j],
            command_strings[j],
            command_file_names[j],
            command_line_numbers[j]
        );
        //std::wstring line = lines[j];

        //// for example convert "<a-<space>> to ["a", "space"]
        //std::vector<std::wstring> tokens;
        //get_tokens(line, tokens);

        //InputParseTreeNode* parent_node = root;

        //for (size_t i = 0; i < tokens.size(); i++) {
        //    InputParseTreeNode node = parse_token(tokens[i]);
        //    bool existing_node = false;
        //    for (InputParseTreeNode* child : parent_node->children) {
        //        if (child->is_same(&node)) {
        //            parent_node = child;
        //            existing_node = true;
        //            break;
        //        }
        //    }
        //    if (!existing_node) {
        //        if ((tokens[i] != L"sym") && (tokens[i] != L"txt")) {
        //            if (parent_node->is_final) {
        //                LOG(std::wcerr
        //                    << L"Warning: key defined in " << command_file_names[j]
        //                    << L":" << command_line_numbers[j]
        //                    << L" for " << command_strings[j]
        //                    << L" is unreachable, shadowed by final key sequence defined in "
        //                    << parent_node->defining_file_path
        //                    << L":" << parent_node->defining_file_line << L"\n");
        //            }
        //            auto new_node = new InputParseTreeNode(node);
        //            new_node->defining_file_line = command_line_numbers[j];
        //            new_node->defining_file_path = command_file_names[j];
        //            parent_node->children.push_back(new_node);
        //            parent_node = parent_node->children[parent_node->children.size() - 1];
        //        }
        //        else {
        //            if (tokens[i] == L"sym") {
        //                parent_node->requires_symbol = true;
        //                parent_node->is_final = true;
        //            }

        //            if (tokens[i] == L"txt") {
        //                parent_node->requires_text = true;
        //                parent_node->is_final = true;
        //            }
        //        }
        //    }
        //    else if (((size_t)i == (tokens.size() - 1)) &&
        //        (SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE ||
        //            (command_file_names[j].compare(parent_node->defining_file_path)) == 0)) {
        //        if ((parent_node->name_.size() == 0) || parent_node->name_[0].compare(utf8_encode(command_strings[j])) != 0) {
        //            if (!is_command_string_modal(command_strings[j])) {

        //                std::wcerr << L"Warning: key defined in " << parent_node->defining_file_path
        //                    << L":" << parent_node->defining_file_line
        //                    << L" overwritten by " << command_file_names[j]
        //                    << L":" << command_line_numbers[j];
        //                if (parent_node->name_.size() > 0) {
        //                    std::wcerr << L". Overriding command: " << line
        //                        << L": replacing " << utf8_decode(parent_node->name_[0])
        //                        << L" with " << command_strings[j];
        //                }
        //                std::wcerr << L"\n";
        //            }
        //        }
        //    }
        //    if ((size_t)i == (tokens.size() - 1)) {
        //        parent_node->is_final = true;

        //        QString command_name_qstr = QString::fromStdWString(command_strings[j]);
        //        std::vector<std::string> command_names = parse_command_name(command_name_qstr);
        //        std::vector<std::string> previous_names = std::move(parent_node->name_);
        //        parent_node->name_ = {};
        //        parent_node->defining_file_line = command_line_numbers[j];
        //        parent_node->defining_file_path = command_file_names[j];
        //        for (size_t k = 0; k < command_names.size(); k++) {
        //            parent_node->name_.push_back(command_names[k]);
        //        }
        //        if (command_name_qstr.startsWith("{holdable}")) {
        //            if (command_name_qstr.indexOf("|") == -1) {
        //                qDebug() << "Error in " << command_file_names[j] << ":" << command_line_numbers[j] << ": holdable command " << command_name_qstr << " does not contain a | character";
        //            }
        //            else {
        //                std::wstring actual_command = command_name_qstr.mid(10).toStdWString();
        //                parent_node->generator = [command_manager, actual_command](MainWidget* w) {return std::make_unique<HoldableCommand>(
        //                    w, command_manager, "", actual_command); };
        //            }
        //        }
        //        else if (command_names.size() == 1 && (command_names[0].find("[") == -1) && (command_names[0].find("(") == -1)) {
        //            if (command_manager->new_commands.find(command_names[0]) != command_manager->new_commands.end()) {
        //                parent_node->generator = command_manager->new_commands[command_names[0]];
        //            }
        //            else {
        //                std::wcerr << L"Warning: command " << utf8_decode(command_names[0]) << L" used in " << parent_node->defining_file_path
        //                    << L":" << parent_node->defining_file_line << L" not found.\n";
        //            }
        //        }
        //        else {
        //            QStringList command_parts;
        //            for (int k = 0; k < command_names.size(); k++) {
        //                command_parts.append(QString::fromStdString(command_names[k]));
        //            }

        //            // is the command incomplete and should be appended to previous command instead of replacing it?
        //            if (is_command_incomplete_macro(command_names)) {
        //                for (int k = 0; k < previous_names.size(); k++) {
        //                    command_parts.append(QString::fromStdString(previous_names[k]));
        //                    parent_node->name_.push_back(previous_names[k]);
        //                }
        //            }

        //            std::wstring joined_command = command_parts.join(";").toStdWString();
        //            parent_node->generator = [joined_command, command_manager](MainWidget* w) {return std::make_unique<MacroCommand>(w, command_manager, "", joined_command); };
        //        }
        //        //if (command_names[j].size())
        //    }
        //    else {
        //        if (SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE && parent_node->is_final && (parent_node->name_.size() > 0)) {
        //            std::wcerr << L"Warning: unmapping " << utf8_decode(parent_node->name_[0]) << L" because of " << command_strings[j] << L" which uses " << line << L"\n";
        //        }
        //        parent_node->is_final = false;
        //    }

        //}
    }

    return root;
}

bool InputHandler::add_keybind(const std::wstring& keybind, const std::wstring& command, const std::wstring& file_name, int line_number) {
    return parse_line(root, command_manager, keybind, command, file_name, line_number);
}

InputParseTreeNode* parse_lines(
    CommandManager* command_manager,
    const std::vector<std::wstring>& lines,
    const std::vector<std::wstring>& command_names,
    const std::vector<std::wstring>& command_file_names,
    const std::vector<int>& command_line_numbers
) {
    // parse key configs into a trie where leaves are annotated with the name of the command

    InputParseTreeNode* root = new InputParseTreeNode;
    root->is_root = true;

    parse_lines(root, command_manager, lines, command_names, command_file_names, command_line_numbers);

    return root;

}


void get_keys_file_lines(const Path& file_path,
    std::vector<std::wstring>& command_strings,
    std::vector<std::wstring>& command_keys,
    std::vector<std::wstring>& command_files,
    std::vector<int>& command_line_numbers) {

    std::ifstream infile = std::ifstream(utf8_encode(file_path.get_path()));

    int line_number = 0;
    std::wstring default_path_name = file_path.get_path();
    while (infile.good()) {
        line_number++;
        std::string line_;
        std::wstring line;
        std::getline(infile, line_);
        line = utf8_decode(line_);

        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        QString line_string = QString::fromStdWString(line).trimmed();
        int last_space_index = line_string.lastIndexOf(' ');

        if (last_space_index >= 0) {
            std::wstring command_name = line_string.left(last_space_index).trimmed().toStdWString();
            std::wstring command_key = line_string.right(line_string.size() - last_space_index - 1).trimmed().toStdWString();

            command_strings.push_back(command_name);
            command_keys.push_back(command_key);
            command_files.push_back(default_path_name);
            command_line_numbers.push_back(line_number);
        }
    }

    infile.close();
}

InputParseTreeNode* parse_key_config_files(CommandManager* command_manager, const Path& default_path,
    const std::vector<Path>& user_paths) {

    std::wifstream default_infile = open_wifstream(default_path.get_path());

    std::vector<std::wstring> command_strings;
    std::vector<std::wstring> command_keys;
    std::vector<std::wstring> command_files;
    std::vector<int> command_line_numbers;

    get_keys_file_lines(default_path, command_strings, command_keys, command_files, command_line_numbers);
    for (auto upath : user_paths) {
        get_keys_file_lines(upath, command_strings, command_keys, command_files, command_line_numbers);
    }

    for (auto additional_keymap : ADDITIONAL_KEYMAPS) {
        QString keymap_string = QString::fromStdWString(additional_keymap.keymap_string);
        int last_space_index = keymap_string.lastIndexOf(' ');
        std::wstring command_name = keymap_string.left(last_space_index).toStdWString();
        std::wstring mapping = keymap_string.right(keymap_string.size() - last_space_index - 1).toStdWString();
        command_strings.push_back(command_name);
        command_keys.push_back(mapping);
        command_files.push_back(additional_keymap.file_name);
        command_line_numbers.push_back(additional_keymap.line_number);
    }

    return parse_lines(command_manager, command_keys, command_strings, command_files, command_line_numbers);
}


InputHandler::InputHandler(const Path& default_path, const std::vector<Path>& user_paths, CommandManager* cm) {
    command_manager = cm;
    user_key_paths = user_paths;
    reload_config_files(default_path, user_paths);
}

void InputHandler::reload_config_files(const Path& default_config_path, const std::vector<Path>& user_config_paths)
{
    command_keymapping_cache.clear();
    delete_current_parse_tree(root);

    root = parse_key_config_files(command_manager, default_config_path, user_config_paths);
    current_node = root;
}

std::vector<std::string> InputHandler::get_key_mappings(std::string command_name) const {
    auto cached = command_keymapping_cache.find(command_name);
    if (cached != command_keymapping_cache.end()) {
        return cached->second;
    }
    else {
        auto all_mappings = get_command_key_mappings();
        for (auto [name, value] : all_mappings) {
            if (name == command_name) {
                command_keymapping_cache[name] = value;
                return value;
            }
        }
        command_keymapping_cache[command_name] = {};
        return {};
    }
}

void InputHandler::get_definition_file_and_line_helper(InputParseTreeNode* parent, std::string command_name, std::vector<KeybindDefinitionLocation>& out_locations) const {
    for (auto child : parent->children) {
        if (child->is_final) {
            for (auto name : child->name_) {
                if (name == command_name) {
                    KeybindDefinitionLocation loc;
                    loc.file_path = child->defining_file_path;
                    loc.line_number = child->defining_file_line;
                    out_locations.push_back(loc);
                }
            }
        }
    }

    for (auto child : parent->children) {
        if (!child->is_final) {
            get_definition_file_and_line_helper(child, command_name, out_locations);
        }
    }

}

std::vector<KeybindDefinitionLocation> InputHandler::get_definition_file_and_line(std::string command_name) const {
    std::vector<KeybindDefinitionLocation> res;
    get_definition_file_and_line_helper(root, command_name, res);
    return res;
}


bool is_digit(int key) {
    return key >= Qt::Key::Key_0 && key <= Qt::Key::Key_9;
}

std::unique_ptr<Command> InputHandler::get_menu_command(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool command_pressed, bool alt_pressed) {
    // get the command for keyevent while we are in a menu. In menus we don't
    // support key melodies so we just check the children of the root if they match
    int key = get_event_key(key_event, &shift_pressed, &control_pressed, &command_pressed, &alt_pressed);
    for (auto child : root->children) {
        if (child->is_final && child->matches(key, shift_pressed, control_pressed, command_pressed, alt_pressed)) {
            if (child->generator.has_value()) {
                return child->generator.value()(w);
            }
        }
    }

    return {};
}

int InputHandler::get_event_key(QKeyEvent* key_event, bool* shift_pressed, bool* control_pressed, bool* command_pressed, bool* alt_pressed) {
    int key = 0;
    if (!USE_LEGACY_KEYBINDS) {
        std::vector<QString> special_texts = { "\b", "\u007F", "\t", " ", "\r", "\n" };
        if (((key_event->key() >= 'A') && (key_event->key() <= 'Z')) || ((key_event->text().size() > 0) &&
            (std::find(special_texts.begin(), special_texts.end(), key_event->text()) == special_texts.end()))) {
            if (!(*control_pressed) && !(*alt_pressed) && !(*command_pressed)) {
                std::wstring text = key_event->text().toStdWString();
                key = key_event->text().toStdWString()[0];
            }
            else {
                auto text = key_event->text();
                key = key_event->key();

                if ((key >= 'A' && key <= 'Z') && (!*shift_pressed)) {
                    if (!*shift_pressed) {
                        key = key - 'A' + 'a';
                    }
                }
            }
            // shift is already handled in the returned text
            *shift_pressed = false;
        }
        else {
            key = key_event->key();

            if (key == Qt::Key::Key_Backtab) {
                key = Qt::Key::Key_Tab;
            }
        }
    }
    else {
        key = key_event->key();
        if (key >= 'A' && key <= 'Z') {
            key = key - 'A' + 'a';
        }

    }
    return key;
}

std::unique_ptr<Command> InputHandler::handle_key(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool command_pressed, bool alt_pressed, int* num_repeats) {

    int key = get_event_key(key_event, &shift_pressed, &control_pressed, &command_pressed, &alt_pressed);
    if (current_node == root && is_digit(key)) {
        if (!(key == '0' && (number_stack.size() == 0)) && (!control_pressed) && (!shift_pressed) && (!alt_pressed) && (!command_pressed)) {
            number_stack.push_back('0' + key - Qt::Key::Key_0);
            return nullptr;
        }
    }

    for (InputParseTreeNode* child : current_node->children) {
        //if (child->command == key && child->shift_modifier == shift_pressed && child->control_modifier == control_pressed){
        if (child->matches(key, shift_pressed, control_pressed, command_pressed, alt_pressed)) {
            if (child->is_final == true) {
                current_node = root;
                //cout << child->name << endl;
                *num_repeats = 0;
                if (number_stack.size() > 0) {
                    *num_repeats = atoi(number_stack.c_str());
                    number_stack.clear();
                }

                //return command_manager.get_command_with_name(child->name);
                if (child->generator.has_value()) {
                    return (child->generator.value())(w);
                }
                return nullptr;
                //for (size_t i = 0; i < child->name.size(); i++) {
                //	res.push_back(command_manager->get_command_with_name(child->name[i]));
                //}
                //return res;
            }
            else {
                current_node = child;
                return nullptr;
            }
        }
    }
    LOG(std::wcerr << "Warning: invalid command (key:" << (char)key << "); resetting to root" << std::endl);
    number_stack.clear();
    current_node = root;
    return nullptr;
}

void InputHandler::delete_current_parse_tree(InputParseTreeNode* node_to_delete)
{
    bool is_root = false;

    if (node_to_delete != nullptr) {
        is_root = node_to_delete->is_root;

        for (size_t i = 0; i < node_to_delete->children.size(); i++) {
            delete_current_parse_tree(node_to_delete->children[i]);
        }
        delete node_to_delete;
    }

    if (is_root) {
        root = nullptr;
    }
}

bool InputParseTreeNode::is_same(const InputParseTreeNode* other) {
    return (command == other->command) &&
        (shift_modifier == other->shift_modifier) &&
        (control_modifier == other->control_modifier) &&
        (command_modifier == other->command_modifier) &&
        (alt_modifier == other->alt_modifier) &&
        (requires_symbol == other->requires_symbol) &&
        (requires_text == other->requires_text);
}

bool InputParseTreeNode::matches(int key, bool shift, bool ctrl, bool cmd, bool alt)
{
    return (key == this->command) && (shift == this->shift_modifier) && (ctrl == this->control_modifier) && (cmd == this->command_modifier) && (alt == this->alt_modifier);
}

std::optional<Path> InputHandler::get_or_create_user_keys_path() {
    if (user_key_paths.size() == 0) {
        return {};
    }

    for (int i = user_key_paths.size() - 1; i >= 0; i--) {
        if (user_key_paths[i].file_exists()) {
            return user_key_paths[i];
        }
    }
    user_key_paths.back().file_parent().create_directories();
    create_file_if_not_exists(user_key_paths.back().get_path());
    return user_key_paths.back();
}

std::unordered_map<std::string, std::vector<std::string>> InputHandler::get_command_key_mappings() const {
    std::unordered_map<std::string, std::vector<std::string>> res;
    std::vector<InputParseTreeNode*> prefix;
    add_command_key_mappings(root, res, prefix);
    return res;
}

void InputHandler::add_command_key_mappings(InputParseTreeNode* thisroot,
    std::unordered_map<std::string, std::vector<std::string>>& map,
    std::vector<InputParseTreeNode*> prefix) const {

    if (thisroot->is_final) {
        std::string key_string = get_key_string_from_tree_node_sequence(prefix);
        std::string cmd_name;

        if (thisroot->name_.size() == 1) {
            map[thisroot->name_[0]].push_back(key_string);
        }
        else if (thisroot->name_.size() > 1) {
            for (const auto& name : thisroot->name_) {
                map[name].push_back("{" + get_key_string_from_tree_node_sequence(prefix) + "}");
            }
        }
    }
    else {
        for (size_t i = 0; i < thisroot->children.size(); i++) {
            prefix.push_back(thisroot->children[i]);
            add_command_key_mappings(thisroot->children[i], map, prefix);
            prefix.pop_back();
        }

    }
}

std::string InputHandler::get_key_name_from_key_code(int key_code) const {
    std::string result;
    std::map<int, std::string> keymap = {
        {Qt::Key::Key_Up, "up"},
        {Qt::Key::Key_Down, "down"},
        {Qt::Key::Key_Left, "left"},
        {Qt::Key::Key_Right, "right"},
        {Qt::Key::Key_Backspace, "backspace"},
        {Qt::Key::Key_Space, "space"},
        {Qt::Key::Key_PageUp, "pageup"},
        {Qt::Key::Key_PageDown, "pagedown"},
        {Qt::Key::Key_Home, "home"},
        {Qt::Key::Key_End, "end"},
        {Qt::Key::Key_Tab, "tab"},
        {Qt::Key::Key_Backtab, "tab"},
        {Qt::Key::Key_Space, "space"},
    };

    //if (((key_code <= 'z') && (key_code >= 'a')) || ((key_code <= 'Z') && (key_code >= 'A'))) {
    if (keymap.find(key_code) != keymap.end()) {
        return "<" + keymap[key_code] + ">";
    }
    else if (key_code < 127) {
        result.push_back(key_code);
        return result;
    }
    else if ((key_code >= Qt::Key::Key_F1) && (key_code <= Qt::Key::Key_F35)) {
        int f_number = 1 + (key_code - Qt::Key::Key_F1);
        return "<f" + QString::number(f_number).toStdString() + ">";
    }
    else {
        return "UNK";
    }
}

std::string InputHandler::get_key_string_from_tree_node_sequence(const std::vector<InputParseTreeNode*> seq) const {
    std::string res;
    for (size_t i = 0; i < seq.size(); i++) {
        bool has_modifier = seq[i]->alt_modifier || seq[i]->shift_modifier || seq[i]->control_modifier || seq[i]->command_modifier;
        if (has_modifier) {
            res += "<";
        }
        std::string current_key_command_name = get_key_name_from_key_code(seq[i]->command);

        if (seq[i]->alt_modifier) {
            res += "A-";
        }
        if (seq[i]->control_modifier) {
            res += "C-";
        }
        if (seq[i]->command_modifier) {
            res += "D-";
        }
        if (seq[i]->shift_modifier) {
            res += "S-";
        }
        res += current_key_command_name;
        if (has_modifier) {
            res += ">";
        }
    }
    return res;
}
std::vector<Path> InputHandler::get_all_user_keys_paths() {
    std::vector<Path> res;

    for (int i = user_key_paths.size() - 1; i >= 0; i--) {
        if (user_key_paths[i].file_exists()) {
            res.push_back(user_key_paths[i]);
        }
    }

    return res;
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

void Command::pre_perform() {

}

void Command::on_cancel() {

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



bool is_macro_command_enabled(Command* command) {
    MacroCommand* macro_command = dynamic_cast<MacroCommand*>(command);
    if (macro_command) {
        return macro_command->is_enabled();
    }

    return false;
}

std::unique_ptr<Command> CommandManager::create_macro_command(MainWidget* w, std::string name, std::wstring macro_string) {
    return std::make_unique<MacroCommand>(w, this, name, macro_string);
}

std::unique_ptr<Command> CommandManager::create_macro_command_with_args(MainWidget* w, std::string name, QString command, QStringList args) {
    return std::make_unique<MacroCommand>(w, this, name, command, args);
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
            result_socket->write("<NULL>");
            return;
        }
        std::string result_str = utf8_encode(result.value());
        if (result_str.size() > 0) {
            result_socket->write(result_str.c_str());
        }
        else {
            result_socket->write("<NULL>");
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
