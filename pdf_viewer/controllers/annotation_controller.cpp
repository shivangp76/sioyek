#include <QTimer>

#include "main_widget.h"
#include "network_manager.h"
#include "controllers/annotation_controller.h"
#include "ui/bookmark_ui.h"
#include "ui.h"

extern bool AUTOMATICALLY_UPLOAD_PORTAL_DESTINATION_FOR_SYNCED_DOCUMENTS;

AnnotationController::AnnotationController(MainWidget* parent) : mw(parent) {
}

Document* AnnotationController::doc(){
    return mw->doc();
}

DocumentView* AnnotationController::dv(){
    return mw->dv();
}

DocumentView* AnnotationController::mdv(){
    return mw->mdv();
}

void AnnotationController::scroll_selected_bookmark(int amount){
    auto bookmark_browser = mw->get_current_bookmark_browser();
    if (bookmark_browser) {
        bookmark_browser.value()->scroll_amount(amount);
    }
    else {
        std::string bookmark_uuid = mw->main_document_view->get_selected_bookmark_uuid();
        mw->scroll_bookmark_with_uuid(bookmark_uuid, amount);
    }
}


void AnnotationController::handle_bookmark_ask_query(std::wstring query, std::wstring bookmark_uuid_) {

    auto bookmark_browser = mw->get_current_bookmark_browser();
    if (bookmark_browser) {
        bookmark_browser.value()->set_pending(true);
    }

    const std::wstring& index = doc()->get_super_fast_index();
    std::string bookmark_uuid = utf8_encode(bookmark_uuid_);
    BookMark* bm_ptr = doc()->get_bookmark_with_uuid(bookmark_uuid);
    bm_ptr->description += L"\n\n";

    int first_page_end_index = doc()->get_first_page_end_index();

    QString query_qstring = QString::fromStdWString(query);

    std::wstring context = L"";
    bool use_context = false;

    if (query_qstring.contains("@selection")) {
        context = dv()->get_selected_text();
        use_context = true;
        first_page_end_index = -1;
        query_qstring = query_qstring.replace("@selection", "");
    }
    else if (query_qstring.contains("@chapter")) {
        context = mdv()->get_current_chapter_text();
        use_context = true;
        first_page_end_index = -1;
        query_qstring = query_qstring.replace("@chapter", "");
    }

    mw->sioyek_network_manager->semantic_ask(mw, query_qstring, use_context ? context : index, first_page_end_index,
        [this, bookmark_uuid, document=doc()](QString chunk) {
        BookMark* bm = mw->add_chunk_to_bookmark(document, bookmark_uuid, chunk);
        if (bm) {
            mw->update_current_bookmark_widget_text(bm);
        }
        },
        [this, bookmark_uuid, bookmark_browser, document=doc()]() {
            //int bookmark_index = document->get_bookmark_index_with_uuid(bookmark_uuid);
            BookMark* bm = document->get_bookmark_with_uuid(bookmark_uuid);
            if (bm) {
                bm->description = replace_verbatim_links(bm->description);
                bm->description += L"\n";
                document->update_bookmark_text(bookmark_uuid, bm->description, bm->font_size);
                mw->update_current_bookmark_widget_text(bm);
                mw->on_bookmark_edited(*bm, false, false);

                auto current_bookmark_browser = mw->get_current_bookmark_browser();
                if (current_bookmark_browser.has_value() && (current_bookmark_browser == bookmark_browser)) {
                    current_bookmark_browser.value()->set_pending(false);
                    QTimer::singleShot(100, [this]() {
                        auto new_current_bookmark_browser = mw->get_current_bookmark_browser();
                        if (new_current_bookmark_browser && new_current_bookmark_browser.value()->follow_output) {
                            new_current_bookmark_browser.value()->scroll_to_end();
                        }
                        });
                }
            }
        });
}

void AnnotationController::pin_current_overview_as_portal() {
    std::optional<Portal> new_portal = mdv()->pin_current_overview_as_portal();
    if (new_portal){
        mw->add_portal(doc()->get_path(), new_portal.value());
    }
}

void AnnotationController::scroll_selected_bookmark_to_end() {

    std::string bookmark_uuid = mdv()->get_selected_bookmark_uuid();
    if (bookmark_uuid.size() > 0) {
        BookMark* bookmark = doc()->get_bookmark_with_uuid(bookmark_uuid);
        if (bookmark) {
            float scroll_amount = mw->background_bookmark_renderer->get_cached_bookmark_height(bookmark_uuid) - bookmark->get_rectangle()->height() * dv()->get_zoom_level();
            dv()->set_bookmark_scroll_amount(bookmark_uuid, scroll_amount);
        }
    }
}

