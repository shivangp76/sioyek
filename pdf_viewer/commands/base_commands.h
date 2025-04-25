#pragma once

#include <string>
#include <optional>
#include <memory>
#include <map>

#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>

#include "coordinates.h"
#include "utils.h"
#include "commands/command_parser.h"

class MainWidget;
class DocumentView;
class QLocalSocket;
class CommandManager;
class Command;
class ConfigManager;

class CommandManager {
private:
    //std::vector<Command> commands;
public:

    std::map < std::string, std::function<std::unique_ptr<Command>(MainWidget*)> > new_commands;
    std::map<std::string, std::string> command_human_readable_names;
    std::map<QString, bool> command_requires_pro;
    std::map<std::string, QDateTime> command_last_uses;
    std::unordered_map<QString, QString> command_required_prefixes;
    std::unordered_map<std::string, std::string> command_aliases;

    CommandManager(ConfigManager* config_manager);
    std::unique_ptr<Command> get_command_with_name(MainWidget* w, std::string name);
    std::unique_ptr<Command> create_macro_command(MainWidget* w, std::string name, std::wstring macro_string);
    std::unique_ptr<Command> create_macro_command_with_args(MainWidget* w, std::string name, QString command, QStringList args);
    QStringList get_all_command_names();
    void handle_new_javascript_command(std::wstring command_name, JsCommandInfo command_files_pair, bool is_async, std::wstring code=L"");
    void update_command_last_use(std::string command_name);

};

enum RequirementType {
    Text,
    Password,
    Symbol,
    File,
    Folder,
    Rect,
    Point,
    Generic,
    OptionalText
};

struct Requirement {
    RequirementType type;
    std::string name;
};


class Command {
private:
protected:
    int num_repeats = 1;
    MainWidget* widget = nullptr;
    std::optional<std::wstring> result = {};
    std::string command_cname;
public:
    virtual void perform() = 0;

    static inline const bool developer_only = false;
    static inline const bool requires_pro = false;
    QLocalSocket* result_socket = nullptr;
    std::wstring* result_holder = nullptr;
    bool* is_done = nullptr;

    Command(std::string name, MainWidget* widget);
    virtual std::optional<Requirement> next_requirement(MainWidget* widget);
    virtual std::optional<std::wstring> get_result();

    DocumentView* dv();
    virtual void set_text_requirement(std::wstring value);
    virtual void set_symbol_requirement(char value);
    virtual void set_file_requirement(std::wstring value);
    virtual void set_rect_requirement(AbsoluteRect value);
    virtual void set_point_requirement(AbsoluteDocumentPos value);
    virtual void set_generic_requirement(QVariant value);
    virtual void handle_generic_requirement();
    virtual void set_num_repeats(int nr);
    virtual int get_num_repeats();
    virtual std::vector<char> special_symbols();
    virtual void pre_perform();
    virtual bool pushes_state();
    virtual bool requires_document();
    virtual void on_cancel();
    virtual void on_result_computed();
    virtual void set_result_socket(QLocalSocket* result_socket);
    virtual void set_result_mutex(bool* res_mut, std::wstring* result_location);
    virtual std::optional<std::wstring> get_text_suggestion(int index);
    virtual bool is_menu_command();
    virtual bool is_holdable();
    virtual void perform_up();
    virtual void on_key_hold();
    virtual void on_text_change(const QString& new_text);
    virtual std::optional<AbsoluteRect> get_text_editor_rectangle();
    virtual std::optional<QString> get_file_path_requirement_root_dir();

    virtual QStringList get_autocomplete_options();

    void set_next_requirement_with_string(std::wstring str);

    virtual void run();
    virtual std::string get_name();
    virtual std::string get_symbol_hint_name();
    virtual std::string get_human_readable_name();
    virtual std::wstring get_text_default_value();
    virtual ~Command();
};

bool is_macro_command_enabled(Command* command);

class KeyboardSelectPointCommand : public Command {
protected:
    std::optional<std::wstring> text = {};
    bool already_pre_performed = false;
    std::unique_ptr<Command> origin;
    bool requires_rect = false;
public:

