#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <qclipboard.h>
#include <qjsondocument.h>
#include <QKeyEvent>
#include <qstring.h>
#include <qstringlist.h>
#include <qlocalsocket.h>
#include <qfileinfo.h>
#include <qdesktopservices.h>
#include <qsettings.h>

#include "utils.h"
#include "input.h"
#include "main_widget.h"
#include "ui.h"
#include "document.h"
#include "document_view.h"
#include "network_manager.h"

#include "commands/base_commands.h"
#include "commands/annotation_commands.h"
#include "commands/misc_commands.h"
#include "commands/navigation_commands.h"
#include "commands/network_commands.h"




extern bool SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE;
extern bool USE_LEGACY_KEYBINDS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, JsCommandInfo> ADDITIONAL_ASYNC_JAVASCRIPT_COMMANDS;
extern std::map<std::wstring, CustomCommandInfo> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, CustomCommandInfo> ADDITIONAL_MACROS;
extern std::vector<AdditionalKeymapData> ADDITIONAL_KEYMAPS;

extern bool VERBOSE;


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



CommandManager::CommandManager(ConfigManager* config_manager) {

    register_base_commands(this);
    register_misc_commands(this);
    register_annotation_commands(this);
    register_navigation_commands(this);
    register_network_commands(this);
    register_command<OpenLinkCommand>(this);

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




bool is_macro_command_enabled(Command* command) {
    MacroCommand* macro_command = dynamic_cast<MacroCommand*>(command);
    if (macro_command) {
        return macro_command->is_enabled();
    }

    return false;
}
