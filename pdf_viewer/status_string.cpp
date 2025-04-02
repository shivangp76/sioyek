#include "status_string.h"
#include "document.h"
#include "document_view.h"
#include "main_widget.h"
#include "commands/base_commands.h"

extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_A_STR;
extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_B_STR;
extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_C_STR;
extern std::wstring STATUS_STRING_CUSTOM_MESSAGE_D_STR;
extern float TTS_RATE;
extern int MAX_CUSTOM_STATUS_MESSAGE_SIZE;

std::unordered_map<QString, int> name_to_id;

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
void StatusLabelLineEdit::contextMenuEvent(QContextMenuEvent* event) {
    return;
}

std::pair<QString, std::vector<int>> P(QString s){
    std::vector<int> types;
    return std::make_pair(s, types);
}

void fill_with_value(std::vector<int>& v, int value, int N){
    std::fill_n(std::back_inserter(v), N, value);
}

std::function<std::pair<QString, std::vector<int>>()> compile_status_string(QString status_string, MainWidget* main_widget) {

    auto custom_message_a_fn = [widget=main_widget]() {return P(QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_A_STR)); };
    auto custom_message_b_fn = [widget=main_widget]() {return P(QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_B_STR)); };
    auto custom_message_c_fn = [widget=main_widget]() {return P(QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_C_STR)); };
    auto custom_message_d_fn = [widget=main_widget]() {return P(QString::fromStdWString(STATUS_STRING_CUSTOM_MESSAGE_D_STR)); };

    auto dummy_fn = []() {return P(QString(""));};
    auto get_current_page_fn = [widget=main_widget]() {return P(QString::number(widget->get_current_page_number() + 1));};
    auto get_current_page_label_fn = [widget=main_widget] {return P(QString::fromStdWString(widget->get_current_page_label())); };

    auto get_num_pages_fn = [widget=main_widget] {
        if (widget->doc()) {
            return P(QString::number(widget->doc()->num_pages()));
        }
        return P(QString(""));
        };

    auto get_chapter_name_fn = [widget=main_widget] {
        if (widget->doc()) {
            return P(" [ " + QString::fromStdWString(widget->main_document_view->get_current_chapter_name()) + " ] ");
        }
        return P(QString(""));
        };

    auto get_document_name_fn = [widget=main_widget] {
        auto file_name = Path(widget->doc()->get_path()).filename();
        if (file_name) {
            return P(QString::fromStdWString(file_name.value()));
        }
        return P(QString(""));
        };

    auto get_search_fn = [widget=main_widget]() {

        int num_search_results = widget->main_document_view->get_num_search_results();
        float progress = -1;
        if (widget->main_document_view->get_is_searching(&progress)) {

            int result_index = widget->main_document_view->get_num_search_results() > 0 ? widget->main_document_view->get_current_search_result_index() + 1 : 0;
            auto res = " | ⌕ " + QString::number(result_index) + " / " + QString::number(num_search_results);
            if (progress > 0) {
                res = res + " (" + QString::number((int)(progress * 100)) + "%" + ")";
            }
            std::optional<SearchResult> current_search_result = widget->main_document_view->get_current_search_result();
            if (current_search_result && current_search_result->message.size() > 0) {
                res += " " + current_search_result->message;
            }
            return P(res);
        }
        return P(QString(""));
        };

    auto get_link_status_fn = [widget=main_widget]() {
        if (widget->main_document_view->is_pending_link_source_filled()) {
            return P(QString(" | linking ..."));
        }
        else if (widget->portal_to_edit) {
            return P(QString(" | editing link ..."));
        }
        else {
            return P(QString(""));
        }
        };

    auto get_waiting_for_symbol_fn = [widget=main_widget]() {

        if (widget->is_waiting_for_symbol()) {
            std::wstring wcommand_name = utf8_decode(widget->pending_command_instance->next_requirement(widget).value().name);
            QString hint_name = QString::fromStdString(widget->pending_command_instance->get_symbol_hint_name());

            if (wcommand_name.size() > 0) {
                hint_name += "(" + QString::fromStdWString(wcommand_name) + ")";
            }

            return P(" " + hint_name + " waiting for symbol");
        }
        return P(QString(""));
        };

    auto indexing_fn = [widget=main_widget]() {

        if (widget->main_document_view != nullptr && widget->main_document_view->get_document() != nullptr &&
            widget->main_document_view->get_document()->get_is_indexing()) {
            float progress = widget->doc()->get_indexing_progress();
            int progress_percent = static_cast<int>(progress * 100);
            QString progress_string = QString::number(progress_percent);
            return P(QString(" | indexing ... " + progress_string + "%"));
        }
        return P(QString(""));
        };

    auto overview_fn = [widget=main_widget]() {

        if (widget->main_document_view && widget->main_document_view->get_overview_page()) {
            if (widget->dv()->index_into_candidates >= 0 && widget->dv()->smart_view_candidates.size() > 1) {
                QString preview_source_string = "";
                if (widget->dv()->smart_view_candidates[widget->dv()->index_into_candidates].source_text.size() > 0) {
                    preview_source_string = " (" + QString::fromStdWString(widget->dv()->smart_view_candidates[widget->dv()->index_into_candidates].source_text) + ")";
                }
                return P(" [ preview " + QString::number(widget->dv()->index_into_candidates + 1) + " / " + QString::number(widget->dv()->smart_view_candidates.size()) + preview_source_string + " ]");
            }
        }

        return P(QString(""));
        };

    auto synctex_fn = [widget=main_widget]() {

        if (widget->synctex_mode) {
            return P(QString(" [ synctex ]"));
        }
        return P(QString(""));
        };


    auto drag_fn = [widget=main_widget]() {
        if (widget->mouse_drag_mode) {
            return P(QString(" [ drag ]"));
        }
        else{
            return P(QString(""));
        }
        };

    auto presentation_fn = [widget=main_widget]() {

        if (widget->main_document_view->is_presentation_mode()) {
            return P(QString(" [ presentation ]"));
        }
        return P(QString(""));
        };

    auto auto_name_fn = [widget=main_widget]() {
        if (widget->doc()){
            return P(QString::fromStdWString(widget->doc()->get_detected_paper_name_if_exists()));
        }
        return P(QString(""));
        };

    auto visual_scroll_fn = [widget=main_widget]() {

        if (widget->visual_scroll_mode) {
            return P(QString(" [ visual scroll ]"));
        }
        return P(QString(""));
        };

    auto horizontal_scroll_fn = [widget=main_widget]() {
        if (widget->horizontal_scroll_locked) {
            return P(QString(" [ locked horizontal scroll ]"));
        }
        return P(QString(""));
        };


    auto highlight_fn = [widget=main_widget]() {

        std::wstring highlight_select_char = L"";

        if (widget->is_select_highlight_mode) {
            highlight_select_char = L"s";
        }

        return P(" [ h" + QString::fromStdWString(highlight_select_char) + ":" + widget->select_highlight_type + " ]");
        };


    auto drawing_fn = [&, widget=main_widget]() {


        QString drawing_mode_string = "";
        if (widget->main_document_view){
            if (widget->freehand_drawing_mode == DrawingMode::Drawing)
            {
                float thickness = widget->main_document_view->freehand_thickness;
                std::vector<int> type_ids;
                QString result = "[ ";
                fill_with_value(type_ids, -1, result.size());

                QString type_part = QString("freehand:") + widget->main_document_view->current_freehand_type;
                result += type_part;
                fill_with_value(type_ids, name_to_id["drawing_type"], type_part.size());

                QString size_part = QString(" size:") + QString::number(thickness);
                result += size_part;
                fill_with_value(type_ids, name_to_id["drawing_size"], size_part.size());

                QString close_part = " ]";
                result += close_part;
                fill_with_value(type_ids, -1, close_part.size());

                return std::make_pair(result, type_ids);
            }
            if (widget->freehand_drawing_mode == DrawingMode::PenDrawing)
            {
                return  P(QString(" [ pen:") + widget->main_document_view->current_freehand_type + " ]");
            }
        }

        return P(drawing_mode_string);
        };

    auto mode_fn = [widget=main_widget]() {
        return P(QString::fromStdString(widget->get_current_mode_string()));
        };

    auto closest_bookmark_fn = [widget=main_widget]() {

        std::optional<BookMark> closest_bookmark = widget->main_document_view->find_closest_bookmark();
        if (closest_bookmark) {
            return P(" [ " + QString::fromStdWString(closest_bookmark.value().description) + " ]");
        }
        return P(QString(""));
        };

    auto closest_portal_fn = [widget=main_widget]() {

        std::optional<Portal> close_portal = widget->main_document_view->get_target_portal(true);
        if (close_portal) {
            return P(QString(" [ PORTAL ]"));
        }
        return P(QString(""));
        };

    auto rect_select_fn = [widget=main_widget]() {

        if (widget->rect_select_mode) {
            return P(QString(" [ select box ]"));
        }
        return P(QString(""));
        };

    
    auto point_select_fn = [widget=main_widget]() {
        if (widget->point_select_mode) {
            return P(QString(" [ select point ]"));
        }
        };

    auto custom_message_fn = [widget=main_widget]() {

        if (widget->status_messages.size() > 0) {
            QString message_string = "";
            for (int i = 0; i < widget->status_messages.size(); i++) {
                message_string += widget->status_messages[i].message;
                if (i < widget->status_messages.size() - 1) {
                    message_string += " | ";
                }
            }
            if (widget->status_messages.size() > 1 && message_string.size() > MAX_CUSTOM_STATUS_MESSAGE_SIZE) {
                int index = widget->current_status_message_index < widget->status_messages.size() ? widget->current_status_message_index : widget->status_messages.size() - 1;
                auto current_message = widget->status_messages[index].message;
                current_message += " (" + QString::number(index + 1) + "/" + QString::number(widget->status_messages.size()) + ")";
                return P(" [ " + current_message + " ]");
            }
            else {
                return P(" [ " + message_string + " ]");
            }
        }
        return P(QString(""));
        };

    auto current_requirement_fn = [widget=main_widget]() {

        if (widget->pending_command_instance){
            if (widget->pending_command_instance->next_requirement(widget).has_value()){
                if (widget->pending_command_instance->next_requirement(widget)->type == RequirementType::Point) {
                    return P(" [ " + QString::fromStdString(widget->pending_command_instance->next_requirement(widget)->name) + " ] ");
                }
            }
        }
        return P(QString(""));
        };

    auto download_fn = [widget=main_widget]() {

        bool is_downloading = false;
        std::wstring message;
        if (widget->is_network_manager_running(&is_downloading, &message)) {
            if (is_downloading) {
                if (message.size() > 0) {
                    return P(QString(" [ " + QString::fromStdWString(message) + " ]"));
                }
                else {
                    return P(QString(" [ downloading ]"));
                }

            }
        }
        return P(QString(""));
        };

    auto selected_highlight_fn = [widget=main_widget]() {
        std::string selected_highlight_uuid = widget->main_document_view->get_selected_highlight_uuid();
        if (selected_highlight_uuid.size() > 0) {
            Highlight* hl = widget->doc()->get_highlight_with_uuid(selected_highlight_uuid);
            if (hl->text_annot.size() > 0) {
                return P(" [ " + QString::fromStdWString(hl->text_annot) + " ]");
            }
            else {

                return P(QString(" [ <no annotation> ]"));
            }
        }
        return P(QString(""));
        };

    auto download_button_fn = [widget=main_widget]() {
        if (widget->dv() && widget->dv()->overview_page) {
            if (((widget->dv()->overview_page->overview_type == "reference") || (widget->dv()->overview_page->overview_type == "reflink"))
                && (widget->dv()->overview_page->highlight_rects.size() > 0)) {
                auto mappings = widget->input_handler->get_key_mappings("download_overview_paper");
                //widget->input_handler->get_com
                if (mappings.size() == 0) {
                    return P(QString(" [ download ]"));
                }
                else {
                    return P(QString(" [ download keybind : ") + QString::fromStdString(mappings[0]) + " ]");
                }
            }
        }
        return P(QString(""));
        };

    auto network_status_fn = [widget=main_widget]() {


        auto network_string = widget->get_network_status_string();
        if (network_string.size() > 0) {
            return P(" [ " + network_string +" ]");
        }
        else {
            return P(QString(""));
        }
        };

    auto tts_status_fn = [widget=main_widget]() {
        if (widget->is_reading) {
            return P(" [ stop reading ]");
        }
        else if (widget->high_quality_play_state) {
            return P(" [ stop reading ]");
        }
        else{
            return P("");
        }
        };

    auto tts_rate_fn = [widget=main_widget]() {
        if (widget->is_reading) {
            return P(" [ rate " + QString::number(TTS_RATE) + " ]");
        }
        else if (widget->high_quality_play_state) {
            return P(" [ rate " + QString::number(TTS_RATE) + " ]");
        }
        else {
            return P(QString(""));
        }
        };


    std::unordered_map<QString, std::function<std::pair<QString, std::vector<int>>()>> name_to_generator = {
        {"current_page", std::move(get_current_page_fn)},
        {"current_page_label", std::move(get_current_page_label_fn)},
        {"num_pages", std::move(get_num_pages_fn)},
        {"chapter_name", std::move(get_chapter_name_fn)},
        {"document_name", std::move(get_document_name_fn)},
        {"search_results", std::move(get_search_fn)},
        {"link_status", std::move(get_link_status_fn)},
        {"waiting_for_symbol", std::move(get_waiting_for_symbol_fn)},
        {"indexing", std::move(indexing_fn)},
        {"preview_index", std::move(overview_fn)},
        {"synctex", std::move(synctex_fn)},
        {"drag", std::move(drag_fn)},
        {"presentation", std::move(presentation_fn)},
        {"auto_name", std::move(auto_name_fn)},
        {"visual_scroll", std::move(visual_scroll_fn)},
        {"locked_scroll", std::move(horizontal_scroll_fn)},
        {"highlight", std::move(highlight_fn)},
        {"freehand_drawing", std::move(drawing_fn)},
        {"mode_string", std::move(mode_fn)},
        {"closest_bookmark", std::move(closest_bookmark_fn)},
        {"selected_highlight", std::move(selected_highlight_fn)},
        {"closest_portal", std::move(closest_portal_fn)},
        {"rect_select", std::move(rect_select_fn)},
        {"point_select", std::move(point_select_fn)},
        {"custom_message", std::move(custom_message_fn)},
        {"current_requirement_desc", std::move(current_requirement_fn)},
        {"download", std::move(download_fn)},
        {"download_button", std::move(download_button_fn)},
        {"network_status", std::move(network_status_fn)},
        {"tts_status", std::move(tts_status_fn)},
        {"tts_rate", std::move(tts_rate_fn)},
        {"custom_message_a", std::move(custom_message_a_fn)},
        {"custom_message_b", std::move(custom_message_b_fn)},
        {"custom_message_c", std::move(custom_message_c_fn)},
        {"custom_message_d", std::move(custom_message_d_fn)},

        {"drawing_type", dummy_fn},
        {"drawing_size", dummy_fn},

    };

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
    int prev_match_end_index = -1;

    std::vector<std::variant<QString, std::pair<int, std::function<std::pair<QString, std::vector<int>>()>>>> parts;

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
        parts.push_back(status_string.right(status_string.size() - prev_match_end_index));
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
                auto& [part_type, part_fn] = std::get <std::pair<int, std::function<std::pair<QString, std::vector<int>>()>>>(part);
                auto [p, part_ids] = part_fn();
                res += p;
                if (p.size() > 0){
                    if (part_ids.size() != p.size()){
                        std::fill_n(std::back_inserter(part_types), p.size(), static_cast<int>(part_type));
                    }
                    else{
                        std::copy(part_ids.begin(), part_ids.end(), std::back_inserter(part_types));
                    }
                }
            }
        }
        return std::make_pair(res, part_types);
        };

    // std::function<std::pair<QString, std::vector<int>>()> res = [](){
    //     std::vector<int> types = {-1};
    //     QString res = "a";
    //     return std::make_pair(res, types);
    // };
    // return std::move(res);
    return std::move(generator);
}
