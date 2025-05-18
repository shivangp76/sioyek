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
