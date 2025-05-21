#include "commands/annotation_commands.h"
#include "main_widget.h"
#include "document_view.h"
#include "ui.h"

extern float FREETEXT_BOOKMARK_COLOR[3];
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern bool TOUCH_MODE;
extern wchar_t FREEHAND_TYPE;
extern float FREEHAND_SIZE;

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

class SetFreehandThickness : public MacroCommand{
public:
    static inline const std::string cname = "set_freehand_thickness";
    static inline const std::string hname = "Set thickness of freehand drawings";
    SetFreehandThickness(MainWidget* w) : MacroCommand(w, w->command_manager, cname, L"set_config(freehand_drawing_size)"){

    }
};

class SetFreehandType : public MacroCommand{
public:
    static inline const std::string cname = "set_freehand_type";
    static inline const std::string hname = "Set the freehand drawing color type";
    SetFreehandType(MainWidget* w) : MacroCommand(w, w->command_manager, cname, L"set_config(freehand_drawing_type)"){

    }
};

class AddBookmarkCommand : public TextCommand {
public:
    static inline const std::string cname = "add_bookmark";
    static inline const std::string hname = "Add an invisible bookmark in the current location";
    AddBookmarkCommand(MainWidget* w) : TextCommand(cname, w) {}

    void perform() {
        std::string uuid = widget->main_document_view->add_bookmark(text.value());
        widget->handle_special_bookmarks(text.value(), utf8_decode(uuid));
        widget->on_new_bookmark_added(uuid);
        result = utf8_decode(uuid);
    }


    std::string text_requirement_name() {
        return "Bookmark Text";
    }