    KeyboardSelectPointCommand(MainWidget* w, std::unique_ptr<Command> original_command);

    bool is_done();

    virtual std::optional<Requirement> next_requirement(MainWidget* widget);

    virtual void perform();
    virtual void on_cancel() override;

    void pre_perform();

    virtual std::string get_name();


    virtual void set_symbol_requirement(char value);
};


// class WrapperCommand: public Command{
//     WrapperCommand(std::wstring macro);
// };

class LazyCommand : public Command {
private:
    CommandManager* command_manager;
    std::string command_name;
    std::vector<std::wstring> command_params;
    std::unique_ptr<Command> actual_command = nullptr;
    std::unique_ptr<Command> noop = nullptr;

    std::optional<std::wstring> get_result() override;
    void set_result_socket(QLocalSocket* socket) override;
    void set_result_mutex(bool* p_is_done, std::wstring* result_location) override;

    Command* get_command();


public:
    LazyCommand(std::string name, MainWidget* widget_, CommandManager* manager, CommandInvocation invocation);

    void set_text_requirement(std::wstring value);
    void set_symbol_requirement(char value);
    void set_file_requirement(std::wstring value);
    void set_rect_requirement(AbsoluteRect value);
    void set_generic_requirement(QVariant value);
    void handle_generic_requirement();
    void set_point_requirement(AbsoluteDocumentPos value);
    void set_num_repeats(int nr);
    std::vector<char> special_symbols();
    void pre_perform();
    bool pushes_state();
    bool requires_document();
    std::optional<Requirement> next_requirement(MainWidget* widget);
    virtual void perform();
    std::string get_name();


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

    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::vector<CommandInvocation> command_invocations);
    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::vector<std::unique_ptr<Command>>&& cmds);
    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, QString command_name, QStringList args);
    MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_);

    std::unique_ptr<Command> get_subcommand(CommandInvocation invocation);
    void set_result_socket(QLocalSocket* rsocket);
    void set_result_mutex(bool* id, std::wstring* result_location);
    void initialize_from_invocations(std::vector<CommandInvocation> command_invocations);
    std::optional<std::wstring> get_result() override;
    void set_text_requirement(std::wstring value);
    bool is_menu_command();
    void set_generic_requirement(QVariant value);
    void handle_generic_requirement();
    void set_symbol_requirement(char value);
    bool requires_document() override;
    void set_file_requirement(std::wstring value);
    int get_command_index_for_requirement_type(RequirementType reqtype);
    void set_rect_requirement(AbsoluteRect value);
    void set_point_requirement(AbsoluteDocumentPos value);
    std::wstring get_text_default_value();
    void pre_perform();
    std::optional<Requirement> next_requirement(MainWidget* widget);
    void perform_subcommand(int index);
    bool is_enabled();
    bool is_holdable();
    void perform_up();
    void on_key_hold();
    void perform();
    int get_current_mode_index();
    int get_current_executing_command_index();
    void on_text_change(const QString& new_text);
    std::vector<char> special_symbols();
    bool mode_matches(std::string current_mode, std::string command_mode);
    std::string get_name();
    std::string get_human_readable_name() override;
    std::optional<std::wstring> get_text_suggestion(int index);

};

class JavascriptCommand : public Command {
public:
    std::string command_name;
    std::wstring code;
    std::optional<std::wstring> entry_point = {};
    bool is_async;

    JavascriptCommand(std::string command_name, std::wstring code_, std::optional<std::wstring> entry_point_, bool is_async_, MainWidget* w);
    void perform();
    std::string get_name();

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
        QString name_qstring = QString::fromStdString(name);
        manager->command_required_prefixes[name_qstring] = "";
        manager->command_requires_pro[name_qstring] = T::requires_pro;
    }
}

class GenericPathCommand : public Command {
public:
    std::optional<std::wstring> selected_path = {};

    GenericPathCommand(std::string name, MainWidget* w);

    std::optional<Requirement> next_requirement(MainWidget* widget) override;
    void set_generic_requirement(QVariant value);

};

class GenericGotoLocationCommand : public Command {
public:
    std::optional<float> target_location = {};

    GenericGotoLocationCommand(std::string name, MainWidget* w);

