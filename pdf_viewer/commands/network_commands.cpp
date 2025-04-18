#include "commands/network_commands.h"
#include "main_widget.h"
#include "document_view.h"
#include "network_manager.h"
#include <qclipboard.h>
#include <qdesktopservices.h>
#include "config.h"
#include "ui.h"

extern std::wstring EXTRACT_TABLE_PROMPT;
extern int TABLE_EXTRACT_BEHAVIOUR;
extern bool TOUCH_MODE;
extern std::wstring SIOYEK_HOST;

class ProCommand : public Command {
public:
    static inline const bool requires_pro = true;
    ProCommand(std::string cname, MainWidget* w) : Command(cname, w) {};

};

class ProTextCommand : public TextCommand {
public:
    static inline const bool requires_pro = true;
    ProTextCommand(std::string cname, MainWidget* w) : TextCommand(cname, w) {};

};

class StartReadingHighQualityCommand : public ProCommand {
public:
    static inline const std::string cname = "start_reading_high_quality";
    static inline const std::string hname = "Read using sioyek servers' text to speech";
    StartReadingHighQualityCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->handle_start_reading_high_quality(true);
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

// class SemanticSearchCommand : public ProTextCommand {
// public:
//     static inline const std::string cname = "semantic_search";
//     static inline const std::string hname = "";

//     SemanticSearchCommand(MainWidget* w) : ProTextCommand(cname, w) {
//     };

//     void perform() {
//         widget->handle_semantic_search(text.value());
//     }

//     std::string text_requirement_name() {
//         return "Search text";
//     }

// };

// class SemanticSearchExtractiveCommand : public ProTextCommand {
// public:
//     static inline const std::string cname = "semantic_search_extractive";
//     static inline const std::string hname = "";

//     SemanticSearchExtractiveCommand(MainWidget* w) : ProTextCommand(cname, w) {
//     };

//     void perform() {
//         widget->handle_semantic_search_extractive(text.value());
//     }

//     std::string text_requirement_name() {
//         return "Search text";
//     }

// };

class LlmCommand : public ProCommand {
private:
    std::optional<std::wstring> system_prompt = {};
    std::optional<std::wstring> user_prompt = {};
    std::optional<AbsoluteRect> selected_rect = {};
public:
    static inline const std::string cname = "llm_command";
    static inline const std::string hname = "";
    LlmCommand(MainWidget* w) : ProCommand(cname, w) {};

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

class DownloadPaperWithNameCommand : public ProTextCommand {
public:
    static inline const std::string cname = "download_paper_with_name";
    static inline const std::string hname = "";
    DownloadPaperWithNameCommand(MainWidget* w) : ProTextCommand(cname, w) {};

    void perform() {
        widget->download_paper_with_name(text.value(), "", PaperDownloadFinishedAction::OpenInNewWindow);

    }

    std::string text_requirement_name() {
        return "Paper Name";
    }

};

class ExtractTableWithPromptCommand : public ProCommand {

public:
    static inline const std::string cname = "extract_table_with_prompt";
    static inline const std::string hname = "Extract the selected table's data";

    std::optional<AbsoluteRect> rect_;
    std::optional<QString> bookmark_type_;
    std::optional<QString> prompt_;

    ExtractTableWithPromptCommand(MainWidget* w) : ProCommand(cname, w) {};

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

class ConvertToLatexCommand : public ProCommand {

public:
    static inline const std::string cname = "convert_to_latex";
    static inline const std::string hname = "Convert the selected image into latex";

    std::optional<AbsoluteRect> rect_;

    ConvertToLatexCommand(MainWidget* w) : ProCommand(cname, w) {};

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

class ExtractTableCommand : public ProCommand {

public:
    static inline const std::string cname = "extract_table";
    static inline const std::string hname = "Extract the selected table's data";

    std::optional<AbsoluteRect> rect_;

    ExtractTableCommand(MainWidget* w) : ProCommand(cname, w) {};

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

class OpenServerOnlyFile : public ProCommand {
public:
    static inline const std::string cname = "open_server_only_file";
    static inline const std::string hname = "Search and open files located only in sioyek servers";
    OpenServerOnlyFile(MainWidget* w) : ProCommand(cname, w) {};

    std::wstring file_name;

    bool pushes_state() {
        return true;
    }