    QStringList get_autocomplete_options() override {
        return {
            "#markdown",
            "#latex",
            "#shell",
            "@chapter",
            "@selection"
        };
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

    QStringList get_autocomplete_options() override {
        return {
            "#markdown",
            "#latex",
            "#shell",
            "@chapter",
            "@selection"
        };
    }

    void perform() {
        if (text_->size() > 0) {
            std::string uuid = widget->doc()->add_pending_bookmark(pending_uuid, text_.value());
            widget->handle_special_bookmarks(text_.value(), utf8_decode(uuid));
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
        QList<FreehandDrawing> drawings;
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

class AddBookmarkFreetextCommand : public Command {

public:
    static inline const std::string cname = "add_freetext_bookmark";
    static inline const std::string hname = "Add a text bookmark in the selected rectangle";

    std::optional<std::wstring> text_;
    std::optional<AbsoluteRect> rect_;
    std::string pending_uuid = "";
    QString status_message_uuid;

    AddBookmarkFreetextCommand(MainWidget* w) : Command(cname, w) {
        status_message_uuid = QString::fromStdWString(new_uuid());
    };

    void on_text_change(const QString& new_text) override {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_bookmark_uuid);

        if (bookmark) {
            bookmark->description = new_text.toStdWString();
        }
        //if (new_text.startsWith())
        if (bookmark->is_question() || bookmark->is_summary()) {
            widget->update_query_tokens_status_message_for_bookmark(status_message_uuid);
        }
        else {
            widget->set_status_message(L"", status_message_uuid);
        }
    }

    QStringList get_autocomplete_options() override {
        return {
            "#markdown",
            "#latex",
            "#shell",
            "@chapter",
            "@selection"
        };
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


    std::optional<AbsoluteRect> get_text_editor_rectangle() override {
        return rect_;
    }

    void on_cancel() {

        if (pending_uuid.size() > 0) {
            widget->doc()->undo_pending_bookmark(pending_uuid);
        }
        widget->set_status_message(L"", status_message_uuid);
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
    bool has_pre_performed = false;


    std::optional<Requirement> next_requirement(MainWidget* widget) override {
        if (rect.has_value()) {
            return {};
        }
        return Requirement { RequirementType::Symbol, "Bookmark Location" };
    }

    void set_symbol_requirement(char value) override {
        if (!has_pre_performed) {
            populate_rects();
        }

        std::string tag;
        tag.push_back(value);
        int index = get_index_from_tag(tag);
        if (index < possible_targets.size() && index >= 0) {
            rect = possible_targets[index];
        }

    }

    void populate_rects() {
        auto largest_rects = widget->get_largest_empty_rects();
        for (auto r : largest_rects) {
            possible_targets.push_back(r.to_absolute(dv()).to_document(widget->doc()));
        }
    }

    void pre_perform() override {
        has_pre_performed = true;
        populate_rects();

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

class OpenChatBookmarkCommand : public Command {
public:
    static inline const std::string cname = "goto_chat_bookmark";
    static inline const std::string hname = "Open the bookmark list of question bookmarks, opens that chat window of selected bookmark.";
    OpenChatBookmarkCommand(MainWidget* w) : Command(cname, w) {};
    std::string bookmark_uuid = "";

    void set_generic_requirement(QVariant uuid) override{
        bookmark_uuid = uuid.toString().toStdString();
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) override{
        if (bookmark_uuid.size() == 0) {
            return Requirement{ RequirementType::Generic, "bookmark" };
        }
        return {};
    }

    void handle_generic_requirement() {
        widget->handle_goto_bookmark(false, true);
    }

    void perform() override {
        widget->main_document_view->set_selected_bookmark_uuid(bookmark_uuid);
        widget->open_selected_bookmark_in_widget();
    }
};

class GotoManualBookmarkCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "goto_manual_bookmark";
    static inline const std::string hname = "Open the bookmark list of current document, the automatic bookmarks will not be displayed.";
    GotoManualBookmarkCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark(true);
    }
};

class AcceptNewBookmarkMessageCommand : public GenericGotoLocationCommand {
public:
    static inline const std::string cname = "accept_new_bookmark_message";
    static inline const std::string hname = "When the bookmark widget is opened, accept the text input as a new question.";
    AcceptNewBookmarkMessageCommand(MainWidget* w) : GenericGotoLocationCommand(cname, w) {};

    void handle_generic_requirement() {
        widget->accept_new_bookmark_message();
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

class GotoManualBookmarkGlobalCommand : public GenericPathAndLocationCommadn {
public:
    static inline const std::string cname = "goto_manual_bookmark_g";
    static inline const std::string hname = "Open the bookmark list of all documents, ignoring automatic bookmarks";

    GotoManualBookmarkGlobalCommand(MainWidget* w) : GenericPathAndLocationCommadn(cname, w) {};

    void handle_generic_requirement() {
        widget->handle_goto_bookmark_global(true);
    }
    bool requires_document() { return false; }
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

class PortalCommand : public Command {
public:
    static inline const std::string cname = "portal";
    static inline const std::string hname = "Start creating a portal";
    PortalCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_portal();
    }
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

class PortalToDefinitionCommand : public Command {
public:
    static inline const std::string cname = "portal_to_definition";
    static inline const std::string hname = "Create a portal to the definition in current highlighted line";
    PortalToDefinitionCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->portal_to_definition();
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

class OpenSelectedBookmarkInWidget : public Command {
public:
    static inline const std::string cname = "open_selected_bookmark_in_widget";
    static inline const std::string hname = "Open the contents of the selected bookmark in a widget.";

    bool use_keyboard = false;

    OpenSelectedBookmarkInWidget(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->open_selected_bookmark_in_widget();

    }

};

class EditSelectedBookmarkCommand : public TextCommand {
public:
    static inline const std::string cname = "edit_selected_bookmark";
    static inline const std::string hname = "Edit selected bookmark";
    std::wstring initial_text;
    float initial_font_size;
    std::string uuid = "";

    std::optional<QString> last_text = {};

    EditSelectedBookmarkCommand(MainWidget* w) : TextCommand(cname, w) {};

    void on_text_change(const QString& new_text) override {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        //widget->doc()->get_bookmarks()[selected_bookmark_index].description = new_text.toStdWString();
        BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_bookmark_uuid);
        if (bookmark) {
            bookmark->description = new_text.toStdWString();
        }
        if (last_text.has_value()) {
            if ((last_text->size() == new_text.size() - 1) && new_text.startsWith(last_text.value())) {
                widget->scroll_selected_bookmark_to_end();
            }
        }
        last_text = new_text;
    }

    void pre_perform() {

        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_bookmark_uuid);
        if (bookmark) {
            initial_text = bookmark->description;
            initial_font_size = bookmark->font_size;
            uuid = selected_bookmark_uuid;

            widget->set_current_text_editor_text(bookmark->get_desc_qstring());
        }
    }

    void on_cancel() {
        if (uuid.size() > 0) {
            BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(uuid);
            bookmark->description = initial_text;
            bookmark->font_size = initial_font_size;
        }
    }

    std::optional<AbsoluteRect> get_text_editor_rectangle() override {
        if (uuid.size() > 0) {
            auto bm = widget->doc()->get_bookmark_with_uuid(uuid);
            if (bm) {
                if (bm->is_freetext()) {
                    return bm->get_rectangle();
                }
            }
        }
        return {};
    }

    std::optional<Requirement> next_requirement(MainWidget* widget) {
        std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        if (selected_bookmark_uuid.size() == 0) return {};
        return TextCommand::next_requirement(widget);
    }

    void perform() {
        //std::string selected_bookmark_uuid = dv()->get_selected_bookmark_uuid();
        if (uuid.size() > 0) {
            std::wstring text_ = text.value();
            widget->change_bookmark_text(uuid, text_);
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
            widget->change_highlight_text_annot(uuid, text_);
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
        if (get_selected_item_uuid().size() == 0) {
            visible_item_uuids = get_visible_item_uuids();
            n_required_tags = get_num_tag_digits(visible_item_uuids.size());
        }

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
        widget->dv()->handle_generic_tags_pre_perform(visible_objects);

    }

    void perform_with_selected_index(std::optional<int> index) override {
        if (index && index.value() < visible_objects.size()) {
            VisibleObjectIndex object_index = visible_objects[index.value()];
            if (object_index.object_type == VisibleObjectType::Highlight) {
                widget->dv()->set_selected_highlight_uuid(object_index.uuid);
            }
            if (object_index.object_type == VisibleObjectType::Bookmark) {
                widget->dv()->set_selected_bookmark_uuid(object_index.uuid);
            }
            if (object_index.object_type == VisibleObjectType::Portal) {
                widget->dv()->set_selected_portal_uuid(object_index.uuid);
            }
            if (object_index.object_type == VisibleObjectType::PinnedPortal) {
                widget->dv()->set_selected_portal_uuid(object_index.uuid, true);
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

class GotoBookmarkLinks : public GenericVisibleBookmarkCommand {

public:
    static inline const std::string cname = "goto_bookmark_links";
    static inline const std::string hname = "Go to the internal reference links in the bookmark.";
    GotoBookmarkLinks(MainWidget* w) : GenericVisibleBookmarkCommand(cname, w) {};

    bool pushes_state() override {
        return true;
    }

    void perform_with_bookmark_selected() override {
        std::vector<std::wstring> queries;

        std::string selected_bookmark_uuid = widget->main_document_view->get_selected_bookmark_uuid();
        BookMark* bookmark = widget->doc()->get_bookmark_with_uuid(selected_bookmark_uuid);
        // regex to match markdown links
        QRegularExpression link_regex = QRegularExpression("\\[\\[([^\\]]+)\\]\\]\\(sioyek://([^\\)]+)\\)");
        if (bookmark && bookmark->is_freetext()) {
            QString text = QString::fromStdWString(bookmark->description);
            QRegularExpressionMatchIterator i = link_regex.globalMatch(text);
            while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                QString link_text = match.captured(1);
                QString link = match.captured(2);
                link = QUrl::fromPercentEncoding(link.toUtf8());
                queries.push_back(link.toStdWString());
            }

        }
        widget->main_document_view->perform_fuzzy_searches(queries);
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

class PortalToLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "portal_to_link";
    static inline const std::string hname = "Create a portal to PDF links using keyboard";
    PortalToLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform_with_link(PdfLink pdf_link) {
        ParsedUri parsed_uri = parse_uri(widget->mupdf_context, widget->doc()->doc, pdf_link.uri);

        AbsoluteDocumentPos src_abspos = DocumentPos{ pdf_link.source_page, 0, pdf_link.rects[0].y0 }.to_absolute(widget->doc());
        AbsoluteDocumentPos dst_abspos = DocumentPos{ parsed_uri.page - 1, parsed_uri.x, parsed_uri.y }.to_absolute(widget->doc());

        Portal portal;
        portal.dst.document_checksum = widget->doc()->get_checksum();
        portal.dst.book_state.offset_x = dst_abspos.x;
        portal.dst.book_state.offset_y = dst_abspos.y;
        portal.dst.book_state.zoom_level = widget->main_document_view->get_zoom_level();
        portal.src_offset_y = src_abspos.y;
        std::string uuid = widget->doc()->add_portal(portal, true);
        widget->on_new_portal_added(uuid);
    }

    std::string get_name() {
        return cname;
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

class PortalToOverviewCommand : public Command {
public:
    static inline const std::string cname = "portal_to_overview";
    static inline const std::string hname = "Create a portal to the current overview location";
    PortalToOverviewCommand(MainWidget* w) : Command(cname, w) {};
    void perform() {
        widget->handle_portal_to_overview();
    }

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

class UndoDeleteCommand : public Command {
public:
    inline static const std::string cname = "undo_delete";
    inline static const std::string hname = "Undo deleted object";

    UndoDeleteCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->undo_delete();
    }

};

class ScrollSelectedBookmarkToStart : public Command {
public:
    static inline const std::string cname = "scroll_selected_bookmark_to_start";
    static inline const std::string hname = "";
    ScrollSelectedBookmarkToStart(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_scroll_selected_bookmark_to_ends(true);
    }
};

class ScrollSelectedBookmarkToEnd : public Command {
public:
    static inline const std::string cname = "scroll_selected_bookmark_to_end";
    static inline const std::string hname = "";
    ScrollSelectedBookmarkToEnd(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_scroll_selected_bookmark_to_ends(false);
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

class ScrollSelectedBookmarkPageDown : public Command {
public:
    static inline const std::string cname = "scroll_selected_bookmark_page_down";
    static inline const std::string hname = "";
    ScrollSelectedBookmarkPageDown(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->scroll_selected_bookmark(10);
    }
};

class ScrollSelectedBookmarkPageUp : public Command {
public:
    static inline const std::string cname = "scroll_selected_bookmark_page_up";
    static inline const std::string hname = "";
    ScrollSelectedBookmarkPageUp(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->scroll_selected_bookmark(-10);
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

    std::optional<QString> get_file_path_requirement_root_dir() override {
        QFileInfo file_info(QString::fromStdWString(widget->doc()->get_path()));
        QString file_path = file_info.absolutePath();
        QString file_name = file_info.fileName();
        QString parent_dir = file_path.mid(file_path.size() - file_name.size());
        return parent_dir;
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

class ToggleSelectHighlightCommand : public Command {
public:
    static inline const std::string cname = "toggle_select_highlight";
    static inline const std::string hname = "Toggle select highlight mode";
    ToggleSelectHighlightCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->is_select_highlight_mode = !widget->is_select_highlight_mode;
    }

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

class EditSelectedBookmarkWithExternalEditorCommand : public Command {
public:
    static inline const std::string cname = "edit_selected_bookmark_with_external_editor";
    static inline const std::string hname = "";
    EditSelectedBookmarkWithExternalEditorCommand (MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->handle_edit_selected_bookmark_with_external_editor();
    }

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
    DrawingMode original_drawing_mode = DrawingMode::NotDrawing;


    void pre_perform() {
        original_drawing_mode = widget->freehand_drawing_mode;
        widget->freehand_drawing_mode = DrawingMode::NotDrawing;
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
        auto rect = rect_.value();
        if (widget->main_document_view->scratchpad) {
            widget->scratchpad->delete_intersecting_objects(rect);
            widget->set_rect_select_mode(false);
            widget->clear_selected_rect();
            widget->invalidate_render();
        }
        else {
            for (int i = 0; i < widget->doc()->num_pages(); i++){
                AbsoluteRect page_rect = widget->doc()->get_page_absolute_rect(i);
                if (page_rect.intersects(rect)){
                    AbsoluteRect intersection = rect.intersect_rect(page_rect);
                    // DocumentRect intersection_doc_rect = intersection.to_document(doc());
                    // DocumentRect page_rect = rect.to_document(doc());
                    widget->doc()->delete_page_intersecting_drawings(i, intersection, widget->main_document_view->visible_drawing_mask);
                }
            }
            widget->set_rect_select_mode(false);
            widget->clear_selected_rect();
            widget->invalidate_render();
        }
        widget->freehand_drawing_mode = original_drawing_mode;
    }
};

class SelectFreehandDrawingsCommand : public Command {
public:
    static inline const std::string cname = "select_freehand_drawings";
    static inline const std::string hname = "Select freehand drawings";
    SelectFreehandDrawingsCommand(MainWidget* w) : Command(cname, w) {};

    std::optional<AbsoluteRect> rect_;
    DrawingMode original_drawing_mode = DrawingMode::NotDrawing;


    void pre_perform() {
        original_drawing_mode = widget->freehand_drawing_mode;
        widget->freehand_drawing_mode = DrawingMode::NotDrawing;
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

void register_annotation_commands(CommandManager* manager) {
    register_command<SetMark>(manager);
    register_command<GotoMark>(manager);
    register_command<ToggleDrawingMask>(manager);
    register_command<SetFreehandThickness>(manager);
    register_command<AddBookmarkCommand>(manager);
    register_command<AddBookmarkMarkedCommand>(manager);
    register_command<AddBookmarkFreetextCommand>(manager);
    register_command<AddFreetextBookmarkAutoCommand>(manager);
    register_command<AddHighlightCommand>(manager);
    register_command<CreateVisiblePortalCommand>(manager);
    register_command<PinOverviewAsPortalCommand>(manager);
    register_command<CopyDrawingsFromScratchpadCommand>(manager);
    register_command<GotoBookmarkCommand>(manager);
    register_command<GotoManualBookmarkCommand>(manager);
    register_command<GotoBookmarkGlobalCommand>(manager);
    register_command<OpenChatBookmarkCommand>(manager);
    register_command<GotoManualBookmarkGlobalCommand>(manager);
    register_command<AcceptNewBookmarkMessageCommand>(manager);
    register_command<GotoHighlightCommand>(manager);
    register_command<GotoHighlightGlobalCommand>(manager);
    register_command<GotoPortalListCommand>(manager);
    register_command<IncreaseFreetextBookmarkFontSizeCommand>(manager);
    register_command<DecreaseFreetextBookmarkFontSizeCommand>(manager);
    register_command<PortalCommand>(manager);
    register_command<SaveScratchpadCommand>(manager);
    register_command<LoadScratchpadCommand>(manager);
    register_command<ClearScratchpadCommand>(manager);
    register_command<PortalToDefinitionCommand>(manager);
    register_command<ResizePendingBookmark>(manager);
    register_command<MoveSelectedBookmarkCommand>(manager);
    register_command<EditSelectedBookmarkCommand>(manager);
    register_command<OpenSelectedBookmarkInWidget>(manager);
    register_command<EditSelectedHighlightCommand>(manager);
    register_command<DeletePortalCommand>(manager);
    register_command<DeleteBookmarkCommand>(manager);
    register_command<SelectVisibleItem>(manager);
    register_command<DeteteVisibleItem>(manager);
    register_command<DeleteVisibleBookmarkCommand>(manager);
    register_command<GotoBookmarkLinks>(manager);
    register_command<EditVisibleBookmarkCommand>(manager);
    register_command<DeleteHighlightCommand>(manager);
    register_command<ChangeHighlightTypeCommand>(manager);
    register_command<AddAnnotationToHighlightCommand>(manager);
    register_command<GotoPortalCommand>(manager);
    register_command<EditPortalCommand>(manager);
    register_command<ToggleFreehandDrawingMode>(manager);
    register_command<TogglePenDrawingMode>(manager);
    register_command<ToggleScratchpadMode>(manager);
    register_command<TogglePDFAnnotationsCommand>(manager);
    register_command<PortalToLinkCommand>(manager);
    register_command<GotoNextHighlightCommand>(manager);
    register_command<GotoPrevHighlightCommand>(manager);
    register_command<GotoNextHighlightOfTypeCommand>(manager);
    register_command<GotoPrevHighlightOfTypeCommand>(manager);
    register_command<SetSelectHighlightTypeCommand>(manager);
    register_command<SetFreehandType>(manager);
    register_command<SetFreehandAlphaCommand>(manager);
    register_command<AddHighlightWithCurrentTypeCommand>(manager);
    register_command<UndoDrawingCommand>(manager);
    register_command<TurnOnAllDrawings>(manager);
    register_command<TurnOffAllDrawings>(manager);
    register_command<PortalToOverviewCommand>(manager);
    register_command<OverviewToPortalCommand>(manager);
    register_command<UndoDeleteCommand>(manager);
    register_command<ScrollSelectedBookmarkDown>(manager);
    register_command<ScrollSelectedBookmarkUp>(manager);
    register_command<ScrollSelectedBookmarkPageUp>(manager);
    register_command<ScrollSelectedBookmarkPageDown>(manager);
    register_command<ScrollSelectedBookmarkToStart>(manager);
    register_command<ScrollSelectedBookmarkToEnd>(manager);
    register_command<DeleteHighlightUnderCursorCommand>(manager);
    register_command<WriteAnnotationsFileCommand>(manager);
    register_command<LoadAnnotationsFileCommand>(manager);
    register_command<ImportAnnotationsCommand>(manager);
    register_command<EmbedAnnotationsCommand>(manager);
    register_command<EmbedSelectedAnnotation>(manager);
    register_command<DeleteAllPDFAnnotations>(manager);
    register_command<DeleteIntersectingPDFAnnotations>(manager);
    register_command<ToggleSelectHighlightCommand>(manager);
    register_command<ClearCurrentPageDrawingsCommand>(manager);
    register_command<ClearCurrentDocumentDrawingsCommand>(manager);
    register_command<DeleteFreehandDrawingsCommand>(manager);
    register_command<SelectFreehandDrawingsCommand>(manager);
    register_command<EditSelectedBookmarkWithExternalEditorCommand>(manager);
}
