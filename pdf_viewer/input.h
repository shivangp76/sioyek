#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <qdatetime.h>

#include "path.h"
#include "coordinates.h"
#include "utils.h"

class QLocalSocket;
class MainWidget;
class ConfigManager;
class Command;
class CommandManager;

struct InputParseTreeNode {

    std::vector<InputParseTreeNode*> children;
    //char command;
    int command;
    //std::vector<std::string> name;
    std::vector<std::string> name_;
    std::optional<std::function<std::unique_ptr<Command>(MainWidget*)>> generator = {};
    bool shift_modifier = false;
    bool control_modifier = false;
    bool alt_modifier = false;
    bool command_modifier = false;
    bool requires_text = false;
    bool requires_symbol = false;
    bool is_root = false;
    bool is_final = false;

    // todo: use a pointer to reduce allocation
    std::wstring defining_file_path;
    int defining_file_line;

    bool is_same(const InputParseTreeNode* other);
    bool matches(int key, bool shift, bool ctrl, bool cmd, bool alt);
};

struct KeybindDefinitionLocation {
    std::wstring file_path;
    int line_number;
};


class InputHandler {
private:
    InputParseTreeNode* root = nullptr;
    InputParseTreeNode* current_node = nullptr;
    CommandManager* command_manager;
    std::string number_stack;
    std::vector<Path> user_key_paths;
    mutable std::unordered_map<std::string, std::vector<std::string>> command_keymapping_cache;

    std::string get_key_string_from_tree_node_sequence(const std::vector<InputParseTreeNode*> seq) const;
    std::string get_key_name_from_key_code(int key_code) const;

    void add_command_key_mappings(InputParseTreeNode* root, std::unordered_map<std::string, std::vector<std::string>>& map, std::vector<InputParseTreeNode*> prefix) const;
public:
    //char create_link_sumbol = 0;
    //char create_bookmark_symbol = 0;

    bool add_keybind(const std::wstring& keybind, const std::wstring& command, const std::wstring& file_name, int line_number);
    InputHandler(const Path& default_path, const std::vector<Path>& user_paths, CommandManager* cm);
    void reload_config_files(const Path& default_path, const std::vector<Path>& user_path);
    //std::vector<std::unique_ptr<Command>> handle_key(QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed ,int* num_repeats);
    int get_event_key(QKeyEvent* key_event, bool* shift_pressed, bool* control_pressed, bool* is_command_pressed, bool* alt_pressed);
    std::unique_ptr<Command> handle_key(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool is_command_pressed, bool alt_pressed, int* num_repeats);
    std::unique_ptr<Command> get_menu_command(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool command_pressed, bool alt_pressed);
    void delete_current_parse_tree(InputParseTreeNode* node_to_delete);

    std::optional<Path> get_or_create_user_keys_path();
    std::vector<Path> get_all_user_keys_paths();
    std::unordered_map<std::string, std::vector<std::string>> get_command_key_mappings() const;
    std::vector<std::string> get_key_mappings(std::string command_name) const;
    void get_definition_file_and_line_helper(InputParseTreeNode* parent, std::string command_name, std::vector<KeybindDefinitionLocation>& locations) const;
    std::vector<KeybindDefinitionLocation> get_definition_file_and_line(std::string command_name) const;

};


struct MenuItems;

using RecursiveItem = std::variant<std::unique_ptr<Command>, std::unique_ptr<MenuItems>>;

struct MenuItems {
    std::wstring name;
    std::vector<RecursiveItem> items;
};

std::unique_ptr<MenuItems> parse_menu_string(MainWidget* widget, CommandManager* command_manager, QString name, QString menu_string);