    void perform() {
        widget->handle_open_server_only_file();
    }

    bool requires_document() { return false; }
};

class DownloadLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "download_link";
    static inline const std::string hname = "Download the destination reference of a PDF link";
    static inline const bool requires_pro = true;
    DownloadLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform_with_link(PdfLink link) {
        ParsedUri uri = widget->doc()->parse_link(link);
        PdfLinkTextInfo link_info = widget->doc()->get_pdf_link_text(link);
        AbsoluteRect link_source_rect = DocumentRect{ link.rects[0], link.source_page }.to_absolute(widget->doc());

        std::optional<std::pair<QString, std::vector<PagelessDocumentRect>>> reftext = widget->doc()->get_page_bib_with_reference(uri.page - 1, link_info.link_text);
        if (reftext.has_value()) {
            QString paper_name = get_paper_name_from_reference_text(reftext->first);
            widget->download_and_portal(paper_name.toStdWString(), reftext->first, link_source_rect.center());
        }
        widget->reset_highlight_links();
        widget->clear_tag_prefix();
    }

    std::string get_name() {
        return cname;
    }
};

class DownloadPaperUnderCursorCommand : public ProCommand {
public:
    static inline const std::string cname = "download_paper_under_cursor";
    static inline const std::string hname = "Try to download the paper name under cursor";
    DownloadPaperUnderCursorCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->download_paper_under_cursor();
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

class CitersCommand : public ProCommand {
public:
    static inline const std::string cname = "citers";
    static inline const std::string hname = "Show a list of citers of this paper";

    CitersCommand(MainWidget* w) : ProCommand(cname, w) {};
    
    void perform() {
        widget->show_citers_of_current_paper();
    }
};

class MagicDrawingAskCommand : public ProCommand {
public:
    static inline const std::string cname = "magic_drawing_ask";
    static inline const std::string hname = "Use an LLM to answer the drawing question.";

    MagicDrawingAskCommand(MainWidget* w) : ProCommand(cname, w) {};
    
    void perform() {
        widget->ai_magic_drawing_ask();
    }
};

class ResumeToServerLocationCommand : public ProCommand {
public:
    static inline const std::string cname = "resume_to_server";
    static inline const std::string hname = "Jump to the location of current document in sioyek servers";

    ResumeToServerLocationCommand(MainWidget* w) : ProCommand(cname, w) {};

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

class SynchronizeCommand : public ProCommand {
public:
    static inline const std::string cname = "synchronize";
    static inline const std::string hname = "Synchronize a desyncronized file with server";
    SynchronizeCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->synchronize_if_desynchronized();
    }

};

class DashboardCommand : public Command {
public:
    static inline const std::string cname = "dashboard";
    static inline const std::string hname = "Open the user dashboard in sioyek website";
    DashboardCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        open_web_url(SIOYEK_HOST + widget->sioyek_network_manager->SIOYEK_DASHBOARD_URL_);
    }

};

class ForceDownloadAnnotations : public ProCommand {
public:
    static inline const std::string cname = "force_download_annotations";
    static inline const std::string hname = "Download all annotations from the server into local database";
    ForceDownloadAnnotations(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->sioyek_network_manager->download_annotations_since_last_sync(true);
    }

};

class UploadCurrentFileCommand : public ProCommand {
public:
    static inline const std::string cname = "upload_current_file";
    static inline const std::string hname = "Upload the current file to sioyek servers";
    UploadCurrentFileCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->upload_current_file();
    }

};

class OcrCurrentFileCommand : public ProCommand {
public:
    static inline const std::string cname = "ocr";
    static inline const std::string hname = "Create a new pdf ocr of current file";
    OcrCurrentFileCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->ocr_current_file();
    }

};

class DeleteCurrentFileFromServer : public ProCommand {
public:
    static inline const std::string cname = "delete_current_file_from_server";
    static inline const std::string hname = "Delete the current file from sioyek servers";
    DeleteCurrentFileFromServer(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->delete_current_file_from_server();
    }

};

class ResyncDocumentCommand : public ProCommand {
public:
    static inline const std::string cname = "resync_document";

