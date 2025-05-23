#include <qlineedit.h>

#include "commands/navigation_commands.h"
#include "main_widget.h"
#include "document_view.h"
#include "ui.h"
#include "path.h"

extern std::vector<MainWidget*> windows;

extern bool TOUCH_MODE;
extern bool INCREMENTAL_SEARCH;
extern bool ALIGN_LINK_DEST_TO_TOP;
extern bool TOC_JUMP_ALIGN_TOP;
extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;
extern float SMOOTH_MOVE_MAX_VELOCITY;
extern bool FUZZY_SEARCHING;
extern bool GG_USES_LABELS;
extern std::wstring SEARCH_URLS[26];

class GotoLoadedDocumentCommand : public GenericPathCommand {
public:
    static inline const std::string cname = "goto_tab";
    static inline const std::string hname = "Open tab";
    GotoLoadedDocumentCommand(MainWidget* w) : GenericPathCommand(cname, w) {}

    void handle_generic_requirement() {
        widget->handle_goto_loaded_document();
    }

    void perform() {
        if (selected_path.has_value()){
            widget->handle_goto_tab(selected_path.value());
        }
    }

    bool requires_document() {
        return false;
    }

    std::string get_name() {
        return cname;
    }

};

class GotoNextTabCommand : public Command {
 public:
     static inline const std::string cname = "goto_next_tab";
     static inline const std::string hname = "Go to the next tab.";
     GotoNextTabCommand(MainWidget* w) : Command(cname, w) {}

     void perform() {
         widget->goto_ith_next_tab(1);
     }

