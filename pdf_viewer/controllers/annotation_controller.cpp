#include <QTimer>
#include <QLineEdit>
#include <QApplication>

#include "main_widget.h"
#include "network_manager.h"
#include "controllers/annotation_controller.h"
#include "ui/bookmark_ui.h"
#include "checksum.h"
#include "commands/base_commands.h"
#include "ui/common_ui.h"
#include "ui/ui_models.h"
#include "ui/annotation_widgets.h"

extern bool AUTOMATICALLY_UPLOAD_PORTAL_DESTINATION_FOR_SYNCED_DOCUMENTS;
extern bool SORT_BOOKMARKS_BY_LOCATION;
extern bool SORT_HIGHLIGHTS_BY_LOCATION;
extern std::wstring ITEM_LIST_PREFIX;
extern bool FUZZY_SEARCHING;
extern bool MULTILINE_MENUS;
extern bool NAVIGATE_BOOKMARK_LINKS_AFTER_SELECTION;
extern bool FANCY_UI_MENUS;
extern std::wstring SERVER_SYMBOL;
extern std::map<std::wstring, std::wstring> SHELL_BOOKMARK_COMMANDS;
extern float FREETEXT_BOOKMARK_FONT_SIZE;

AnnotationController::AnnotationController(MainWidget* parent) : BaseController(parent) {
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
            QString text_ = bookmark_widget->get_line_edit()->text();
            accept_new_bookmark_message_with_text(text_);
            bookmark_widget->get_line_edit()->clear();
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

void AnnotationController::handle_goto_portal_list() {
    std::vector<std::wstring> option_names;
    std::vector<std::wstring> option_location_strings;
    std::vector<Portal> portals;

    if (!doc()) return;

    if (SORT_BOOKMARKS_BY_LOCATION) {
        portals = mdv()->get_document()->get_sorted_portals();
    }
    else {
        portals = mdv()->get_document()->get_portals();
    }

    for (auto portal : portals) {
        std::wstring portal_type_string = L"[*]";
        if (!portal.is_visible()) {
            portal_type_string = L"[.]";
        }

        option_names.push_back(ITEM_LIST_PREFIX + L" " + portal_type_string + L" " +  mw->checksummer->get_path(portal.dst.document_checksum).value_or(L"[ERROR]"));
        //option_locations.push_back(bookmark.y_offset);
        auto [page, _, __] = mdv()->get_document()->absolute_to_page_pos({ 0, portal.src_offset_y });
        option_location_strings.push_back(get_page_formatted_string(page + 1));
    }

    int closest_portal_index = mdv()->get_document()->find_closest_portal_index(portals, mdv()->get_offset_y());

    set_filtered_select_menu<Portal>(mw, FUZZY_SEARCHING, MULTILINE_MENUS, { option_names, option_location_strings }, portals, closest_portal_index,
        [&](Portal* portal) {
            mw->pending_command_instance->set_generic_requirement(portal->src_offset_y);
            mw->advance_command(std::move(mw->pending_command_instance));
            mw->pop_current_widget();

        },
        [&](Portal* portal) {
            std::string uuid = portal->uuid;
            std::optional<Portal> deleted_portal = doc()->delete_portal_with_uuid(uuid);
            if (deleted_portal) {
                on_portal_deleted(deleted_portal.value(), doc()->get_checksum());
            }
        },
            [&](Portal* portal) {
                mw->portal_to_edit = *portal;
                mw->open_document(portal->dst);
                mw->pop_current_widget();
                mw->invalidate_render();
        }
        );

    mw->show_current_widget();
}

void AnnotationController::handle_goto_bookmark(bool manual_only, bool chat) {
    //std::vector<std::wstring> option_names;
    std::vector<QString> option_location_strings;
    std::vector<BookMark> bookmarks;

    if (!doc()) return;

    if (SORT_BOOKMARKS_BY_LOCATION) {
        bookmarks = mdv()->get_document()->get_sorted_bookmarks();
    }
    else {
        bookmarks = mdv()->get_document()->get_bookmarks();
    }

    if (manual_only) {
        auto predicate = [](const BookMark& bm) {
            return bm.is_question() || bm.is_summary();
            };

        bookmarks.erase(std::remove_if(bookmarks.begin(), bookmarks.end(), predicate), bookmarks.end());
    }
    else if (chat) {
        auto predicate = [](const BookMark& bm) {
            return !bm.is_question();
            };
        bookmarks.erase(std::remove_if(bookmarks.begin(), bookmarks.end(), predicate), bookmarks.end());
    }

    for (auto bookmark : bookmarks) {
        //option_names.push_back(ITEM_LIST_PREFIX + L" " + bookmark.description);
        //option_locations.push_back(bookmark.y_offset);
        auto [page, _, __] = mdv()->get_document()->absolute_to_page_pos({ 0, bookmark.get_y_offset()});
        option_location_strings.push_back(QString::fromStdWString(get_page_formatted_string(page + 1)));
    }

    int closest_bookmark_index = mdv()->get_document()->find_closest_bookmark_index(bookmarks, mdv()->get_offset_y());

    auto handle_select_fn = [&, chat](BookMark bm) {
        if (mw->pending_command_instance) {
            if (chat) {
                mw->pending_command_instance->set_generic_requirement(QString::fromStdString(bm.uuid));
            }
            else {
                mw->pending_command_instance->set_generic_requirement(bm.get_y_offset());
            }
        }

        if (!chat && NAVIGATE_BOOKMARK_LINKS_AFTER_SELECTION && bm.can_have_links()) {
            auto [link_names, link_targets] = bm.get_links();
            std::vector<std::wstring> queries;
            std::vector<QString> messages;

            for (int i = 0; i < link_names.size(); i++) {
                QString link = link_targets[i];
                QString message = link_names[i];
                if (link.startsWith("sioyek://")) {
                    QString decoded = QUrl::fromPercentEncoding(link.mid(9).toUtf8());
                    QStringList parts = decoded.split("...");
                    for (auto part : parts) {
                        queries.push_back(part.trimmed().toStdWString());
                        messages.push_back(message);
                    }
                }
            }
            dv()->perform_fuzzy_searches(queries, messages);
        }

        mw->pop_current_widget();
        mw->advance_command(std::move(mw->pending_command_instance));
        };

    auto handle_delete_fn = [&](BookMark bm) {
        mw->delete_current_document_bookmark_with_bookmark(&bm);
        };

    auto handle_edit_fn = [&](BookMark bm) {
        mdv()->set_selected_bookmark_uuid(bm.uuid);
        mw->pop_current_widget();
        mw->handle_command_types(mw->command_manager->get_command_with_name(mw, "edit_selected_bookmark"), 0);
        };

    if (TOUCH_MODE) {
        BookmarkModel* bookmark_model = new BookmarkModel(std::move(bookmarks), std::move(option_location_strings), {}, mw);

        TouchDelegateListView* lv = new TouchDelegateListView(bookmark_model, true, "TouchBookmarksView", {std::make_pair("_selected_index", closest_bookmark_index)}, mw);
        // lv->list_view->proxy_model->set_is_highlight(true);
        lv->list_view->proxy_model->setFilterKeyColumn(-1);

        lv->set_select_fn([&, bookmark_model, handle_select_fn](int index) {
            BookMark bm = bookmark_model->bookmarks[index];
            handle_select_fn(bm);
            });

        lv->set_delete_fn(
            [&, bookmark_model, handle_delete_fn](int index) {
                BookMark bm = bookmark_model->bookmarks[index];
                handle_delete_fn(bm);
            }
        );

        mw->set_current_widget(lv);
        mw->show_current_widget();

    }
    else{
        if (!FANCY_UI_MENUS) {
            std::vector<std::wstring> option_names;
            std::vector<std::wstring> option_location_wstrings;
            //std::vector<BookMark> bookmarks;

            for (auto bookmark : bookmarks) {
                // option_names.push_back(ITEM_LIST_PREFIX + L" " + bookmark.description);
                // option_names.push_back(ITEM_LIST_PREFIX + L" " + bookmark.get_render_text().toStdWString());
                option_names.push_back(bookmark.get_render_text().toStdWString());
                auto [page, _, __] = mdv()->get_document()->absolute_to_page_pos({ 0, bookmark.get_y_offset() });
                option_location_wstrings.push_back(get_page_formatted_string(page + 1));
            }

            set_filtered_select_menu<BookMark>(mw, FUZZY_SEARCHING, MULTILINE_MENUS, { option_names, option_location_wstrings }, bookmarks, closest_bookmark_index,
                                               [&, handle_select_fn](BookMark* bm) {
                handle_select_fn(*bm);
            },
            [&, handle_delete_fn](BookMark* bm) {
                handle_delete_fn(*bm);
            },
            [&, handle_edit_fn](BookMark* bm) {
                handle_edit_fn(*bm);
            }
            );
            mw->show_current_widget();
        }
        else {
            BookmarkSelectorWidget* bookmark_widget = BookmarkSelectorWidget::from_bookmarks(
                        std::move(bookmarks), mw, std::move(option_location_strings));
            bookmark_widget->set_selected_index(closest_bookmark_index);

            bookmark_widget->set_select_fn([&, bookmark_widget, handle_select_fn](int index) {
                BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
                handle_select_fn(bm);

            });

            bookmark_widget->set_delete_fn([&, bookmark_widget, handle_delete_fn](int index) {
                BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
                handle_delete_fn(bm);
            });

            bookmark_widget->set_edit_fn([&, bookmark_widget, handle_edit_fn](int index) {
                BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
                handle_edit_fn(bm);
            });

            mw->set_current_widget(bookmark_widget);
            mw->show_current_widget();

        }
    }
}

void AnnotationController::handle_goto_bookmark_global(bool manual_only) {
    std::vector<std::pair<std::string, BookMark>> global_bookmarks;
    mw->db_manager->global_select_bookmark(global_bookmarks);

    if (manual_only) {
        auto predicate = [](const std::pair<std::string, BookMark>& bm) {
            return bm.second.is_question() || bm.second.is_summary();
            };

        global_bookmarks.erase(std::remove_if(global_bookmarks.begin(), global_bookmarks.end(), predicate), global_bookmarks.end());
    }

    auto handle_select_fn = [&](QString checksum, float offset_y) {
        if (checksum.startsWith("SERVER://")) {
            mw->download_and_open(checksum.mid(9).toStdString(), offset_y);
        }
        else {
            if (mw->pending_command_instance) {
                QString file_path = QString::fromStdWString(mw->document_manager->get_path_from_hash(checksum.toStdString()).value_or(L""));
                mw->pending_command_instance->set_generic_requirement(QList<QVariant>() << file_path << offset_y);
            }
            mw->advance_command(std::move(mw->pending_command_instance));
            mw->pop_current_widget();
        }
        };

    auto handle_delete_fn = [&](std::string uuid) {
        mw->delete_global_bookmark(uuid);
        };


    std::vector<BookMark> bookmarks;
    std::vector<QString> file_names;
    std::vector<QString> file_checksums;

    for (const auto& desc_bm_pair : global_bookmarks) {
        std::string checksum = desc_bm_pair.first;
        bool is_remote = false;
        std::optional<std::wstring> path = mw->checksummer->get_path(checksum);
        if (!path.has_value() && mw->sioyek_network_manager->is_checksum_available_on_server(checksum)) {
            is_remote = true;
            path = L"SERVER://" + utf8_decode(checksum);
        }
        if (path) {
            BookMark bm = desc_bm_pair.second;
            std::wstring file_name = is_remote ? SERVER_SYMBOL  : Path(path.value()).filename().value_or(L"");
            bookmarks.push_back(bm);
            file_names.push_back(QString::fromStdWString(path.value_or(L"")));
            file_checksums.push_back(QString::fromStdString(checksum));
        }
    }
    if (TOUCH_MODE){

        BookmarkModel* bookmark_model = new BookmarkModel(std::move(bookmarks), std::move(file_names), std::move(file_checksums), mw);

        TouchDelegateListView* lv = new TouchDelegateListView(bookmark_model, true, "TouchBookmarksView", {}, mw);
        // lv->list_view->proxy_model->set_is_highlight(true);
        lv->list_view->proxy_model->setFilterKeyColumn(-1);

        lv->set_select_fn([&, bookmark_model, handle_select_fn](int index) {
            BookMark bm = bookmark_model->bookmarks[index];
            auto checksum = bookmark_model->checksums[index];

            handle_select_fn(checksum, bm.get_y_offset());
            });

        lv->set_delete_fn(
            [&, bookmark_model, handle_delete_fn](int index) {
                BookMark bm = bookmark_model->bookmarks[index];
                handle_delete_fn(bm.uuid);
            }
        );

        mw->set_current_widget(lv);
        mw->show_current_widget();
    }
    else{
        if (!FANCY_UI_MENUS) {
            std::vector<std::wstring> descs;
            std::vector<std::wstring> file_names_wstring;
            std::vector<std::pair<BookState, std::string>> book_states;
            for (int i = 0; i < bookmarks.size(); i++) {
                descs.push_back(bookmarks[i].description);
                file_names_wstring.push_back(file_names[i].toStdWString());
                BookState book_state;
                book_state.document_path = file_checksums[i].toStdWString();
                book_state.offset_y = bookmarks[i].get_y_offset();
                book_states.push_back(std::make_pair(book_state, bookmarks[i].uuid));
            }

            set_filtered_select_menu<std::pair<BookState, std::string>>(mw, FUZZY_SEARCHING, MULTILINE_MENUS, { descs, file_names_wstring }, book_states, -1,
                                                                        [&, handle_select_fn](std::pair<BookState, std::string>* book_state) {
                QString path = QString::fromStdWString(book_state->first.document_path);

                handle_select_fn(path, book_state->first.offset_y);

            },
            [&, handle_delete_fn](std::pair<BookState, std::string>* book_state) {
                handle_delete_fn(book_state->second);
            }
            );
            mw->show_current_widget();
        }
        else {
            BookmarkSelectorWidget* bookmark_widget = BookmarkSelectorWidget::from_bookmarks(
                        std::move(bookmarks), mw, std::move(file_names), std::move(file_checksums)
                        );

            bookmark_widget->set_select_fn([&, bookmark_widget, handle_select_fn](int index) {
                QString path = bookmark_widget->bookmark_model->checksums[index];
                BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
                QString file_path = bookmark_widget->bookmark_model->documents[index];
                handle_select_fn(path, bm.get_y_offset());
            });

            bookmark_widget->set_delete_fn(
                        [&, bookmark_widget, handle_delete_fn](int index) {
                BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
                handle_delete_fn(bm.uuid);
            }
            );

            mw->set_current_widget(bookmark_widget);
            mw->show_current_widget();
        }
    }
}

void AnnotationController::handle_goto_highlight() {
    std::vector<Highlight> highlights;

    if (SORT_HIGHLIGHTS_BY_LOCATION) {
        highlights = doc()->get_highlights_sorted();
    }
    else {
        highlights = doc()->get_highlights();
    }

    std::vector<QString> page_numbers;
    page_numbers.reserve(highlights.size());

    for (auto hl : highlights) {
        page_numbers.push_back(QString::number(hl.selection_begin.to_document(doc()).page));
    }

    int closest_highlight_index = doc()->find_closest_highlight_index(highlights, mdv()->get_offset_y());


    auto handle_select_fn = [&](Highlight hl) {
        if (mw->pending_command_instance) {
            mw->pending_command_instance->set_generic_requirement(hl.selection_begin.y);
        }
        mw->advance_command(std::move(mw->pending_command_instance));
        mw->pop_current_widget();
        };

    auto handle_edit_fn = [&](Highlight hl) {
        mdv()->set_selected_highlight_uuid(hl.uuid);
        mw->pop_current_widget();

        std::unique_ptr<Command> cmd = mw->command_manager->get_command_with_name(mw, "edit_selected_highlight");
        cmd->pre_perform();
        mw->advance_command(std::move(cmd));
        };

    auto handle_delete_fn = [&](Highlight hl) {
        delete_current_document_highlight(&hl);
        mw->invalidate_render();
        };

    if (TOUCH_MODE) {
        HighlightModel* highlights_model = new HighlightModel(std::move(highlights), std::move(page_numbers), {}, mw);

        TouchDelegateListView* lv = new TouchDelegateListView(highlights_model, true, "TouchHighlightsView", { std::make_pair("_colorMap", get_color_mapping()), std::make_pair("_selected_index", closest_highlight_index)}, mw);
        lv->list_view->proxy_model->set_is_highlight(true);
        lv->list_view->proxy_model->setFilterKeyColumn(-1);

        lv->set_select_fn([&, highlights_model, handle_select_fn](int index) {
            Highlight hl = highlights_model->highlights[index];
            handle_select_fn(hl);
            });

        lv->set_delete_fn(
            [&, highlights_model, handle_delete_fn](int index) {
                Highlight hl = highlights_model->highlights[index];
                handle_delete_fn(hl);
            }
        );

        mw->set_current_widget(lv);
        mw->show_current_widget();

    }
    else {
        if (FANCY_UI_MENUS) {
            HighlightSelectorWidget* highlight_selector_widget = HighlightSelectorWidget::from_highlights(std::move(highlights), mw, std::move(page_numbers));
            highlight_selector_widget->set_selected_index(closest_highlight_index);

            highlight_selector_widget->set_select_fn(
                [&, highlight_selector_widget, handle_select_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    handle_select_fn(hl);
                }
            );

            highlight_selector_widget->set_delete_fn(
                [&, highlight_selector_widget, handle_delete_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    handle_delete_fn(hl);
                }
            );
            highlight_selector_widget->set_edit_fn(
                [&, highlight_selector_widget, handle_edit_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    handle_edit_fn(hl);
                }
            );

            mw->set_current_widget(highlight_selector_widget);
            mw->show_current_widget();
        }
        else {
            std::vector<std::wstring> option_names;
            std::vector<std::wstring> option_location_wstrings;

            for (auto highlight : highlights) {
                option_names.push_back(ITEM_LIST_PREFIX + L" " + highlight.description);
                int page = highlight.selection_begin.to_document(doc()).page;
                option_location_wstrings.push_back(get_page_formatted_string(page + 1));
            }

            set_filtered_select_menu<Highlight>(mw, FUZZY_SEARCHING, MULTILINE_MENUS, { option_names, option_location_wstrings }, highlights, closest_highlight_index,
                [&, handle_select_fn](Highlight* hl) {
                    handle_select_fn(*hl);
                },
                [&, handle_delete_fn](Highlight* hl) {
                    handle_delete_fn(*hl);
                },
                [&, handle_edit_fn](Highlight* hl) {
                    handle_edit_fn(*hl);
                }
            );
            mw->show_current_widget();

        }
    }
}

void AnnotationController::handle_goto_highlight_global() {
    std::vector<std::pair<std::string, Highlight>> global_highlights;
    mw->db_manager->global_select_highlight(global_highlights);

    auto handle_select_fn = [&](float offset_y, std::string checksum) {
        if (checksum.size() > 0) {
            if (QString::fromStdString(checksum).startsWith("SERVER://")) {
                mw->download_and_open(QString::fromStdString(checksum).mid(9).toStdString(), offset_y);

            }
            else {
                if (mw->pending_command_instance) {
                    QString file_path = QString::fromStdWString(mw->checksummer->get_path(checksum).value_or(L""));
                    mw->pending_command_instance->set_generic_requirement(
                        QList<QVariant>() << file_path << offset_y);
                }
                mw->advance_command(std::move(mw->pending_command_instance));
            }
            mw->pop_current_widget();
        }
    };

    auto handle_delete_fn = [&](std::string uuid) {
            delete_highlight_with_uuid(uuid);
        };

    std::vector<QString> file_names;
    std::vector<QString> file_checksums;
    std::vector<Highlight> highlights;

    for (auto [checksum, hl] : global_highlights) {

        QString file_name = QString::fromStdWString(mw->checksummer->get_path(checksum).value_or(L""));
        if (file_name.size() == 0) {
            if (mw->sioyek_network_manager->is_checksum_available_on_server(checksum)) {
                file_name = QString::fromStdWString(SERVER_SYMBOL);
                checksum = "SERVER://" + checksum;
            }
            else {
                continue;
                //file_name = QString::fromStdString(checksum);
            }
        }

        highlights.push_back(hl);
        file_checksums.push_back(QString::fromStdString(checksum));
        file_names.push_back(file_name);
    }

    if (TOUCH_MODE) {
        //HighlightSelectorWidget* highlight_selector_widget = HighlightSelectorWidget::from_highlights(std::move(highlights), this, std::move(file_names), std::move(file_checksums));
        HighlightModel* highlights_model = new HighlightModel(std::move(highlights), std::move(file_names), std::move(file_checksums), mw);

        TouchDelegateListView* lv = new TouchDelegateListView(highlights_model, true, "TouchHighlightsView", { std::make_pair("_colorMap", get_color_mapping())}, mw);
        lv->list_view->proxy_model->set_is_highlight(true);
        lv->list_view->proxy_model->setFilterKeyColumn(-1);

        lv->set_select_fn(
            [&, highlights_model, handle_select_fn](int index) {
                Highlight hl = highlights_model->highlights[index];
                std::string checksum = highlights_model->checksums[index].toStdString();
                handle_select_fn(hl.selection_begin.y, checksum);
            });

        lv->set_delete_fn([&, highlights_model, handle_delete_fn](int index) {
            Highlight hl = highlights_model->highlights[index];
            handle_delete_fn(hl.uuid);
            });


        mw->set_current_widget(lv);
        mw->show_current_widget();
    }
    else {
        if (FANCY_UI_MENUS) {
            HighlightSelectorWidget* highlight_selector_widget = HighlightSelectorWidget::from_highlights(std::move(highlights), mw, std::move(file_names), std::move(file_checksums));

            highlight_selector_widget->set_select_fn(
                [&, highlight_selector_widget, handle_select_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    std::string checksum = highlight_selector_widget->highlight_model->checksums[index].toStdString();
                    handle_select_fn(hl.selection_begin.y, checksum);
                });

            highlight_selector_widget->set_delete_fn([&, highlight_selector_widget, handle_delete_fn](int index) {
                Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                handle_delete_fn(hl.uuid);
                });

            mw->set_current_widget(highlight_selector_widget);
            mw->show_current_widget();
        }
        else {
            std::vector<std::wstring> descs;
            std::vector<std::wstring> file_names_wstring;
            std::vector<std::pair<BookState, std::string>> book_states;
            for (int i = 0; i < highlights.size(); i++) {
                descs.push_back(highlights[i].description);
                file_names_wstring.push_back(file_names[i].toStdWString());
                BookState book_state;
                book_state.document_path = file_checksums[i].toStdWString();
                book_state.offset_y = highlights[i].selection_begin.y;
                book_states.push_back(std::make_pair(book_state, highlights[i].uuid));
            }

            set_filtered_select_menu<std::pair<BookState, std::string>>(mw, FUZZY_SEARCHING, MULTILINE_MENUS, { descs, file_names_wstring }, book_states, -1,
                [&, handle_select_fn](std::pair<BookState, std::string>* book_state) {
                    handle_select_fn(book_state->first.offset_y, book_state->second);

                },
                [&, handle_delete_fn](std::pair<BookState, std::string>* book_state) {
                    handle_delete_fn(book_state->second);
                }
            );
            mw->show_current_widget();

        }
    }
}

void AnnotationController::handle_delete_highlight_under_cursor() {
    QPoint mouse_pos = mw->mapFromGlobal(mw->cursor_pos());
    WindowPos window_pos = WindowPos{ mouse_pos.x(), mouse_pos.y() };
    std::string sel_highlight = mdv()->get_highlight_uuid_in_pos(window_pos);
    if (sel_highlight.size() > 0) {
        if (mdv()->get_selected_highlight_uuid() == sel_highlight) {
            mdv()->clear_selected_object();
        }
        delete_current_document_highlight_with_uuid(sel_highlight);
    }
}

std::optional<Highlight> AnnotationController::handle_delete_selected_highlight() {
    std::string selected_highlight_uuid = mdv()->get_selected_highlight_uuid();
    std::optional<Highlight> deleted_highlight = {};
    if (selected_highlight_uuid.size() > 0) {
        mdv()->set_selected_highlight_uuid("");
        deleted_highlight = delete_current_document_highlight_with_uuid(selected_highlight_uuid);
    }
    mw->validate_render();
    mdv()->clear_selected_object();
    return deleted_highlight;
}

std::optional<BookMark> AnnotationController::handle_delete_selected_bookmark() {
    std::string selected_bookmark_uuid = mdv()->get_selected_bookmark_uuid();
    std::optional<BookMark> deleted_bookmark = {};
    if (selected_bookmark_uuid.size() > 0) {
        mdv()->set_selected_bookmark_uuid("");
        deleted_bookmark = mw->delete_current_document_bookmark(selected_bookmark_uuid);
    }
    mdv()->clear_selected_object();
    mw->validate_render();
    return deleted_bookmark;
}

std::optional<Portal> AnnotationController::handle_delete_selected_portal() {
    std::string selected_portal_uuid = mdv()->get_selected_portal_uuid();
    std::optional<Portal> deleted_portal = {};
    if (selected_portal_uuid.size() > 0) {
        mdv()->clear_selected_object();
        deleted_portal = doc()->delete_portal_with_uuid(selected_portal_uuid);
        if (deleted_portal.has_value()) {
            on_portal_deleted(deleted_portal.value(), doc()->get_checksum());
        }
        //push_deleted_portal(deleted_portal);
    }
    mdv()->clear_selected_object();
    mw->validate_render();
    return deleted_portal;
}

void AnnotationController::handle_overview_to_portal() {
    if (mdv()->get_overview_page()) {
        mw->set_overview_page({}, false);
    }
    else {

        OverviewState overview_state;
        std::optional<Portal> portal_ = mdv()->get_target_portal(false);
        if (portal_) {
            Portal portal = portal_.value();
            auto destination_path = mw->checksummer->get_path(portal.dst.document_checksum);
            if (destination_path) {
                Document* doc = mw->document_manager->get_document(destination_path.value());
                if (doc) {
                    doc->open(true);
                    overview_state.absolute_offset_y = portal.dst.book_state.offset_y;
                    overview_state.doc = doc;
                    mw->set_overview_page(overview_state, true);
                }
            }
        }
    }
}

void AnnotationController::handle_special_bookmarks(std::wstring text, std::wstring bookmark_uuid) {

    QString qtext = QString::fromStdWString(text);

    BookMark* bm = doc()->get_bookmark_with_uuid(utf8_encode(bookmark_uuid));

    if (text.size() > 2 && text.substr(0, 2) == L"? ") {
        bm->is_pending = true;
        handle_bookmark_ask_query(text, bookmark_uuid);
    }
    else if (qtext.startsWith("#summarize")) {
        bm->is_pending = true;
        handle_bookmark_summarize_query(bookmark_uuid);
    }
    else if (qtext.startsWith("#shell")) {
        bm->is_pending = true;
        mw->handle_bookmark_shell_command(qtext, utf8_encode(bookmark_uuid));
    }
    else if (QString::fromStdWString(text).startsWith("@")) {
        // the text after the @ and before the first space is the command name
        QString command_name = qtext.mid(1).split(" ").at(0);
        QString text_arg = qtext.mid(1 + command_name.size()).trimmed();
        if (SHELL_BOOKMARK_COMMANDS.find(command_name.toStdWString()) != SHELL_BOOKMARK_COMMANDS.end()) {
            QString command_string = QString::fromStdWString(SHELL_BOOKMARK_COMMANDS[command_name.toStdWString()]);
            QString equivalent_shell_command = "#shell " + command_string;
            mw->handle_bookmark_shell_command(equivalent_shell_command, utf8_encode(bookmark_uuid), text_arg);
        }
    }
}

std::wstring AnnotationController::handle_freetext_bookmark_perform(const std::wstring& text, const std::string& pending_uuid) {
    std::wstring result = L"";
    if (text.size() > 0) {
        std::string uuid = doc()->add_pending_bookmark(pending_uuid, text);
        on_new_bookmark_added(uuid);
        result = utf8_decode(uuid);
        mdv()->set_selected_bookmark_uuid("");
        handle_special_bookmarks(text, utf8_decode(uuid));
    }
    else {
        doc()->undo_pending_bookmark(pending_uuid);
        result = L"";
    }

    mw->clear_selected_rect();
    mw->invalidate_render();
    return result;
}

void AnnotationController::add_portal(std::wstring source_path, Portal new_link) {
    if (source_path == mdv()->get_document()->get_path()) {
        std::string uuid = mdv()->get_document()->add_portal(new_link);
        on_new_portal_added(uuid);
    }
    else if (mw->document_manager->get_cached_document(source_path)){
        // if the source of the portal is not the current document
        // we should add it to the loaded document if exists
        Document* source_doc = mw->document_manager->get_cached_document(source_path).value();
        source_doc->add_portal(new_link);
        on_new_portal_added(new_link.uuid);
    }
    else {
        //const std::unordered_map<std::wstring, Document*> cached_documents = document_manager->get_cached_documents();
        Document* doc = mw->document_manager->get_document(source_path);
        std::string uuid = doc->add_portal(new_link, false);
        on_new_portal_added(uuid);

        if (new_link.is_visible()) {
            std::string uuid = utf8_encode(new_uuid());
            bool success = mw->db_manager->insert_visible_portal(mw->checksummer->get_checksum(source_path),
                new_link.dst.document_checksum,
                new_link.dst.book_state.offset_x,
                new_link.dst.book_state.offset_y,
                new_link.dst.book_state.zoom_level,
                new_link.src_offset_x.value(),
                new_link.src_offset_y,
                utf8_decode(uuid));
            if (success) {
                on_new_portal_added(uuid);
            }
        }
        else {
            std::string uuid = utf8_encode(new_uuid());
            bool success = mw->db_manager->insert_portal(mw->checksummer->get_checksum(source_path),
                new_link.dst.document_checksum,
                new_link.dst.book_state.offset_x,
                new_link.dst.book_state.offset_y,
                new_link.dst.book_state.zoom_level,
                new_link.src_offset_y,
                utf8_decode(uuid));
            if (success) {
                on_new_portal_added(uuid);
            }
        }
    }
}

void AnnotationController::change_bookmark_text(std::string uuid, const std::wstring& new_text) {
    std::string selected_bookmark_uuid = uuid;
    if (selected_bookmark_uuid.size() > 0) {
        BookMark* selected_bookmark = doc()->get_bookmark_with_uuid(selected_bookmark_uuid);
        if (selected_bookmark) {
            if (new_text.size() > 0) {
                float new_font_size = selected_bookmark->font_size;
                doc()->update_bookmark_text(selected_bookmark_uuid, new_text, new_font_size);

                if (new_text[0] == '?') {
                    int last_newline_index = new_text.find_last_of(L"\n");
                    int next_index = last_newline_index + 1;
                    if (next_index < new_text.size() - 1 && new_text[next_index] == '?' && new_text[next_index + 1] == ' ') {
                        handle_special_bookmarks(new_text, utf8_decode(selected_bookmark->uuid));
                    }

                }

                on_bookmark_edited(*selected_bookmark, true, false);
            }
            else {
                delete_current_document_bookmark(selected_bookmark_uuid);
            }
        }
    }
}

void AnnotationController::change_highlight_text_annot(std::string uuid, const std::wstring& new_text) {
    std::string selected_highlight_uuid = uuid;
    if (selected_highlight_uuid.size() > 0) {
        mw->update_highlight_annot_with_uuid(selected_highlight_uuid, new_text);
    }
}

std::optional<BookMark> AnnotationController::delete_current_document_bookmark(const std::string& uuid) {
    std::optional<BookMark> deleted_bookmark = doc()->delete_bookmark_with_uuid(uuid);
    if (deleted_bookmark) {
        on_bookmark_deleted(deleted_bookmark.value(), doc()->get_checksum());
    }
    return deleted_bookmark;
}

void AnnotationController::delete_global_bookmark(const std::string& uuid) {
    std::vector<std::pair<std::string, BookMark>> deleted_bookmark;
    mw->db_manager->select_bookmark_with_uuid(uuid, deleted_bookmark);
    if (deleted_bookmark.size() > 0) {
        if (deleted_bookmark[0].first == doc()->get_checksum()) {
            doc()->delete_bookmark_with_uuid(uuid);
        }
        else {
            mw->db_manager->delete_bookmark(uuid);
        }
        on_bookmark_deleted(deleted_bookmark[0].second, deleted_bookmark[0].first);
    }
}

void AnnotationController::update_highlight_annot_with_uuid(const std::string& uuid, const std::wstring& new_annot) {
    doc()->update_highlight_add_text_annotation(uuid, new_annot);
    on_highlight_annotation_edited(uuid);
}


void AnnotationController::handle_portal() {
    if (!mw->main_document_view_has_document()) return;

    if (mdv()->is_pending_link_source_filled()) {
        auto [source_path, pl] = mdv()->current_pending_portal.value();
        pl.dst = mdv()->get_checksum_state();

        if (source_path.has_value()) {
            add_portal(source_path.value(), pl);
        }

        mdv()->set_pending_portal({});
    }
    else {
        // if an overview is opened, add the overview as a pinned portal
        if (dv()->get_overview_page().has_value()){
            pin_current_overview_as_portal();
            mw->close_overview();
        }
        else{
            mdv()->set_pending_portal(mdv()->get_document()->get_path(),
                Portal::with_src_offset(mdv()->get_offset_y()));
        }
    }

    mw->synchronize_pending_link();
    mw->refresh_all_windows();
    mw->validate_render();
}

void AnnotationController::start_creating_rect_portal(AbsoluteDocumentPos location) {

    Portal new_portal;
    new_portal.src_offset_y = location.y;
    new_portal.src_offset_x = location.x;

    mdv()->set_pending_portal(mdv()->get_document()->get_path(), new_portal);

    mw->synchronize_pending_link();
    mw->refresh_all_windows();
    mw->validate_render();
}

bool AnnotationController::handle_annotation_move_finish(){

    if (mdv()->visible_object_move_data && mdv()->visible_object_move_data->is_moving) {
        mdv()->visible_object_move_data->handle_move_end(mw);
        mdv()->visible_object_move_data = {};
        mw->stop_dragging();
        mdv()->is_selecting = false;
        return true;
    }

    if (mdv()->freehand_drawing_move_data) {
        mdv()->handle_freehand_drawing_move_finish(mw->get_cursor_abspos());
        mw->invalidate_render();
        mdv()->is_selecting = false;
        return true;
    }

    return false;
}

void AnnotationController::update_bookmark_with_uuid(const std::string& uuid) {
    if (uuid.size() > 0) {
        //BookMark& bm = doc()->get_bookmarks()[index];
        BookMark* bm = doc()->get_bookmark_with_uuid(uuid);
        if (bm) {
            doc()->update_bookmark_position(uuid, { bm->begin_x, bm->begin_y }, { bm->end_x, bm->end_y });
            on_bookmark_edited(*bm, true, true);
        }
    }
}

void AnnotationController::handle_bookmark_move_finish() {
    std::string uuid = mdv()->visible_object_move_data->index.uuid;
    update_bookmark_with_uuid(uuid);
    mdv()->handle_bookmark_move_finish();
}

void AnnotationController::update_portal_with_uuid(const std::string& uuid) {
    if (uuid.size() > 0) {
        //Portal& portal = doc()->get_portals()[index];
        Portal* portal = doc()->get_portal_with_uuid(uuid);

        if (portal) {
            if (portal->is_pinned()) {
                doc()->update_portal_src_position(uuid,
                    AbsoluteDocumentPos{ portal->src_offset_x.value(), portal->src_offset_y },
                    AbsoluteDocumentPos{ portal->src_offset_end_x.value(), portal->src_offset_end_y.value() }
                );
            }
            else {
                if (portal->src_offset_end_x.has_value()) {
                    doc()->update_portal_src_position(uuid,
                        AbsoluteDocumentPos{ portal->src_offset_x.value(), portal->src_offset_y },
                        {}
                    );
                }
            }
            //doc()->update_portal(portal);
            on_portal_edited(uuid);
        }
    }
}

void AnnotationController::handle_visible_object_move() {
    if (mdv()->visible_object_move_data.has_value()) {
        auto type = mdv()->visible_object_move_data->index.object_type;
        if (type == VisibleObjectType::Bookmark) {
            handle_bookmark_move();
        }
        else if (type == VisibleObjectType::Portal || type == VisibleObjectType::PendingPortal) {
            handle_portal_move();
        }
    }
}

void AnnotationController::handle_bookmark_move() {
    AbsoluteDocumentPos current_mouse_abspos = mw->get_cursor_abspos();
    mdv()->handle_bookmark_move(current_mouse_abspos);
}

void AnnotationController::handle_portal_move(){
    mdv()->handle_portal_move(mw->get_cursor_abspos());
}

void AnnotationController::handle_portal_move_finish() {
    mdv()->handle_portal_move_finish();
}

bool AnnotationController::handle_visible_object_resize_finish() {
    auto& visible_object_resize_data = mdv()->visible_object_resize_data;

    if (visible_object_resize_data->type == VisibleObjectType::Bookmark) {
        update_bookmark_with_uuid(visible_object_resize_data->object_uuid);
        visible_object_resize_data = {};
        return true;
    }
    if (visible_object_resize_data->type == VisibleObjectType::PinnedPortal) {
        update_portal_with_uuid(visible_object_resize_data->object_uuid);
        visible_object_resize_data = {};
        return true;
    }
    return false;
}

void AnnotationController::set_mark_in_current_location(char symbol) {
    // it is a global mark, we delete other marks with the same symbol from database and add the new mark
    std::string uuid;
    if (isupper(symbol)) {
        mw->db_manager->delete_mark_with_symbol(symbol);
        // we should also delete the cached marks
        mw->document_manager->delete_global_mark(symbol);
        uuid = mdv()->add_mark(symbol);
    }
    else {
        uuid = mdv()->add_mark(symbol);
        mw->validate_render();
    }
    on_mark_added(uuid, symbol);
}

void AnnotationController::goto_mark(char symbol) {
    if (symbol == '`' || symbol == '\'') {
        mw->return_to_last_visual_mark();
    }
    else if (isupper(symbol)) { // global mark
        std::vector<std::pair<std::string, float>> mark_vector;
        mw->db_manager->select_global_mark(symbol, mark_vector);
        if (mark_vector.size() > 0) {
            assert(mark_vector.size() == 1); // we can not have more than one global mark with the same name
            std::wstring doc_path = mw->checksummer->get_path(mark_vector[0].first).value();
            mw->open_document(doc_path, 0.0f, mark_vector[0].second);
        }

    }
    else {
        mdv()->goto_mark(symbol);
    }
}

void AnnotationController::handle_portal_to_overview() {
    std::optional<Portal> new_portal = mdv()->create_portal_to_overview();
    if (new_portal.has_value()) {
        add_portal(mdv()->get_document()->get_path(), new_portal.value());
    }
}