    static inline const std::string hname = "Re-sync current document's annotations with sioyek servers";
    ResyncDocumentCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->handle_sync_open_document();
    }

};

class DownloadUnsyncedFilesCommand : public ProCommand {
public:
    static inline const std::string cname = "download_unsynced_files";
    static inline const std::string hname = "Download unsynced files from sioyek servers";
    DownloadUnsyncedFilesCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->sioyek_network_manager->download_unsynced_files(widget, widget->db_manager);
    }

};

class SyncCurrentFileLocation : public ProCommand {
public:
    static inline const std::string cname = "sync_current_file_location";
    static inline const std::string hname = "Sync the current file location to sioyek servers";
    SyncCurrentFileLocation(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->sync_current_file_location_to_servers();
    }
};

class AskCommand : public ProCommand {
public:
    static inline const std::string cname = "ask";
    static inline const std::string hname = "Ask a question from the current document.";
    AskCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->handle_ask();
    }
};

class DownloadOverviewPaperCommand : public ProTextCommand {
public:
    static inline const std::string cname = "download_overview_paper";
    static inline const std::string hname = "Download the referenced paper overview window";
    std::optional<AbsoluteRect> source_rect = {};
    std::wstring src_doc_path;
    QString full_bib_text;

    DownloadOverviewPaperCommand(MainWidget* w) : ProTextCommand(cname, w) {};

    void perform() {

        std::wstring text_ = text.value();

        if (source_rect) {
            widget->download_and_portal(text_, full_bib_text, source_rect->center());
        }
        else {
            widget->download_paper_with_name(text_, full_bib_text);
        }

    }

    void pre_perform() {
        std::optional<QString> paper_name = widget->main_document_view->get_overview_paper_name(&full_bib_text);
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

class DownloadOverviewPaperNoPrompt : public ProCommand {
public:
    static inline const std::string cname = "download_overview_paper_no_prompt";
    static inline const std::string hname = "Download and portal to the current highlighted overview paper";
    DownloadOverviewPaperNoPrompt(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->download_and_portal_to_highlighted_overview_paper();
        widget->close_overview();
    }
};

class LoadAnnotationsFileSyncDeletedCommand : public ProCommand {
public:
    static inline const std::string cname = "import_annotations_file_sync_deleted";
    static inline const std::string hname = "";
    LoadAnnotationsFileSyncDeletedCommand(MainWidget* w) : ProCommand(cname, w) {};

    void perform() {
        widget->doc()->load_annotations(true);
    }

};

void register_network_commands(CommandManager* manager) {
    register_command<StartReadingHighQualityCommand>(manager);
    register_command<DownloadClipboardUrlCommand>(manager);
    register_command<DownloadPaperWithUrlCommand>(manager);
    // register_command<SemanticSearchCommand>(manager);
    // register_command<SemanticSearchExtractiveCommand>(manager);
    register_command<LlmCommand>(manager);
    register_command<DownloadPaperWithNameCommand>(manager);
    register_command<ExtractTableWithPromptCommand>(manager);
    register_command<ConvertToLatexCommand>(manager);
    register_command<ExtractTableCommand>(manager);
    register_command<OpenServerOnlyFile>(manager);
    register_command<DownloadLinkCommand>(manager);
    register_command<DownloadPaperUnderCursorCommand>(manager);
    register_command<LoginWithGoogleCommand>(manager);
    register_command<LoginCommand>(manager);
    register_command<LogoutCommand>(manager);
    register_command<CancelAllDownloadsCommand>(manager);
    register_command<CitersCommand>(manager);
    register_command<MagicDrawingAskCommand>(manager);
    register_command<ResumeToServerLocationCommand>(manager);
    register_command<LoginUsingAccessTokenCommand>(manager);
    register_command<SynchronizeCommand>(manager);
    register_command<ForceDownloadAnnotations>(manager);
    register_command<UploadCurrentFileCommand>(manager);
    register_command<OcrCurrentFileCommand>(manager);
    register_command<DeleteCurrentFileFromServer>(manager);
    register_command<ResyncDocumentCommand>(manager);
    register_command<DownloadUnsyncedFilesCommand>(manager);
    register_command<SyncCurrentFileLocation>(manager);
    register_command<AskCommand>(manager);
    register_command<DownloadOverviewPaperCommand>(manager);
    register_command<DownloadOverviewPaperNoPrompt>(manager);
    register_command<LoadAnnotationsFileSyncDeletedCommand>(manager);
    register_command<DashboardCommand>(manager);
}
