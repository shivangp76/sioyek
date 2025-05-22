#include <QPushButton>

#include "controllers/navigation_controller.h"
#include "main_widget.h"
#include "controllers/network_controller.h"
#include "pdf_view_opengl_widget.h"

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
