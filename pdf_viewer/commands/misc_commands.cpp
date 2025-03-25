#include <qlineedit.h>
#include <qclipboard.h>
#include <qapplication.h>
#include <qdesktopservices.h>

#include "commands/misc_commands.h"
#include "main_widget.h"
#include "document_view.h"
#include "config.h"
#include "ui.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

extern Path default_keys_path;
extern Path sioyek_js_path;
extern Path local_database_file_path;
extern Path global_database_file_path;

extern float TTS_RATE;
extern float TTS_RATE_INCREMENT;
extern bool TOUCH_MODE;
extern bool ADD_NEWLINES_WHEN_COPYING_TEXT;
extern std::wstring CONTEXT_MENU_ITEMS;
extern Path default_config_path;
extern bool SHOW_SETCONFIG_IN_STATUSBAR;

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

class SetTtsVoiceCommand : public Command {
public:
    static inline const std::string cname = "set_tts_voice";
    static inline const std::string hname = "Set the text to speech voice";
    SetTtsVoiceCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->show_tts_voice_selector();
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
        else if (widget->is_high_quality_tts_playing()) {
            widget->set_high_quality_tts_rate(TTS_RATE);
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
        else if (widget->is_high_quality_tts_playing()) {
            widget->set_high_quality_tts_rate(TTS_RATE);
        }
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

class GetCurrentDocumentTextCommand : public Command {

public:
    static inline const std::string cname = "get_current_document_text";
    static inline const std::string hname = "";
    GetCurrentDocumentTextCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = widget->get_current_document_text().toStdWString();
    }
};

class GetCurrentChapterTextCommand : public Command {

public:
    static inline const std::string cname = "get_current_chapter_text";
    static inline const std::string hname = "";
    GetCurrentChapterTextCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = widget->get_current_chapter_text();
    }
};

class GetCurrentPageTextCommand : public Command {

public:
    static inline const std::string cname = "get_current_page_text";
    static inline const std::string hname = "";
    GetCurrentPageTextCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        result = widget->get_current_page_text();
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

class CopyCommand : public Command {
public:
    static inline const std::string cname = "copy";
    static inline const std::string hname = "Copy";
    CopyCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        QString chat_selected_text = widget->get_selected_text_in_chat_window();
        if (chat_selected_text.size() > 0) {
            copy_to_clipboard(chat_selected_text.toStdWString());
            return;
        }

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

class CopyAllTextCommand : public Command {
public:
    static inline const std::string cname = "copy_all_text";
    static inline const std::string hname = "Copy all the text in the document.";
    CopyAllTextCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        copy_to_clipboard(widget->doc()->get_super_fast_index());
    }
};


class CopyCurrentChapterTextCommand : public Command {
public:
    static inline const std::string cname = "copy_current_chapter_text";
    static inline const std::string hname = "Copy current chapter's text";
    CopyCurrentChapterTextCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        std::wstring chapter_text = widget->get_current_chapter_text();
        copy_to_clipboard(chapter_text);

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

class ToggleShowLastCommand : public Command {
public:
    static inline const std::string cname = "toggle_show_last_command";
    static inline const std::string hname = "Toggle whether the last command is shown in statusbar";
    ToggleShowLastCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->should_show_last_command = !widget->should_show_last_command;
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

class OpenEmbeddedExternalTextEditorCommand : public Command {
public:
    static inline const std::string cname = "open_embedded_external_text_editor";
    static inline const std::string hname = "Edit the current text input in the configured embedded external text editor";

    OpenEmbeddedExternalTextEditorCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->open_embedded_external_text_editor();
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

class ToggleLineSelectCursor : public Command {
public:
    static inline const std::string cname = "toggle_line_select_cursor";
    static inline const std::string hname = "Swap between begin/end of line selection.";
    ToggleLineSelectCursor(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->main_document_view->swap_line_select_cursor();
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

class CopyLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "copy_link";
    static inline const std::string hname = "Copy URL of PDF links using keyboard";
    CopyLinkCommand(MainWidget* w) : OpenLinkCommand(cname, w) {};

    void perform_with_link(PdfLink link) {
        widget->handle_open_link(link, true);
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

class JsConfigCommand : public Command {
public:
    static inline const std::string cname = "javascript_config";
    static inline const std::string hname = "Opens the javascript config file.";
    JsConfigCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->open_file(sioyek_js_path.get_path(), true);
    }

    bool requires_document() { return false; }
};

class KeysCommand : public Command {
public:
    static inline const std::string cname = "keys";
    static inline const std::string hname = "Open the default keys config file";
    KeysCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->open_file(default_keys_path.get_path(), true);
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
            widget->open_file(key_file_path.value().get_path(), true);
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
        widget->open_file(default_config_path.get_path(), true);
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
            widget->open_file(pref_file_path.value().get_path(), true);
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

class ToggleMouseRulerCommand : public Command {
public:
    static inline const std::string cname = "toggle_mouse_ruler";
    static inline const std::string hname = "Toggle mouse ruler mode";

    ToggleMouseRulerCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->toggle_mouse_ruler_mode();
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

class SetStatusStringCommand : public Command {
public:
    static inline const std::string cname = "set_status_string";
    static inline const std::string hname = "Set custom message to be shown in statusbar";
    SetStatusStringCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<std::wstring> text = {};
    std::optional<std::wstring> id = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!text.has_value()) {
            return Requirement{ RequirementType::Text, "Status String" };
        }
        if (!id.has_value()) {
            return Requirement{ RequirementType::OptionalText, "Status ID" };
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        if (!text.has_value()) {
            text = value;
        }
        else {
            id = value;
        }
    }

    void perform() {
        QString res_id = widget->set_status_message(text.value(), QString::fromStdWString(id.value_or(L"")));
        result = res_id.toStdWString();
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
    std::optional<std::wstring> id = {};

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        if (!id.has_value()) {
            return Requirement{ RequirementType::OptionalText, "Status ID" };
        }
        return {};
    }

    void set_text_requirement(std::wstring value) {
        id = value;
    }

    void perform() {
        widget->set_status_message(L"", QString::fromStdWString(id.value_or(L"")));
    }

    bool requires_document() { return false; }
};

class NextStatusMessageCommand : public Command {
public:
    static inline const std::string cname = "next_status_message";
    static inline const std::string hname = "Show the next status message in statusbar";
    NextStatusMessageCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int current_index = widget->current_status_message_index;
        int next_index = (current_index + 1) % (widget->status_messages.size());
        widget->current_status_message_index = next_index;
    }

    bool requires_document() { return false; }
};

class PrevStatusMessageCommand : public Command {
public:
    static inline const std::string cname = "prev_status_message";
    static inline const std::string hname = "Show the previous status message in statusbar";
    PrevStatusMessageCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int current_index = widget->current_status_message_index;
        int prev_index = (current_index + widget->status_messages.size() - 1) % (widget->status_messages.size());
        widget->current_status_message_index = prev_index;
    }

    bool requires_document() { return false; }
};

class ClearCurrentStatusMessageCommand : public Command {
public:
    static inline const std::string cname = "clear_current_status_message";
    static inline const std::string hname = "Clear the current status message";
    ClearCurrentStatusMessageCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        int current_index = widget->current_status_message_index;
        if (current_index >= 0 && current_index < widget->status_messages.size()) {
            widget->status_messages.erase(widget->status_messages.begin() + current_index);
            current_index = std::min(current_index, (int)widget->status_messages.size() - 1);
            widget->current_status_message_index = current_index;
        }
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

class DocumentationSearchCommand : public Command {
public:
    static inline const std::string cname = "documentation_search";
    static inline const std::string hname = "Search sioyek's documentation";

    DocumentationSearchCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_documentation_search();
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
    
    bool requires_document() { return false; }

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

class RepeatLastCommandCommand : public Command {
public:
    static inline const std::string cname = "repeat_last_command";
    static inline const std::string hname = "Repeat the last command executed.";
    RepeatLastCommandCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->repeat_last_command();
    }
};

void register_misc_commands(CommandManager* manager) {
    register_command<SendSymbolCommand>(manager);
    register_command<ToggleTextMarkCommand>(manager);
    register_command<MoveTextMarkForwardCommand>(manager);
    register_command<MoveTextMarkDownCommand>(manager);
    register_command<MoveTextMarkUpCommand>(manager);
    register_command<MoveTextMarkForwardCommand>(manager);
    register_command<MoveTextMarkBackwardCommand>(manager);
    register_command<MoveTextMarkBackwardWordCommand>(manager);
    register_command<MoveTextMarkForwardWordCommand>(manager);
    register_command<SetTtsVoiceCommand>(manager);
    register_command<IncreaseTtsRateCommand>(manager);
    register_command<DecreaseTtsRateCommand>(manager);
    register_command<StopReadingCommand>(manager);
    register_command<ToggleReadingCommand>(manager);
    register_command<AddKeybindCommand>(manager);
    register_command<TypeTextCommand>(manager);
    register_command<ControlMenuCommand>(manager);
    register_command<ExecuteMacroCommand>(manager);
    register_command<GetConfigNoDialogCommand>(manager);
    register_command<GetConfigCommand>(manager);
    register_command<ShowTextPromptCommand>(manager);
    register_command<GetModeString>(manager);
    register_command<GetStateJsonCommand>(manager);
    register_command<GetPaperNameCommand>(manager);
    register_command<GetOverviewPaperName>(manager);
    register_command<ToggleRectHintsCommand>(manager);
    register_command<GetAnnotationsJsonCommand>(manager);
    register_command<ShowOptionsCommand>(manager);
    register_command<RenameCommand>(manager);
    register_command<CopyScreenshotToClipboard>(manager);
    register_command<CopyScreenshotToScratchpad>(manager);
    register_command<ToggleWindowConfigurationCommand>(manager);
    register_command<CommandPaletteCommand>(manager);
    register_command<RegisterUrlHandler>(manager);
    register_command<UnregisterUrlHandler>(manager);
    register_command<CommandCommand>(manager);
    register_command<ScreenshotCommand>(manager);
    register_command<FramebufferScreenshotCommand>(manager);
    register_command<ExportDefaultConfigFile>(manager);
    register_command<ExportCommandNamesCommand>(manager);
    register_command<ExportConfigNamesCommand>(manager);
    register_command<WaitCommand>(manager);
    register_command<WaitForIndexingToFinishCommand>(manager);
    register_command<WaitForRendersToFinishCommand>(manager);
    register_command<WaitForSearchToFinishCommand>(manager);
    register_command<WaitForDownloadsToFinish>(manager);
    register_command<CopyCommand>(manager);
    register_command<CopyAllTextCommand>(manager);
    register_command<CopyCurrentChapterTextCommand>(manager);
    register_command<PrintNonDefaultConfigs>(manager);
    register_command<PrintUndocumentedCommandsCommand>(manager);
    register_command<PrintUndocumentedConfigsCommand>(manager);
    register_command<ToggleShowLastCommand>(manager);
    register_command<OpenSelectedUrlCommand>(manager);
    register_command<ShowContextMenuCommand>(manager);
    register_command<ShowCustomContextMenuCommand>(manager);
    register_command<OpenExternalTextEditorCommand>(manager);
    register_command<CloseWindowCommand>(manager, "q");
    register_command<NewWindowCommand>(manager);
    register_command<QuitCommand>(manager);
    register_command<EscapeCommand>(manager);
    register_command<StartReadingCommand>(manager);
    register_command<CopyLinkCommand>(manager);
    register_command<KeyboardSelectCommand>(manager);
    register_command<KeyboardOverviewCommand>(manager);
    register_command<KeysCommand>(manager);
    register_command<JsConfigCommand>(manager);
    register_command<KeysUserCommand>(manager);
    register_command<KeysUserAllCommand>(manager);
    register_command<PrefsCommand>(manager);
    register_command<PrefsUserCommand>(manager);
    register_command<PrefsUserAllCommand>(manager);
    register_command<KeyboardSelectLineCommand>(manager);
    register_command<ToggleLineSelectCursor>(manager);
    register_command<OpenContainingFolderCommand>(manager);
    register_command<SynctexUnderCursorCommand>(manager);
    register_command<SynctexUnderRulerCommand>(manager);
    register_command<VisualMarkUnderCursorCommand>(manager, "ruler_under_cursor");
    register_command<RulerUnderSelectedPointCommand>(manager);
    register_command<ToggleMouseRulerCommand>(manager);
    register_command<StartMobileTextSelectionAtPointCommand>(manager);
    register_command<EnterPasswordCommand>(manager);
    register_command<ToggleFastreadCommand>(manager);
    register_command<ReloadCommand>(manager);
    register_command<ReloadNoFlickerCommand>(manager);
    register_command<ReloadConfigCommand>(manager);
    register_command<SetStatusStringCommand>(manager);
    register_command<ClearStatusStringCommand>(manager);
    register_command<NextStatusMessageCommand>(manager);
    register_command<PrevStatusMessageCommand>(manager);
    register_command<ClearCurrentStatusMessageCommand>(manager);
    register_command<ToggleTittlebarCommand>(manager);
    register_command<SetWindowRectCommand>(manager);
    register_command<ScanNewFilesFromScanDirCommand>(manager);
    register_command<DebugCommand>(manager);
    register_command<SetConfigCommand>(manager);
    register_command<ToggleConfigWithNameCommand>(manager);
    register_command<SaveConfigWithNameCommand>(manager);
    register_command<DocumentationSearchCommand>(manager);
    register_command<CollapseMenuCommand>(manager);
    register_command<ShowTouchMainMenu>(manager);
    register_command<ShowTouchPageSelectCommand>(manager);
    register_command<ShowTouchHighlightTypeSelectCommand>(manager);
    register_command<ShowTouchDrawingMenu>(manager);
    register_command<ShowTouchSettingsMenu>(manager);
    register_command<ExportPythonApiCommand>(manager);
    register_command<SelectRectCommand>(manager);
    register_command<ToggleTypingModeCommand>(manager);
    register_command<DonateCommand>(manager);
    register_command<ImportLocalDatabaseCommand>(manager);
    register_command<ImportSharedDatabaseCommand>(manager);
    register_command<ImportCommand>(manager);
    register_command<ExportCommand>(manager);
    register_command<OpenLocalDatabaseContainingFolder>(manager);
    register_command<OpenSharedDatabaseContainingFolder>(manager);
    register_command<SetPageOffsetCommand>(manager);
    register_command<ExecuteCommand>(manager, "execute_shell_command");
    register_command<CopyWindowSizeConfigCommand>(manager);
    register_command<AddMarkedDataCommand>(manager);
    register_command<UndoMarkedDataCommand>(manager);
    register_command<RemoveMarkedDataCommand>(manager);
    register_command<ExportMarkedDataCommand>(manager);
    register_command<ShowTouchConfigCommand>(manager);
    register_command<RepeatLastCommandCommand>(manager);
    register_command<GetCurrentDocumentTextCommand>(manager);
    register_command<GetCurrentChapterTextCommand>(manager);
    register_command<GetCurrentPageTextCommand>(manager);

#ifdef Q_OS_WIN
    register_command<OpenEmbeddedExternalTextEditorCommand>(manager);
#endif
}