void AnnotationController::handle_ask() {
    float current_y_offset = mdv()->get_offset_y();
    BookMark pending_bookmark;
    pending_bookmark.description = L"";
    pending_bookmark.y_offset_ = current_y_offset;
    //std::string uuid = doc()->add_bookmark(L"", current_y_offset);
    std::string uuid = doc()->add_incomplete_bookmark(pending_bookmark);
    //on_new_bookmark_added(uuid);
    mw->open_selected_bookmark_in_widget(uuid, true, true);
}

void AnnotationController::open_selected_bookmark_in_widget(std::string bookmark_uuid, bool force_chat, bool is_bookmark_pending) {
    if (bookmark_uuid.size() == 0) {
        bookmark_uuid = mdv()->get_selected_bookmark_uuid();
    }
    auto selected_bookmark = doc()->get_bookmark_with_uuid(bookmark_uuid);
    if (selected_bookmark) {
        bool is_question_bookmark = selected_bookmark->is_question();
        QString bookmark_display_text = QString::fromStdWString(selected_bookmark->description);
        bookmark_display_text = bookmark_display_text.replace("sioyek://", "sioyeklink#");

        SioyekBookmarkTextBrowser* text_browser = new SioyekBookmarkTextBrowser(
            mw, QString::fromStdString(selected_bookmark->uuid), bookmark_display_text, is_question_bookmark || force_chat
        );
        if (is_bookmark_pending) {
            text_browser->is_bookmark_pending = true;
        }

        mw->set_current_widget(text_browser);
        // text_browser->handle_resize();
        text_browser->show();

    }
}

void AnnotationController::accept_new_bookmark_message_with_text(QString message){
    if (mw->current_widget_stack.size() > 0) {
        auto bookmark_widget = dynamic_cast<SioyekBookmarkTextBrowser*>(mw->current_widget_stack.back());
        if (bookmark_widget){
            if (bookmark_widget->is_bookmark_pending) {
                doc()->add_pending_bookmark(bookmark_widget->bookmark_uuid.toStdString(), L"");
                bookmark_widget->is_bookmark_pending = false;
            }
            QString text_ = message;
            QString text;

            for (auto line : text_.split("\n")) {
                text += "? " + line + "\n";
            }

            auto bookmark = doc()->get_bookmark_with_uuid(bookmark_widget->bookmark_uuid.toStdString());

            if (bookmark) {
                bookmark->description += text.toStdWString();
                handle_bookmark_ask_query(bookmark->description, utf8_decode(bookmark->uuid));
                bookmark_widget->set_follow_output(true);
            }
        }
    }
}

void AnnotationController::accept_new_bookmark_message() {
    if (mw->current_widget_stack.size() > 0) {
        auto bookmark_widget = dynamic_cast<SioyekBookmarkTextBrowser*>(mw->current_widget_stack.back());
        if (bookmark_widget && bookmark_widget->line_edit && (!bookmark_widget->is_pending)) {
            QString text_ = bookmark_widget->line_edit->text();
            accept_new_bookmark_message_with_text(text_);
            bookmark_widget->line_edit->clear();
        }
    }
}

void AnnotationController::update_current_bookmark_widget_text(BookMark* bm) {
    if (mw->current_widget_stack.size() > 0) {
        auto bookmark_widget = dynamic_cast<SioyekBookmarkTextBrowser*>(mw->current_widget_stack.back());
        if (bookmark_widget) {
            if (bookmark_widget->bookmark_uuid.toStdString() == bm->uuid) {
                QString bookmark_display_text = QString::fromStdWString(bm->description);
                bookmark_display_text = bookmark_display_text.replace("sioyek://", "sioyeklink#");
                bookmark_widget->update_text(bookmark_display_text);
            }
        }
    }
}

void AnnotationController::handle_scroll_selected_bookmark_to_ends(bool goto_start){

    auto bookmark_browser = mw->get_current_bookmark_browser();
    if (bookmark_browser) {
        if (goto_start) {
            bookmark_browser.value()->scroll_to_start();
        }
        else {
            bookmark_browser.value()->scroll_to_end();
        }
    }
    else {
        std::string bookmark_uuid = mdv()->get_selected_bookmark_uuid();
        if (goto_start) {
            mw->scroll_bookmark_with_uuid(bookmark_uuid, -INT_MAX / 2);
        }
        else {
            mw->scroll_bookmark_with_uuid(bookmark_uuid, INT_MAX / 2);
        }
    }
}

