#include "status_string.h"
#include "document.h"
#include "document_view.h"
#include "main_widget.h"

extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_A_STR;
extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_B_STR;
extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_C_STR;
extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_D_STR;
extern float TTS_RATE;

StatusLabelLineEdit::StatusLabelLineEdit(QWidget* parent ) : QLineEdit(parent) {
    setCursor(Qt::ArrowCursor);
    QObject::connect(this, &QLineEdit::selectionChanged, [this]() {
        this->setSelection(0, 0);
        });
}

void StatusLabelLineEdit::mousePressEvent(QMouseEvent* mevent) {
    if (on_click) {
        on_click.value()();
    }
}

std::function<std::pair<QString, std::vector<int>>()> compile_status_string(QString status_string, MainWidget* widget) {

    auto custom_message_a_fn = [widget]() {return QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_A_STR); };
    auto custom_message_b_fn = [widget]() {return QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_B_STR); };
    auto custom_message_c_fn = [widget]() {return QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_C_STR); };
    auto custom_message_d_fn = [widget]() {return QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_D_STR); };

    auto get_current_page_fn = [widget]() {return QString::number(widget->get_current_page_number() + 1);};
    auto get_current_page_label_fn = [widget] {return QString::fromStdWString(widget->get_current_page_label()); };
    auto get_num_pages_fn = [widget] {
        if (widget->doc()) {
            return QString::number(widget->doc()->num_pages());
        }
        return QString("");
        };
    auto get_chapter_name_fn = [widget] {
        if (widget->doc()) {
            return " [ " + QString::fromStdWString(widget->main_document_view->get_current_chapter_name()) + " ] ";
        }
        return QString("");
        };

    auto get_document_name_fn = [widget] {
        auto file_name = Path(widget->doc()->get_path()).filename();
        if (file_name) {
            return QString::fromStdWString(file_name.value());
        }
        return QString("");
        };
    auto get_search_fn = [widget]() {

        int num_search_results = widget->main_document_view->get_num_search_results();
        float progress = -1;
        if (widget->main_document_view->get_is_searching(&progress)) {

            int result_index = widget->main_document_view->get_num_search_results() > 0 ? widget->main_document_view->get_current_search_result_index() + 1 : 0;
            auto res = " | showing result " + QString::number(result_index) + " / " + QString::number(num_search_results);
            if (progress > 0) {
                res = res + " (" + QString::number((int)(progress * 100)) + "%" + ")";
            }
            return res;
        }
        return QString("");
        };
    auto get_link_status_fn = [widget]() {
        if (widget->is_pending_link_source_filled()) {
            return QString(" | linking ...");
        }
        else if (widget->portal_to_edit) {
            return QString(" | editing link ...");
        }
        else {
            return QString("");
        }
        };
    auto get_waiting_for_symbol_fn = [widget]() {

        if (widget->is_waiting_for_symbol()) {
            std::wstring wcommand_name = utf8_decode(widget->pending_command_instance->next_requirement(widget).value().name);
            QString hint_name = QString::fromStdString(widget->pending_command_instance->get_name());

            if (wcommand_name.size() > 0) {
                hint_name += "(" + QString::fromStdWString(wcommand_name) + ")";
            }

            return " " + hint_name + " waiting for symbol";
        }
        return QString("");
        };
    auto indexing_fn = [widget]() {
        
        if (widget->main_document_view != nullptr && widget->main_document_view->get_document() != nullptr &&
            widget->main_document_view->get_document()->get_is_indexing()) {
            return QString(" | indexing ... ");
        }
        return QString("");
        };
    auto overview_fn = [widget]() {

        if (widget->main_document_view && widget->main_document_view->get_overview_page()) {
            if (widget->dv()->index_into_candidates >= 0 && widget->dv()->smart_view_candidates.size() > 1) {
                QString preview_source_string = "";
                if (widget->dv()->smart_view_candidates[widget->dv()->index_into_candidates].source_text.size() > 0) {
                    preview_source_string = " (" + QString::fromStdWString(widget->dv()->smart_view_candidates[widget->dv()->index_into_candidates].source_text) + ")";
                }
                return " [ preview " + QString::number(widget->dv()->index_into_candidates + 1) + " / " + QString::number(widget->dv()->smart_view_candidates.size()) + preview_source_string + " ]";
            }
        }

        return QString("");
        };
    auto synctex_fn = [widget]() {

        if (widget->synctex_mode) {
            return QString(" [ synctex ]");
        }
        return QString("");
        };

    auto drag_fn = [widget]() {
        if (widget->mouse_drag_mode) {
            return QString(" [ drag ]");
        }
        };
    auto presentation_fn = [widget]() {

        if (widget->main_document_view->is_presentation_mode()) {
            return QString(" [ presentation ]");
        }
        return QString("");
        };
    auto auto_name_fn = [widget]() {
        return QString::fromStdWString(widget->doc()->get_detected_paper_name_if_exists());
        };
    auto visual_scroll_fn = [widget]() {

        if (widget->visual_scroll_mode) {
            return QString(" [ visual scroll ]");
        }
        return QString("");
        };
    auto horizontal_scroll_fn = [widget]() {
        if (widget->horizontal_scroll_locked) {
            return QString(" [ locked horizontal scroll ]");
        }
        return QString("");
        };

    auto highlight_fn = [widget]() {

        std::wstring highlight_select_char = L"";

        if (widget->is_select_highlight_mode) {
            highlight_select_char = L"s";
        }

        return " [ h" + QString::fromStdWString(highlight_select_char) + ":" + widget->select_highlight_type + " ]";
        };

    auto drawing_fn = [widget]() {

        QString drawing_mode_string = "";
        if (widget->freehand_drawing_mode == DrawingMode::Drawing) {
            drawing_mode_string = QString(" [ freehand:") + widget->current_freehand_type + " ]";
        }
        if (widget->freehand_drawing_mode == DrawingMode::PenDrawing) {
            drawing_mode_string = QString(" [ pen:") + widget->current_freehand_type + " ]";
        }

        return drawing_mode_string;
        };
    auto mode_fn = [widget]() {
        return QString::fromStdString(widget->get_current_mode_string());
        };
    auto closest_bookmark_fn = [widget]() {

        std::optional<BookMark> closest_bookmark = widget->main_document_view->find_closest_bookmark();
        if (closest_bookmark) {
            return " [ " + QString::fromStdWString(closest_bookmark.value().description) + " ]";
        }
        return QString("");
        };
    auto closest_portal_fn = [widget]() {

        std::optional<Portal> close_portal = widget->get_target_portal(true);
        if (close_portal) {
            return QString(" [ PORTAL ]");
        }
        return QString("");
        };
    auto rect_select_fn = [widget]() {

        if (widget->rect_select_mode) {
            return QString(" [ select box ]");
        }
        return QString("");
        };
    
    auto point_select_fn = [widget]() {
        if (widget->point_select_mode) {
            return QString(" [ select point ]");
        }
        };
    auto custom_message_fn = [widget]() {

        if (widget->custom_status_message.size() > 0) {
            return " [ " + QString::fromStdWString(widget->custom_status_message) + " ]";
        }
        return QString("");
        };
    auto current_requirement_fn = [widget]() {

        if (widget->pending_command_instance){
            if (widget->pending_command_instance->next_requirement(widget)->type == RequirementType::Point) {
                return " [ " + QString::fromStdString(widget->pending_command_instance->next_requirement(widget)->name) + " ] ";
            }
        }
        return QString("");
        };
    auto download_fn = [widget]() {

        bool is_downloading = false;
        if (widget->is_network_manager_running(&is_downloading)) {
            if (is_downloading) {
                return QString(" [ downloading ]");
            }
        }
        return QString("");
        };
    auto selected_highlight_fn = [widget]() {
        int selected_highlight_index = widget->get_selected_highlight_index();
        if (selected_highlight_index != -1) {
            Highlight hl = widget->main_document_view->get_highlight_with_index(selected_highlight_index);
            return " [ " + QString::fromStdWString(hl.text_annot) + " ]";
        }
        return QString("");
        };
    auto download_button_fn = [widget]() {
        if (widget->dv() && widget->dv()->overview_page) {
            if (((widget->dv()->overview_page->overview_type == "reference") || (widget->dv()->overview_page->overview_type == "reflink"))
                && (widget->dv()->overview_page->highlight_rects.size() > 0)) {
                return " [ download ]";
            }
        }
        return "";
        };
    auto network_status_fn = [widget]() {
        return widget->get_network_status_string();
        };

    auto tts_status_fn = [widget]() {
        if (widget->is_reading) {
            return " [ stop reading ]";
        }
        else if (widget->high_quality_play_state) {
            return " [ stop reading ]";
        }
        };
    auto tts_rate_fn = [widget]() {
        if (widget->is_reading) {
            return " [ rate " + QString::number(TTS_RATE) + " ]";
        }
        else if (widget->high_quality_play_state) {
            return " [ rate " + QString::number(TTS_RATE) + " ]";
        }
        };


    std::unordered_map<QString, std::function<QString()>> name_to_generator = {
        {"current_page", get_current_page_fn},
        {"current_page_label", get_current_page_label_fn},
        {"num_pages", get_num_pages_fn},
        {"chapter_name", get_chapter_name_fn},
        {"document_name", get_document_name_fn},
        {"search_results", get_search_fn},
        {"link_status", get_link_status_fn},
        {"waiting_for_symbol", get_waiting_for_symbol_fn},
        {"indexing", indexing_fn},
        {"preview_index", overview_fn},
        {"synctex", synctex_fn},
        {"drag", drag_fn},
        {"presentation", presentation_fn},
        {"auto_name", auto_name_fn},
        {"visual_scroll", visual_scroll_fn},
        {"locked_scroll", horizontal_scroll_fn},
        {"highlight", highlight_fn},
        {"freehand_drawing", drawing_fn},
        {"mode_string", mode_fn},
        {"closest_bookmark", closest_bookmark_fn},
        {"closest_portal", closest_portal_fn},
        {"rect_select", rect_select_fn},
        {"point_select", point_select_fn},
        {"custom_message", custom_message_fn},
        {"current_requirement_desc", current_requirement_fn},
        {"download", download_fn},
        {"download_button", download_button_fn},
        {"network_status", network_status_fn},
        {"tts_status", tts_status_fn},
        {"tts_rate", tts_rate_fn},
        {"custom_message_a", custom_message_a_fn},
        {"custom_message_b", custom_message_b_fn},
        {"custom_message_c", custom_message_c_fn},
        {"custom_message_d", custom_message_d_fn},
    };

    std::unordered_map<QString, int> name_to_id;
    for (int i = 0; i < STATUS_STRING_PARTS.size(); i++) {
        name_to_id[STATUS_STRING_PARTS[i]] = i;
    }

    //std::unordered_map<QString, StatusStringPart> name_to_id = {
    //    {"current_page", StatusStringPart::CURRENT_PAGE},
    //    {"current_page_label", StatusStringPart::CURRENT_PAGE_LABEL},
    //    {"num_pages", StatusStringPart::NUM_PAGES},
    //    {"chapter_name", StatusStringPart::CHAPTER_NAME},
    //    {"document_name", StatusStringPart::DOCUMENT_NAME},
    //    {"search_results", StatusStringPart::SEARCH_RESULTS},
    //    {"link_status", StatusStringPart::LINK_STATUS},
    //    {"waiting_for_symbol", StatusStringPart::WAITING_FOR_SYMBOL},
    //    {"indexing", StatusStringPart::INDEXING},
    //    {"preview_index", StatusStringPart::PREVIEW_INDEX},
    //    {"synctex", StatusStringPart::SYNCTEX},
    //    {"drag", StatusStringPart::DRAG},
    //    {"presentation", StatusStringPart::PRESENTATION},
    //    {"auto_name", StatusStringPart::AUTO_NAME},
    //    {"visual_scroll", StatusStringPart::VISUAL_SCROLL},
    //    {"locked_scroll", StatusStringPart::LOCKED_SCROLL},
    //    {"highlight", StatusStringPart::HIGHLIGHT},
    //    {"freehand_drawing", StatusStringPart::FREEHAND_DRAWING},
    //    {"mode_string", StatusStringPart::MODE_STRING},
    //    {"closest_bookmark", StatusStringPart::CLOSEST_BOOKMARK},
    //    {"closest_portal", StatusStringPart::CLOSEST_PORTAL},
    //    {"rect_select", StatusStringPart::RECT_SELECT},
    //    {"point_select", StatusStringPart::POINT_SELECT},
    //    {"custom_message", StatusStringPart::CUSTOM_MESSAGE},
    //    {"current_requirement_desc", StatusStringPart::CURRENT_REQUIREMENT_DESC},
    //    {"download", StatusStringPart::DOWNLOAD},
    //};

    QRegularExpression expr("%\\{[a-z_]+\\}");
    QRegularExpressionMatchIterator matches = expr.globalMatch(status_string);
    int prev_match_end_index = 0;

    std::vector<std::variant<QString, std::pair<int, std::function<QString()>>>> parts;

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString intermatch = status_string.mid(prev_match_end_index, match.capturedStart() - prev_match_end_index);

        parts.push_back(intermatch);

        QString captured = match.captured();
        QString captured_name = captured.mid(2, captured.size() - 3);

        if (name_to_generator.find(captured_name) != name_to_generator.end()) {
            parts.push_back(std::make_pair(name_to_id[captured_name], name_to_generator[captured_name]));
        }

        prev_match_end_index = match.capturedEnd();
    }
    if (prev_match_end_index < status_string.size() - 1) {
        parts.push_back(status_string.right(status_string.size() - 1 - prev_match_end_index));
    }

    auto generator = [name_to_generator=std::move(name_to_generator), parts=std::move(parts)]() {
        QString res = "";
        std::vector<int> part_types;
        for (auto& part : parts) {
            if (std::holds_alternative<QString>(part)) {
                QString p = std::get<QString>(part);
                res += p;
                std::fill_n(std::back_inserter(part_types), p.size(), -1);
            }
            else {
                auto& [part_type, part_fn] = std::get <std::pair<int, std::function<QString()>>>(part);
                QString p = part_fn();
                res += part_fn();
                std::fill_n(std::back_inserter(part_types), p.size(), static_cast<int>(part_type));
            }
        }
        return std::make_pair(res, part_types);
        };

    return std::move(generator);
}
