#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <optional>
#include <variant>
#include <map>
#include <functional>
#include <qwidget.h>
#include <qabstractitemmodel.h>

#include "path.h"
#include "coordinates.h"
#include "book.h"

//#include <main_widget.h>

class MainWidget;


enum class ConfigType {
    Int,
    Float,
    Color3,
    Color4,
    Bool,
    String,
    FilePath,
    FolderPath,
    IVec2,
    FVec2,
    EnableRectangle,
    Range,
    Macro,
    Symbol,
    Enum,
    DynamicEnum
};

struct UIRange {
    float top;
    float bottom;
};

struct UIRect {
    bool enabled;
    float left;
    float right;
    float top;
    float bottom;


    bool contains(NormalizedWindowPos window_pos);
    QRect to_window(int window_width, int window_height);
};

struct FloatExtras {
    float min_val;
    float max_val;
};

struct ColorExtras {
    float light_mode[4] = {-1, -1, -1, -1};
    float dark_mode[4] = {-1, -1, -1, -1};
    float custom_mode[4] = {-1, -1, -1, -1};
};

struct IntExtras {
    int min_val;
    int max_val;
};

enum DocumentLocationMismatchStrategy {
    Local = 0,
    Server = 1,
    Ask = 2,
    ShowButton = 3
};

enum RulerDisplayMode {
    RulerBox = 0,
    RulerSlit = 1,
    RulerUnderline = 2,
    RulerHighlightBelow = 3,
    RulerHighlightRuler = 4,
    RulerHighlightTransparent = 5,
};


enum ColorMode {
    Light = 0,
    Dark = 1,
    Custom = 2
};

enum TableExtractBehaviour {
    Bookmark = 0,
    Copy = 1
};


enum HighlightStyle {
    HighlightTransparent = 0,
    HighlightBackground = 1,
    HighlightBorder=2
};

enum SelectedTextHighlightStyle {
    SelectedTextHighlightTransparent = 0,
    SelectedTextHighlightInverted = 1,
    SelectedTextHighlightBackground = 2
};

struct EnumExtras {
    std::vector<std::wstring> possible_values;
};

struct DynamicEnumExtras {
    std::function<std::vector<std::wstring>(MainWidget* widget)> get_possible_values;
};

struct EmptyExtras {
};

struct AdditionalKeymapData {
    std::wstring file_name;
    int line_number;
    std::wstring keymap_string;
};
//union ConfigExtras {
//	struct Rest {
//
//	} rest;
//};

using Extras = std::variant<FloatExtras, IntExtras, EmptyExtras, EnumExtras, ColorExtras, DynamicEnumExtras>;

struct Config {

    std::wstring name;
    ConfigType config_type;
    void* value = nullptr;
    std::function<void(void*, std::wstringstream&)> serialize = nullptr;
    std::function<void*(std::wstringstream&, void* res, bool* changed)> deserialize_ = nullptr;
    std::function<bool(const std::wstring& value)> validator = nullptr;
    std::optional<std::function<void(MainWidget*)>> on_change = {};
    Extras extras = EmptyExtras{};
    std::wstring default_value_string;
    bool is_auto = false;

    std::wstring alias_for = L"";

    std::wstring definition_file = L"";
    int definition_line = -1;

    void* deserialize(std::wstringstream&, void* res, bool* changed);
    void* get_value();
    void save_value_into_default();
    void load_default();
    std::wstring get_type_string() const;
    std::wstring get_current_string();
    bool has_changed_from_default();
    bool is_empty_string();
    void set_change_fn(std::function<void(MainWidget*)> on_change_fn);

};

class ConfigManager {

    std::vector<Config*> configs;


    std::vector<Path> user_config_paths;

public:
    Config* get_mut_config_with_name(std::wstring config_name);
    std::wstring get_config_value_string(std::wstring config_name);

    ConfigManager(const Path& default_path, const Path& auto_path, const std::vector<Path>& user_paths);
    void serialize(const Path& path);
    void serialize_auto_configs(std::wofstream& stream);
    void restore_default();
    void clear_file(const Path& path);
    void persist_config();
    void deserialize(std::vector<std::string>* changed_config_names, const Path& default_file_path, const Path& auto_path, const std::vector<Path>& user_file_paths);
    void deserialize_file(std::vector<std::string>* changed_config_names, const Path& file_path, bool warn_if_not_exists = false, bool is_auto=false);
    template<typename T>
    const T* get_config(std::wstring name) {

        void* raw_pointer = get_mut_config_with_name(name)->get_value();

        // todo: Provide a default value for all configs, so that all the nullchecks here and in the
        // places where `get_config` is called can be removed.
        if (raw_pointer == nullptr) return nullptr;
        return (T*)raw_pointer;
    }
    std::optional<Path> get_or_create_user_config_file();
    std::vector<Path> get_all_user_config_files();
    std::vector<Config*> get_configs();
    std::vector<Config*>* get_configs_ptr();
    bool deserialize_config(std::string config_name, std::wstring config_value);
    void restore_defaults_in_memory();
    std::vector<std::wstring> get_auto_config_names();
    void handle_set_color_palette(MainWidget* w, ColorPalette palette);
    void export_config_names(std::wstring file_path);
    void export_default_config_file(std::wstring file_path);
};

bool get_custom_command_definition_file_and_line_number(std::wstring command_name, std::wstring& out_path, int& out_line_number);
