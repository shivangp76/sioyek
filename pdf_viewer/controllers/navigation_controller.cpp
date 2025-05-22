#include <QPushButton>

#include "controllers/navigation_controller.h"
#include "main_widget.h"
#include "controllers/network_controller.h"
#include "pdf_view_opengl_widget.h"
#include "commands/base_commands.h"
#include "ui.h"
#include "checksum.h"

extern bool ALIGN_LINK_DEST_TO_TOP;
extern std::wstring MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring SEARCH_URLS[26];
extern bool SCROLLBAR;
extern bool SHOW_DOC_PATH;
extern bool TOUCH_MODE;
extern bool FANCY_UI_MENUS;
extern bool MULTILINE_MENUS;
extern std::wstring SERVER_SYMBOL;

void search_paper_name_using_external_browser(QString paper_name, bool is_shift_pressed) {
    if (paper_name.size() > 5) {
        char type;
        if (is_shift_pressed) {
            type = SHIFT_MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        else {
            type = MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        if ((type >= 'a') && (type <= 'z')) {
            search_custom_engine(paper_name.toStdWString(), SEARCH_URLS[type - 'a']);
        }
    }

}

NavigationController::NavigationController(MainWidget* parent) : BaseController(parent){
}

void NavigationController::open_document(const Path& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level, std::string downloaded_checksum) {
    if (doc()) {
        if (mw->resume_to_server_position_button->isVisible()) {
            mw->resume_to_server_position_button->hide();
        }
        mw->network_controller->sync_current_file_location_to_servers();
    }

    mw->opengl_widget->clear_all_selections();

    //save the previous document state
    if (mdv() && doc()) {
        bool should_sync_drawings = doc()->get_drawings_are_dirty();
        mdv()->persist(true);
        mw->network_controller->perform_sync_operations_when_document_is_closed(false, should_sync_drawings);
    }

    if (mdv()->get_view_width() > mw->main_window_width) {
        mw->main_window_width = mdv()->get_view_width();
    }

    mdv()->on_view_size_change(mw->main_window_width, mw->main_window_height);
    mdv()->open_document(path.get_path(), true, {}, false, downloaded_checksum);

    if (downloaded_checksum.size() > 0) {
        // if this documents is downloaded from the server, it must be synced
        doc()->set_is_synced(true);
    }

    mw->on_open_document(path.get_path());

    if (doc()) {
        mw->document_manager->add_tab(doc()->get_path());
        //doc()->set_only_for_portal(false);
    }

    bool has_document = mw->main_document_view_has_document();

    if (has_document) {
        //setWindowTitle(QString::fromStdWString(path.get_path()));
        if (path.filename().has_value()) {
            mw->setWindowTitle(QString::fromStdWString(path.filename().value()));
        }
        else {
            mw->setWindowTitle(QString::fromStdWString(path.get_path()));
        }

    }

    if ((path.get_path().size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file1: " + path.get_path());
    }

    if (offset_x) {
        mdv()->set_offset_x(offset_x.value());
    }
    if (offset_y) {
        mdv()->set_offset_y(offset_y.value());
    }

    if (zoom_level) {
        mdv()->set_zoom_level(zoom_level.value(), true);
    }

    mw->show_password_prompt_if_required();

    if (mw->main_document_view_has_document()) {
        mw->update_scrollbar();
    }

    mw->deselect_document_indices();
    mw->invalidate_render();

}

void NavigationController::open_document_at_location(const Path& path_,
    int page,
    std::optional<float> x_loc,
    std::optional<float> y_loc,
    std::optional<float> zoom_level,
    bool should_push_state)
{
    //save the previous document state
    if (mdv()) {
        mdv()->persist();
    }
    std::wstring path = path_.get_path();

    open_document(path, true, {}, true);
    bool has_document = mw->main_document_view_has_document();

    if (has_document && should_push_state) {
        //setWindowTitle(QString::fromStdWString(path));
        push_state();
    }

    if ((path.size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file2: " + path);
    }
    else {
        mdv()->on_view_size_change(mw->main_window_width, mw->main_window_height);

        AbsoluteDocumentPos absolute_pos = DocumentPos{ page, x_loc.value_or(0), y_loc.value_or(0) }.to_absolute(doc());

        if (x_loc) {
            mdv()->set_offset_x(absolute_pos.x);
        }
        mdv()->set_offset_y(absolute_pos.y);

        if (zoom_level) {
            mdv()->set_zoom_level(zoom_level.value(), true);
        }
    }


    mw->show_password_prompt_if_required();
}

void NavigationController::push_state(bool update) {

    if (!mw->main_document_view_has_document()) return; // we don't add empty document to history

    DocumentViewState dvs = mdv()->get_state();

    //if (history.size() > 0) { // this check should always be true
    //	history[history.size() - 1] = dvs;
    //}
    //// don't add the same place in history multiple times
    //// todo: we probably don't need this check anymore
    //if (history.size() > 0) {
    //	DocumentViewState last_history = history.back();
    //	if (last_history == dvs) return;
    //}

    // delete all history elements after the current history point
    history.erase(history.begin() + (1 + current_history_index), history.end());
    if (!((history.size() > 0) && (history.back() == dvs))) {
        history.push_back(dvs);
    }
    if (update) {
        current_history_index = static_cast<int>(history.size() - 1);
    }
}

void NavigationController::next_state() {
    if (current_history_index < (static_cast<int>(history.size()) - 1)) {
        update_current_history_index();
        current_history_index++;
        if (current_history_index + 1 < history.size()) {
            set_main_document_view_state(history[current_history_index + 1]);
        }

    }
}

void NavigationController::prev_state() {
    if (current_history_index >= 0) {
        update_current_history_index();

        /*
        Goto previous history
        In order to edit a link, we set the link to edit and jump to the link location, when going back, we
        update the link with the current location of document, therefore, we must check to see if a link
        is being edited and if so, we should update its destination position
        */
        if (mw->portal_to_edit) {

            //std::wstring link_document_path = checksummer->get_path(link_to_edit.value().dst.document_checksum).value();
            std::wstring link_document_path = history[current_history_index].document_path;
            Document* link_owner = mw->document_manager->get_document(link_document_path);

            OpenedBookState state = mdv()->get_state().book_state;
            mw->portal_to_edit.value().dst.book_state = state;

            if (link_owner) {
                link_owner->update_portal(mw->portal_to_edit.value());
            }

            mw->db_manager->update_portal(mw->portal_to_edit->uuid, state.offset_x, state.offset_y, state.zoom_level);
            mw->set_recently_updated_portal(mw->portal_to_edit->uuid);

            mw->portal_to_edit = {};
        }

        if (current_history_index == (history.size() - 1)) {
            if (!(history[history.size() - 1] == mdv()->get_state())) {
                push_state(false);
            }
        }
        if (history[current_history_index] == mdv()->get_state()) {
            current_history_index--;
        }
        if (current_history_index >= 0) {
            DocumentViewState new_state = history[current_history_index];
            // save the current document in the list of opened documents
            if (doc() && doc()->get_path() != new_state.document_path) {
                mw->persist();
            }
            set_main_document_view_state(new_state);
            current_history_index--;
        }
    }
}

void NavigationController::update_current_history_index() {
    if (mw->main_document_view_has_document()) {
        int index_to_update = current_history_index + 1;
        if (index_to_update < history.size()) {
            DocumentViewState current_state = mdv()->get_state();
            history[index_to_update] = current_state;
        }
    }
}

void NavigationController::set_main_document_view_state(DocumentViewState new_view_state) {

    if ((!mw->main_document_view_has_document()) || (doc()->get_path() != new_view_state.document_path)) {
        mw->open_document(new_view_state.document_path, &mw->is_ui_invalidated);

        //setwindowtitle(qstring::fromstdwstring(new_view_state.document_path));
    }

    mdv()->on_view_size_change(mw->main_window_width, mw->main_window_height);
    mdv()->set_book_state(new_view_state.book_state);
}

void NavigationController::update_renamed_document_in_history(std::wstring old_path, std::wstring new_path){

    for (int i = 0; i < history.size(); i++) {
        if (history[i].document_path == old_path) {
            history[i].document_path = new_path;
        }
    }
}

void NavigationController::smart_jump_under_pos(WindowPos pos) {
    if ((!mw->main_document_view_has_document()) || mdv()->scratchpad) {
        return;
    }


    Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
    bool is_shift_pressed = modifiers.testFlag(Qt::ShiftModifier);

    auto main_document_view = mdv();
    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(pos);

    // if overview page is open and we middle click on a paper name, search it in a search engine
    if (main_document_view->is_window_point_in_overview({ normal_x, normal_y })) {
        DocumentPos docpos = main_document_view->window_pos_to_overview_pos({ normal_x, normal_y });
        std::optional<PaperNameWithRects> paper_name = main_document_view->get_document()->get_paper_name_at_position(docpos);
        if (paper_name) {
            search_paper_name_using_external_browser(paper_name->paper_name, is_shift_pressed);
        }
        return;
    }

    auto docpos = main_document_view->window_to_document_pos(pos);

    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(docpos.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    TextUnderPointerInfo text_under_pos_info = dv()->find_location_of_text_under_pointer(docpos);
    if ((text_under_pos_info.candidates.size() > 0) && (text_under_pos_info.candidates[0].reference_type != ReferenceType::NoReference)){
        DocumentPos candid_docpos = text_under_pos_info.candidates[0].get_docpos(main_document_view);
        mw->long_jump_to_destination(candid_docpos.page, candid_docpos.y);
    }
    else {
        std::optional<PaperNameWithRects> paper_name_on_pointer = main_document_view->get_document()->get_paper_name_at_position(flat_chars, docpos);
        if (paper_name_on_pointer) {
            search_paper_name_using_external_browser(paper_name_on_pointer->paper_name, is_shift_pressed);
        }
    }

}

void NavigationController::long_jump_to_destination(DocumentPos pos) {
    AbsoluteDocumentPos abs_pos = pos.to_absolute(doc());

    if (ALIGN_LINK_DEST_TO_TOP) {
        abs_pos.y += mw->get_align_to_top_offset();

    }

    auto main_document_view = mdv();
    if (!main_document_view->is_pending_link_source_filled()) {
        push_state();
        main_document_view->set_offsets(pos.x, abs_pos.y, true);
        //main_document_view->goto_offset_within_page({ pos.page, pos.x, pos.y });
    }
    else {
        // if we press the link button and then click on a pdf link, we automatically link to the
        // link's destination


        PortalViewState dest_state;
        dest_state.document_checksum = main_document_view->get_document()->get_checksum();
        dest_state.book_state.offset_x = abs_pos.x;
        dest_state.book_state.offset_y = abs_pos.y;
        dest_state.book_state.zoom_level = main_document_view->get_zoom_level();

        mw->complete_pending_link(dest_state);
    }
    mw->invalidate_render();
}

void NavigationController::open_document(const std::wstring& doc_path,
    bool load_prev_state,
    std::optional<OpenedBookState> prev_state,
    bool force_load_dimensions) {
    mw->opengl_widget->clear_all_selections();

    auto main_document_view = mdv();
    if (main_document_view) {
        main_document_view->persist();
    }
    mw->on_open_document(doc_path);

    main_document_view->open_document(doc_path, load_prev_state, prev_state, force_load_dimensions);

    if (doc()) {
        mw->document_manager->add_tab(doc()->get_path());
        //doc()->set_only_for_portal(false);
    }

    std::optional<std::wstring> filename = Path(doc_path).filename();
    if (filename) {
        mw->setWindowTitle(QString::fromStdWString(filename.value()));
    }

    if (SCROLLBAR) {
        mw->update_scrollbar();
    }
}

void NavigationController::handle_link_click(const PdfLink& link) {
    if (link.uri.substr(0, 4).compare("http") == 0) {
        open_web_url(utf8_decode(link.uri));
        return;
    }

    if (link.uri.substr(0, 4).compare("file") == 0) {
        QString path_uri;
        if (link.uri.substr(0, 7) == "file://") {
            path_uri = QString::fromStdString(link.uri.substr(7, link.uri.size() - 7)); // skip file://
        }
        else {
            path_uri = QString::fromStdString(link.uri.substr(5, link.uri.size() - 5)); // skip file:
        }
        auto parts = path_uri.split('#');
        std::wstring path_part = parts.at(0).toStdWString();
        auto docpath = doc()->get_path();
        Path linked_file_path = Path(doc()->get_path()).file_parent().slash(path_part);
        int page = 0;
        if (parts.size() > 1) {
            if (parts.at(1).startsWith("nameddest")) {
                QString standard_uri = QString::fromStdString(link.uri);
                if (standard_uri.startsWith("file:") && !(standard_uri.startsWith("file://"))) {
                    standard_uri = "file://" + standard_uri.mid(5);
                }

                Document* linked_doc = mw->document_manager->get_document(linked_file_path.get_path());
                if (!linked_doc->doc) {
                    linked_doc->open();
                }

                if (linked_doc && linked_doc->doc) {
                    ParsedUri parsed_uri = parse_uri(mw->mupdf_context, linked_doc->doc, standard_uri.toStdString());
                    page = parsed_uri.page - 1;
                    push_state();
                    open_document_at_location(linked_file_path, page, parsed_uri.x, parsed_uri.y, {});
                    return;
                }
            }
            else {
                std::string page_string = parts.at(1).toStdString();
                page_string = page_string.substr(5, page_string.size() - 5);
                page = QString::fromStdString(page_string).toInt() - 1;
            }
        }
        push_state();
        open_document_at_location(linked_file_path, page, {}, {}, {});
        return;
    }

    auto [page, offset_x, offset_y] = parse_uri(mw->mupdf_context, doc()->doc, link.uri);

    // convert one indexed page to zero indexed page
    page--;

    if (mdv()->is_presentation_mode()) {
        mw->goto_page_with_page_number(page);
    }
    else {
        mw->long_jump_to_destination(page, offset_y);
    }
}

std::vector<OpenedBookInfo> NavigationController::get_all_opened_books(bool include_server_books, bool force_full_path) {
    std::vector<OpenedBookInfo> res;
    mw->db_manager->select_opened_books(res);
    for (int i = 0; i < res.size(); i++) {
        std::wstring path = mw->document_manager->get_path_from_hash(res[i].checksum).value_or(L"");
        if (path.size() > 0) {
            if (SHOW_DOC_PATH || force_full_path) {
                res[i].file_name = QString::fromStdWString(path);
            }
            else {
                res[i].file_name = QString::fromStdWString(Path(path).filename().value_or(L""));
            }
        }
    }

    auto new_end = std::remove_if(res.begin(), res.end(), [](OpenedBookInfo& info) {
        return info.file_name.size() == 0;
        });
    res.erase(new_end, res.end());

    if ((mw->network_controller->is_logged_in()) && include_server_books) {
        std::vector<std::string> local_file_hashes;
        for (auto info : res) {
            local_file_hashes.push_back(info.checksum);
        }
        std::vector<OpenedBookInfo> server_opened_books = mw->network_controller->get_excluded_opened_files(local_file_hashes);

        auto middle_index = res.size();

        for (auto& server_book : server_opened_books) {
            server_book.checksum = "SERVER://" + server_book.checksum;
            res.push_back(server_book);
        }

        auto last = res.end();
        // at the time of this commit, inplace_merge on android seems to be wrong, so we
        // sort the entire array instead, we should just be using the inplace_merge when
        // it is fixed
#ifndef SIOYEK_MOBILE
        std::inplace_merge(res.begin(), res.begin() + middle_index, last, [](const OpenedBookInfo& lhs, const OpenedBookInfo& rhs) {
            return lhs.last_access_time > rhs.last_access_time;
            });
#else
        std::sort(res.begin(), res.end(), [](const OpenedBookInfo& lhs, const OpenedBookInfo& rhs) {
            return lhs.last_access_time > rhs.last_access_time;
            });
#endif
    }
    return res;
}

void NavigationController::handle_open_prev_doc() {
    auto handle_select_fn = [&](std::string checksum, float offset_y) {
        if ((checksum.size() > 0) && (mw->pending_command_instance)) {
            QString doc_hash_qstring = QString::fromStdString(checksum);
            if (doc_hash_qstring.startsWith("SERVER://")) {
                doc_hash_qstring = doc_hash_qstring.mid(9);
                mw->network_controller->download_and_open(doc_hash_qstring.toStdString(), offset_y);
            }
            else {
                mw->pending_command_instance->set_generic_requirement(QList<QVariant>() << QString::fromStdString(checksum));
                mw->advance_command(std::move(mw->pending_command_instance));
            }
        }
        mw->pop_current_widget();
        };

    auto handle_delete_fn = [&](std::string checksum) {
        QString doc_hash_qstring = QString::fromStdString(checksum);
        if (doc_hash_qstring.startsWith("SERVER://")) {
            mw->network_controller->delete_file_from_server(mw, doc_hash_qstring.mid(9).toStdString(), []() {});
        }
        else {
            mw->db_manager->delete_opened_book(checksum);
        }
        };

    if (TOUCH_MODE || (!FANCY_UI_MENUS)) {
        std::vector<std::wstring> opened_docs_names;
        std::vector<std::wstring> opened_docs_actual_names;
        std::vector<OpenedBookInfo> opened_docs = get_all_opened_books();
        std::vector<OpenedBookInfo> opened_docs_instances;

        std::wstring current_path = L"";

        if (doc()) {
            current_path = doc()->get_path();
        }

        for (const auto& opened_doc : opened_docs) {
            if (QString::fromStdString(opened_doc.checksum).startsWith("SERVER://")) {
                opened_docs_names.push_back(L"[" + SERVER_SYMBOL + L"] " + opened_doc.file_name.toStdWString());
                //opened_docs_hashes.push_back(opened_doc.checksum);
                opened_docs_actual_names.push_back(opened_doc.document_title.toStdWString());
                opened_docs_instances.push_back(opened_doc);
            }
            else {

                std::optional<std::wstring> path = mw->checksummer->get_path(opened_doc.checksum);
                if (path) {
                    if (path == current_path) continue;

                    if (SHOW_DOC_PATH) {
                        opened_docs_names.push_back(path.value_or(L"<ERROR>"));
                    }
                    else {
#ifdef SIOYEK_ANDROID
                        std::wstring path_value = path.value();
                        if (path_value.substr(0, 10) == L"content://") {
                            path_value = android_file_name_from_uri(QString::fromStdWString(path_value)).toStdWString();
                        }
                        opened_docs_names.push_back(Path(path.value()).filename_no_ext().value_or(L"<ERROR>"));
#else
                        opened_docs_names.push_back(Path(path.value()).filename().value_or(L"<ERROR>"));
#endif
                    }
                    //opened_docs_hashes.push_back(opened_doc.checksum);
                    opened_docs_actual_names.push_back(opened_doc.document_title.toStdWString());
                    opened_docs_instances.push_back(opened_doc);
                }
            }
        }

        set_filtered_select_menu<OpenedBookInfo>(mw->widget_controller.get(), true, MULTILINE_MENUS, { opened_docs_names, opened_docs_actual_names }, opened_docs_instances, -1,
            [&, handle_select_fn](OpenedBookInfo* info) {
                handle_select_fn(info->checksum, info->offset_y);
            },
            [&, handle_delete_fn](OpenedBookInfo* info) {
                handle_delete_fn(info->checksum);
            }
        );

        mw->make_current_menu_columns_equal();
        mw->show_current_widget();
    }
    else {
        std::vector<OpenedBookInfo> opened_documents = get_all_opened_books();
        DocumentSelectorWidget* selector_widget = DocumentSelectorWidget::from_documents(std::move(opened_documents), mw);

        selector_widget->set_select_fn([&, selector_widget, handle_select_fn](int index) {
            OpenedBookInfo info = selector_widget->document_model->opened_documents[index];
            handle_select_fn(info.checksum, info.offset_y);
            });

        selector_widget->set_delete_fn([&, selector_widget, handle_delete_fn](int index) {
            OpenedBookInfo info = selector_widget->document_model->opened_documents[index];
            handle_delete_fn(info.checksum);
            });

        mw->set_current_widget(selector_widget);
        mw->show_current_widget();
    }

}