    std::optional<Requirement> next_requirement(MainWidget* widget);
    void set_generic_requirement(QVariant value);
    void perform();
    bool pushes_state();

};

class GenericPathAndLocationCommadn : public Command {
public:

    std::optional<QVariant> target_location;
    bool is_hash = false;
    GenericPathAndLocationCommadn(std::string name, MainWidget* w, bool is_hash_ = false);

    std::optional<Requirement> next_requirement(MainWidget* widget);
    void set_generic_requirement(QVariant value);
    void perform();
    bool pushes_state();
};

class SymbolCommand : public Command {
public:
    char symbol = 0;
    SymbolCommand(std::string name, MainWidget* w);

    virtual std::optional<Requirement> next_requirement(MainWidget* widget);
    virtual void set_symbol_requirement(char value);
    virtual std::string get_human_readable_name();

};

class TextCommand : public Command {
protected:
    std::optional<std::wstring> text = {};
public:

    TextCommand(std::string name, MainWidget* w);

    virtual std::string text_requirement_name();
    virtual std::optional<Requirement> next_requirement(MainWidget* widget);
    virtual void set_text_requirement(std::wstring value);
    std::wstring get_text_default_value();
};

class GenericWaitCommand : public Command {
public:
    GenericWaitCommand(std::string name, MainWidget* w);
    bool finished = false;
    QTimer* timer = nullptr;
    QMetaObject::Connection timer_connection;

    std::optional<Requirement> next_requirement(MainWidget* widget);

    virtual ~GenericWaitCommand();

    void set_generic_requirement(QVariant value);
    virtual bool is_ready() = 0;
    void handle_generic_requirement() override;
    void perform();

};

class JsCallCommand : public Command {
public:
    std::string command_name;
    std::wstring funcall;

    JsCallCommand(std::string command_name, std::wstring func_, MainWidget* widget);
    void perform();
    std::string get_name();

};

class OpenLinkCommand : public Command {
public:
    static inline const std::string cname = "open_link";
    static inline const std::string hname = "Go to PDF links using keyboard";
    OpenLinkCommand(std::string name, MainWidget* w);
    OpenLinkCommand(MainWidget* w);
protected:
    std::optional<std::wstring> text = {};
    bool already_pre_performed = false;
    std::vector<PdfLink> tagged_links;
public:

    virtual std::string text_requirement_name();
    bool is_done();
    virtual std::optional<Requirement> next_requirement(MainWidget* widget);
    virtual void perform();
    void pre_perform();
    virtual void set_text_requirement(std::wstring value);
    virtual void set_symbol_requirement(char value);
    virtual void perform_with_link(PdfLink link);
};

class TagCommand : public Command {
public:
    std::vector<DocumentRect> tags;
    std::string selected_tag;
    bool pre_performed = false;

    TagCommand(std::string cname, MainWidget* w);
    bool is_done();
    std::optional<Requirement> next_requirement(MainWidget* widget) override;
    void set_symbol_requirement(char value) override;
    void pre_perform() override;

};

class CustomCommand : public Command {

    std::wstring raw_command;
    std::string name;
    std::optional<AbsoluteRect> command_rect;
    std::optional<AbsoluteDocumentPos> command_point;
    std::optional<std::wstring> command_text;

public:

    CustomCommand(MainWidget* widget_, std::string name_, std::wstring command_);
    std::optional<Requirement> next_requirement(MainWidget* widget);
    void set_rect_requirement(AbsoluteRect rect);
    void set_point_requirement(AbsoluteDocumentPos point);
    void set_text_requirement(std::wstring txt);
    void perform();
    std::string get_name();


};

class HoldableCommand : public Command {
    std::unique_ptr<Command> down_command = {};
    std::unique_ptr<Command> up_command = {};
    std::unique_ptr<Command> hold_command = {};
    std::string name;
    CommandManager* command_manager;

public:
    HoldableCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_);
    std::optional<Requirement> next_requirement(MainWidget* widget);
    bool requires_document();
    void perform();
    void perform_up();
    void on_key_hold();
    bool is_holdable();


};

void register_base_commands(CommandManager* manager);