BookMark* AnnotationController::add_chunk_to_bookmark(Document* document, std::string bookmark_uuid, QString chunk) {
    // int bookmark_index = document->get_bookmark_index_with_uuid(bookmark_uuid);
    BookMark* bookmark_ptr = document->get_bookmark_with_uuid(bookmark_uuid);
    if (bookmark_ptr) {
        BookMark& bm = *bookmark_ptr;
        bm.is_pending = false;
        bm.description += chunk.toStdWString();
        for (auto& following_window : mw->following_windows) {
            if (following_window.bookmark_uuid == bm.uuid) {
                following_window.file->open(QFile::WriteOnly);
                following_window.file->write(QString::fromStdWString(bm.description).toUtf8());
                following_window.file->close();
            }
        }

        mw->invalidate_render();
        return &bm;
    }
    return nullptr;
}

void AnnotationController::handle_bookmark_summarize_query(std::wstring bookmark_uuid_) {
    if (!mw->ensure_super_fast_search_index()) {
        return;
    }
    const std::wstring& index = doc()->get_super_fast_index();

    int first_page_end_index = doc()->get_first_page_end_index();

    std::string bookmark_uuid = utf8_encode(bookmark_uuid_);
    BookMark* bm_ptr = doc()->get_bookmark_with_uuid(bookmark_uuid);
    bm_ptr->description += L"\n\n";
    mw->sioyek_network_manager->summarize(mw,  index, first_page_end_index, [this, bookmark_uuid, document=doc()](QString chunk) {
        add_chunk_to_bookmark(document, bookmark_uuid, chunk);
        },
        [this, bookmark_uuid, document=doc()]() {
            //int bookmark_index = document->get_bookmark_index_with_uuid(bookmark_uuid);
            BookMark* bm = document->get_bookmark_with_uuid(bookmark_uuid);
            if (bm) {
                bm->is_pending = false;
                bm->description = replace_verbatim_links(bm->description);
                document->update_bookmark_text(bookmark_uuid, bm->description, bm->font_size);
                on_bookmark_edited(*bm, false, false);
            }
        });
}


void AnnotationController::on_bookmark_edited(BookMark bm, bool was_manual_edit, bool was_move_or_resize) {
    if (mw->edit_bookmark_hook_function_name) {
        mw->call_js_function_with_bookmark_arg_with_uuid(mw->edit_bookmark_hook_function_name.value(), bm.uuid);
    }
    if (was_manual_edit && !was_move_or_resize) {
        QStringList lines = QString::fromStdWString(bm.description).split("\n", Qt::SkipEmptyParts);
        if (lines.size() > 0) {
            if (lines.last().trimmed().startsWith("? ")) {
                //qDebug() << "we should continue";
                handle_bookmark_ask_query(bm.description, utf8_decode(bm.uuid));
            }
        }

    }
    mw->sync_edited_annot("bookmark", bm.uuid);
}

void AnnotationController::delete_highlight_with_uuid(const std::string& uuid) {
    //db_manager->delete_highlight(uuid);
    std::vector<std::pair<std::string, Highlight>> deleted_highlight;
    mw->db_manager->select_highlight_with_uuid(uuid, deleted_highlight);

    if (deleted_highlight.size() > 0) {
        if (deleted_highlight[0].first == doc()->get_checksum()) {
            doc()->delete_highlight(deleted_highlight[0].second);
        }
        else {
            mw->db_manager->delete_highlight(uuid);
        }

        mw->on_highlight_deleted(deleted_highlight[0].second, deleted_highlight[0].first);
    }
}

std::optional<Highlight> AnnotationController::delete_current_document_highlight_with_uuid(const std::string& uuid) {
    std::optional<Highlight> deleted_highlight = doc()->delete_highlight_with_uuid(uuid);
    if (deleted_highlight.has_value()) {
        mw->on_highlight_deleted(deleted_highlight.value(), doc()->get_checksum());
    }
    return deleted_highlight;
}

void AnnotationController::delete_current_document_highlight(Highlight* hl) {
    std::string uuid = hl->uuid;
    std::optional<Highlight> deleted_highlight = doc()->delete_highlight(*hl);
    if (deleted_highlight) {
        mw->on_highlight_deleted(deleted_highlight.value(), doc()->get_checksum());
    }
}

