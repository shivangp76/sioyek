#include <QTimer>

#include "main_widget.h"
#include "network_manager.h"
#include "controllers/annotation_controller.h"
#include "ui/bookmark_ui.h"

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