     std::string get_name() {
         return cname;
     }

 };

 class GotoPrevTabCommand : public Command {
 public:
     static inline const std::string cname = "goto_prev_tab";
     static inline const std::string hname = "Go to the previous tab.";
     GotoPrevTabCommand(MainWidget* w) : Command(cname, w) {}

     void perform() {
         widget->goto_ith_next_tab(-1);
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

    virtual void perform() override {
        widget->push_state();
        if (dv()->selected_character_rects.size() > 0) {
            widget->handle_find_references_to_selected_text();
        }
        else {
            if (text.has_value()) {
                dv()->find_references_to_link(text.value());
            }
        }
        widget->dv()->set_highlight_words({});
        widget->reset_highlight_links();
    }
};


class OverviewLinkCommand : public OpenLinkCommand {
public:
    static inline const std::string cname = "overview_link";
    static inline const std::string hname = "Overview to PDF links using keyboard";
    OverviewLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

    void perform_with_link(PdfLink selected_link) override{
        PdfLink pdf_link;
        pdf_link.rects = selected_link.rects;
        pdf_link.uri = selected_link.uri;
        pdf_link.source_page = selected_link.source_page;
        dv()->set_overview_link(pdf_link);
        widget->reset_highlight_links();
    }

    std::string get_name() {
        return cname;
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

class GotoSelectedTextCommand : public Command {
public:
    static inline const std::string cname = "goto_selected_text";
    static inline const std::string hname = "Go to the location of current selected text";
    GotoSelectedTextCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->long_jump_to_destination(widget->dv()->selection_begin.y);
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
        std::vector<std::wstring> all_tags = widget->get_all_fulltext_tags();
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

class EnterVisualMarkModeCommand : public Command {
public:
    static inline const std::string cname = "enter_visual_mark_mode";
    static inline const std::string hname = "Enter ruler mode using keyboard";
    EnterVisualMarkModeCommand(MainWidget* w) : Command(cname, w) {};

    void perform() {
        widget->ruler_under_pos({ widget->width() / 2, widget->height() / 2 });
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

void register_navigation_commands(CommandManager* manager) {
    register_command<GotoLoadedDocumentCommand>(manager);
    register_command<GotoNextTabCommand>(manager);
    register_command<GotoPrevTabCommand>(manager);
    register_command<NextItemCommand>(manager);
    register_command<PrevItemCommand>(manager);
    register_command<SearchCommand>(manager);
    register_command<ChapterSearchCommand>(manager);
    register_command<RegexSearchCommand>(manager);
    register_command<FuzzySearchCommand>(manager);
    register_command<SetViewStateCommand>(manager);
    register_command<GotoPageWithLabel>(manager);
    register_command<GotoTableOfContentsCommand>(manager);
    register_command<NextStateCommand>(manager);
    register_command<PrevStateCommand>(manager);
    register_command<OpenDocumentCommand>(manager);
    register_command<MoveUpSmoothCommand>(manager);
    register_command<MoveDownSmoothCommand>(manager);
    register_command<ScreenUpSmoothCommand>(manager);
    register_command<ScreenDownSmoothCommand>(manager);
    register_command<ToggleTwoPageModeCommand>(manager);
    register_command<FitEpubToWindowCommand>(manager);
    register_command<MoveDownCommand>(manager);
    register_command<MoveUpCommand>(manager);
    register_command<MoveLeftInOverviewCommand>(manager);
    register_command<MoveRightInOverviewCommand>(manager);
    register_command<MoveLeftCommand>(manager);
    register_command<MoveRightCommand>(manager);
    register_command<ZoomInCommand>(manager);
    register_command<ZoomOutCommand>(manager);
    register_command<ZoomInOverviewCommand>(manager);
    register_command<ZoomOutOverviewCommand>(manager);
    register_command<FitToPageWidthCommand>(manager);
    register_command<FitToPageWidthSmartCommand>(manager);
    register_command<FitToOverviewWidth>(manager);
    register_command<FitToPageSmartCommand>(manager);
    register_command<FitToPageHeightCommand>(manager);
    register_command<FitToPageHeightSmartCommand>(manager);
    register_command<NextPageCommand>(manager);
    register_command<PreviousPageCommand>(manager);
    register_command<GotoDefinitionCommand>(manager);
    register_command<OverviewDefinitionCommand>(manager);
    register_command<GotoRulerPortalCommand>(manager);
    register_command<OverviewRulerPortalCommand>(manager);
    register_command<MoveRulerToNextBlockCommand>(manager);
    register_command<MoveRulerToPrevBlockCommand>(manager);
    register_command<MoveVisualMarkDownCommand>(manager, "move_ruler_down");
    register_command<MoveVisualMarkUpCommand>(manager, "move_ruler_up");
    register_command<MoveVisualMarkNextCommand>(manager, "move_ruler_next");
    register_command<MoveVisualMarkPrevCommand>(manager, "move_ruler_prev");
    register_command<GotoPageWithPageNumberCommand>(manager);
    register_command<OpenPrevDocCommand>(manager);
    register_command<OpenAllDocsCommand>(manager);
    register_command<OpenDocumentEmbeddedCommand>(manager);
    register_command<OpenDocumentEmbeddedFromCurrentPathCommand>(manager);
    register_command<GotoBeginningCommand>(manager, "goto_beginning");
    register_command<GotoEndCommand>(manager);
    register_command<ToggleFullscreenCommand>(manager);
    register_command<MaximizeCommand>(manager);
    register_command<ToggleOneWindowCommand>(manager);
    register_command<ToggleHighlightCommand>(manager);
    register_command<ToggleSynctexCommand>(manager);
    register_command<TurnOnSynctexCommand>(manager);
    register_command<ForwardSearchCommand>(manager);
    register_command<ExternalSearchCommand>(manager);
    register_command<ScreenDownCommand>(manager);
    register_command<ScreenUpCommand>(manager);
    register_command<NextChapterCommand>(manager);
    register_command<PrevChapterCommand>(manager);
    register_command<ToggleDarkModeCommand>(manager);
    register_command<ToggleCustomColorMode>(manager);
    register_command<TogglePresentationModeCommand>(manager);
    register_command<TurnOnPresentationModeCommand>(manager);
    register_command<ToggleMouseDragMode>(manager);
    register_command<OpenLinkCommand>(manager);
    register_command<FindReferencesCommand>(manager);
    register_command<OverviewLinkCommand>(manager);
    register_command<KeyboardSmartjumpCommand>(manager);
    register_command<FitToPageWidthRatioCommand>(manager);
    register_command<SmartJumpUnderCursorCommand>(manager);
    register_command<OverviewUnderCursorCommand>(manager);
    register_command<CloseOverviewCommand>(manager);
    register_command<CloseVisualMarkCommand>(manager, "exit_ruler_mode");
    register_command<ZoomInCursorCommand>(manager);
    register_command<ZoomOutCursorCommand>(manager);
    register_command<GotoLeftCommand>(manager);
    register_command<GotoLeftSmartCommand>(manager);
    register_command<GotoRightCommand>(manager);
    register_command<GotoRightSmartCommand>(manager);
    register_command<RotateClockwiseCommand>(manager);
    register_command<RotateCounterClockwiseCommand>(manager);
    register_command<GotoTopOfPageCommand>(manager);
    register_command<GotoBottomOfPageCommand>(manager);
    register_command<NextPreviewCommand>(manager);
    register_command<PreviousPreviewCommand>(manager);
    register_command<GotoOverviewCommand>(manager);
    register_command<GotoSelectedTextCommand>(manager);
    register_command<FocusTextCommand>(manager);
    register_command<GotoWindowCommand>(manager);
    register_command<ToggleSmoothScrollModeCommand>(manager);
    register_command<ToggleScrollbarCommand>(manager);
    register_command<FulltextSearchCommand>(manager);
    register_command<ShowCurrentDocumentFulltextTags>(manager);
    register_command<FulltextSearchCommandWithTag>(manager);
    register_command<FulltextSearchCurrentDocumentCommand>(manager);
    register_command<DeleteDocumentFromFulltextSearchIndex>(manager);
    register_command<CreateFulltextIndexForCurrentDocumentCommand>(manager);
    register_command<CreateFulltextIndexForCurrentDocumentCommandWithTag>(manager);
    register_command<SelectCurrentSearchMatchCommand>(manager);
    register_command<SelectRulerTextCommand>(manager);
    register_command<OverviewNextItemCommand>(manager);
    register_command<OverviewPrevItemCommand>(manager);
    register_command<EnterVisualMarkModeCommand>(manager);
    register_command<ToggleVisualScrollCommand>(manager, "toggle_ruler_scroll_mode");
    register_command<ToggleHorizontalLockCommand>(manager);
    register_command<OpenLastDocumentCommand>(manager);
    register_command<GotoRandomPageCommand>(manager);
    register_command<ToggleStatusbarCommand>(manager);
}