void AnnotationController::on_highlight_deleted(const Highlight& hl, const std::string& document_checksum){

    DeletedObject deleted_highlight_object;
    deleted_highlight_object.document_checksum = document_checksum;
    deleted_highlight_object.object = hl;
    mw->deleted_objects.push_back(deleted_highlight_object);

    if (mw->delete_highlight_hook_function_name) {
        mw->call_async_js_function_with_args(mw->delete_highlight_hook_function_name.value(), QJsonArray() << QString::fromStdString(hl.uuid));
    }
    mw->sync_deleted_annot("highlight", hl.uuid);
}

void AnnotationController::on_bookmark_deleted(const BookMark& bookmark, const std::string& document_checksum){
    DeletedObject deleted_bookmark_object;
    deleted_bookmark_object.document_checksum = document_checksum;
    deleted_bookmark_object.object = bookmark;
    mw->deleted_objects.push_back(deleted_bookmark_object);

    if (mw->delete_bookmark_hook_function_name) {
        //call_async_js_function_with_args(delete_bookmark_hook_function_name.value(), QJsonArray() << QString::fromStdString(bookmark.uuid));
        mw->call_async_js_function_with_args(mw->delete_bookmark_hook_function_name.value(), QJsonArray() << bookmark.to_json(document_checksum));
    }

    // if we have deleted a bookmark which shows the output of a command
    // we need to kill the process and remove it from shell_output_bookmarks
    for (int i = 0; i < mw->shell_output_bookmarks.size(); i++){
        if (mw->shell_output_bookmarks[i].uuid == bookmark.uuid) {
            kill_process(mw->shell_output_bookmarks[i].pid);
            mw->remove_finished_shell_bookmark_with_index(i);
            break;
        }
    }

    mw->sync_deleted_annot("bookmark", bookmark.uuid);
}

void AnnotationController::on_new_bookmark_added(const std::string& uuid) {
    if (mw->add_bookmark_hook_function_name) {
        mw->call_js_function_with_bookmark_arg_with_uuid(mw->add_bookmark_hook_function_name.value(), uuid);
    }

    mw->sync_newly_added_annot("bookmark", uuid);

    doc()->update_last_local_edit_time();
}

void AnnotationController::on_new_portal_added(const std::string& uuid) {
    if (mw->add_portal_hook_function_name) {
        mw->call_js_function_with_portal_arg_with_uuid(mw->add_bookmark_hook_function_name.value(), uuid);
    }
    mw->sync_newly_added_annot("portal", uuid);
    if (AUTOMATICALLY_UPLOAD_PORTAL_DESTINATION_FOR_SYNCED_DOCUMENTS) {
        int portal_index = doc()->get_portal_index_with_uuid(uuid);
        if (portal_index >= 0) {
            const Portal& portal = doc()->get_portals()[portal_index];
            if (!mw->sioyek_network_manager->is_checksum_available_on_server(portal.dst.document_checksum)) {
                std::optional<std::wstring> document_path = mw->document_manager->get_path_from_hash(portal.dst.document_checksum);
                if (document_path) {
                    mw->sioyek_network_manager->upload_file(
                        QApplication::instance(),
                        QString::fromStdWString(document_path.value()),
                        QString::fromStdString(portal.dst.document_checksum), [network_manager=mw->sioyek_network_manager]() {
                            network_manager->update_user_files_hash_set();
                        }, [](){});
                }
            }
        }
    }

    doc()->update_last_local_edit_time();
}

void AnnotationController::on_portal_deleted(const Portal& deleted_portal, const std::string& document_checksum) {

    DeletedObject deleted_portal_object;
    deleted_portal_object.document_checksum = document_checksum;
    deleted_portal_object.object = deleted_portal;
    mw->deleted_objects.push_back(deleted_portal_object);

    if (mw->delete_portal_hook_function_name) {
        mw->call_async_js_function_with_args(mw->delete_portal_hook_function_name.value(),
            QJsonArray() << QString::fromStdString(deleted_portal.uuid));
    }
    mw->sync_deleted_annot("portal", deleted_portal.uuid);
}

void AnnotationController::on_portal_edited(const std::string& uuid) {
    if (mw->edit_portal_hook_function_name) {
        mw->call_async_js_function_with_args(mw->edit_portal_hook_function_name.value(),
            QJsonArray() << QString::fromStdString(uuid));
    }
    mw->sync_edited_annot("portal", uuid);
}

void AnnotationController::on_mark_added(const std::string& uuid, char type) {
    if (mw->add_mark_hook_function_name) {

        QString type_string = QString(QChar(type));
        mw->call_async_js_function_with_args(mw->add_mark_hook_function_name.value(), QJsonArray() << QString::fromStdString(uuid) << type_string);
    }
}

void AnnotationController::sync_newly_added_annot(const std::string& annot_type, const std::string& uuid) {
    if (mw->is_current_document_available_on_server()) {
        //int highlight_index = doc()->get_highlight_index_with_uuid(uuid);
        const Annotation* annot = doc()->get_annot_with_uuid(annot_type, uuid);
        std::optional<std::string> checksum = doc()->get_checksum_fast();
        if ((annot != nullptr) && checksum) {
            mw->sioyek_network_manager->upload_annot(mw,
                QString::fromStdString(checksum.value()),
                *annot,
                [&, uuid, this, annot_type, document=doc()]() { // on success
                    if (document != doc()) return;
                    std::vector<std::string> uuids = { uuid };
                    doc()->set_annots_to_synced_with_type(annot_type, uuids);
                },
                [&, uuid, this]() { // on failure
                }
            );
        }
    }
}

void AnnotationController::on_new_highlight_added(const std::string& uuid) {
    if (mw->add_highlight_hook_function_name) {
        mw->call_js_function_with_highlight_arg_with_uuid(mw->add_highlight_hook_function_name.value(), uuid);
    }
    sync_newly_added_annot("highlight", uuid);

    doc()->update_last_local_edit_time();
}

void AnnotationController::on_highlight_annotation_edited(const std::string& uuid) {
    if (mw->highlight_annotation_changed_hook_function_name) {
        mw->call_js_function_with_highlight_arg_with_uuid(mw->highlight_annotation_changed_hook_function_name.value(), uuid);
    }
    mw->sync_edited_annot("highlight", uuid);
}

void AnnotationController::on_highlight_type_edited(const std::string& uuid) {
    if (mw->highlight_type_changed_hook_function_name) {
        mw->call_js_function_with_highlight_arg_with_uuid(mw->highlight_type_changed_hook_function_name.value(), uuid);
    }
    mw->sync_edited_annot("highlight", uuid);
}

void AnnotationController::sync_edited_annot(const std::string& annot_type, const std::string& uuid) {
    if (mw->is_current_document_available_on_server()) {
        const Annotation* annot = doc()->get_annot_with_uuid(annot_type, uuid);
        if (annot) {
            std::optional<std::string> doc_checksum = doc()->get_checksum_fast();
            if (doc_checksum.has_value()) {
                mw->sioyek_network_manager->upload_annot(mw,
                    QString::fromStdString(doc_checksum.value()),
                    *annot,
                    []() {},
                    []() {}
                );
            }
        }
    }
    doc()->update_last_local_edit_time();
}

std::string AnnotationController::add_highlight_to_current_document(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type) {
    std::string uuid = mw->main_document_view->add_highlight_(selection_begin, selection_end, type);
    on_new_highlight_added(uuid);
    return uuid;
}

std::wstring AnnotationController::handle_add_highlight(char symbol) {
    if (mdv()->selected_character_rects.size() > 0) {
        std::string uuid = add_highlight_to_current_document(dv()->selection_begin, dv()->selection_end, symbol);
        mdv()->clear_selected_text();
        return utf8_decode(uuid);
    }
    else {
        mw->change_selected_highlight_type(symbol);
        return utf8_decode(mdv()->get_selected_highlight_uuid());
    }
}

void AnnotationController::change_selected_highlight_type(char new_type) {
    std::string selected_highlight_uuid = mdv()->get_selected_highlight_uuid();
    if (selected_highlight_uuid.size() > 0) {
        doc()->update_highlight_type(selected_highlight_uuid, new_type);
        on_highlight_type_edited(selected_highlight_uuid);
    }
}

char AnnotationController::get_current_selected_highlight_type() {
    std::string selected_highlight_uuid = mdv()->get_selected_highlight_uuid();
    if (selected_highlight_uuid.size() > 0) {
        Highlight* highlight = doc()->get_highlight_with_uuid(selected_highlight_uuid);
        if (highlight) {
            return highlight->type;
        }
    }
    return 'a';
}
