#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QThread>

#include "network_manager.h"
#include "controllers/network_controller.h"
#include "main_widget.h"
#include "document_view.h"
#include "document.h"
#include "checksum.h"
#include "ui.h"
#include "path.h"
#include "utils.h"
#include "database.h"

extern int DOCUMENT_LOCATION_MISMATCH_STRATEGY;
extern float SERVER_AND_LOCAL_DOCUMENT_MISMATCH_THRESHOLD;
extern bool AUTO_RENAME_DOWNLOADED_PAPERS;
extern Path downloaded_papers_path;
extern bool AUTO_LOGIN_ON_STARTUP;
extern bool PAPER_DOWNLOAD_CREATE_PORTAL;


NetworkController::NetworkController(MainWidget* parent, SioyekNetworkManager* manager) : BaseController(parent), sioyek_network_manager(manager){
}

void NetworkController::handle_resume_to_server_location() {
    if (doc() && doc()->get_checksum_fast()) {
        auto res = sioyek_network_manager->last_server_location.find(doc()->get_checksum_fast().value());
        if (res != sioyek_network_manager->last_server_location.end()) {
            mw->long_jump_to_destination(res->second);
            mw->resume_to_server_position_button->hide();
            mw->invalidate_render();
        }
    }
}

void NetworkController::handle_server_actions_button_pressed() {
    if (sioyek_network_manager->status == ServerStatus::LoggedIn) {
        if (!is_current_document_available_on_server()) {
            mw->show_context_menu("logout|upload_current_file");
        }
        else {
            mw->show_context_menu("logout");
        }
    }
    else if ((sioyek_network_manager->status == ServerStatus::NotLoggedIn) || (sioyek_network_manager->status == ServerStatus::InvalidCredentials)) {
        mw->show_context_menu("login");
    }

}

bool NetworkController::is_current_document_available_on_server() {
    return sioyek_network_manager->is_document_available_on_server(doc());
}

bool NetworkController::is_network_manager_running(bool* is_downloading, std::wstring* message) {
    if (sioyek_network_manager->network_manager_ == nullptr) {
        return false;
    }

    auto children = sioyek_network_manager->network_manager_->findChildren<QNetworkReply*>();
    bool running = false;

    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isRunning()) {
            running = true;
            if (is_downloading) {
                bool downloading = !children.at(i)->property("sioyek_downloading").isNull();

                if (!children.at(i)->property("sioyek_network_message").isNull()) {
                    *message = children.at(i)->property("sioyek_network_message").toString().toStdWString();
                }

                *is_downloading = downloading;
                return running;
            }
        }
    }
    return running;
}

void NetworkController::sync_annotations_with_server() {
    sioyek_network_manager->sync_document_annotations_to_server(mw, doc(), [this]() {mw->invalidate_render(); });
}

void NetworkController::cleanup_expired_pending_portals() {
    std::vector<int> indices_to_delete;

    if ((mdv()->pending_download_portals.size() > 0) && (mw->current_widget_stack.size() == 0)) {
        if (sioyek_network_manager->network_manager_ == nullptr) {
            return;
        }

        auto children_ = mw->findChildren<QNetworkReply*>() + sioyek_network_manager->network_manager_->findChildren<QNetworkReply*>();

        for (int i = 0; i < mdv()->pending_download_portals.size(); i++) {
            auto paper_name = mdv()->pending_download_portals[i].paper_name;
            bool still_pending = false;
            //network_manager.
            for (int i = 0; i < children_.size(); i++) {
                auto paper_name_prop = children_[i]->property("sioyek_paper_name");
                if ((!paper_name_prop.isNull()) && paper_name_prop.toString().toStdWString() == paper_name) {
                    still_pending = true;
                }
            }
            if (!still_pending) {
                if (mdv()->pending_download_portals[i].marked) {
                    indices_to_delete.push_back(i);
                }
                else {
                    mdv()->pending_download_portals[i].marked = true;
                }
            }
        }
    }
    if (indices_to_delete.size() > 0) {
        //update_pending_portal_indices_after_removed_indices(indices_to_delete);
        for (int i = indices_to_delete.size() - 1; i >= 0; i--) {
            mdv()->pending_download_portals.erase(mdv()->pending_download_portals.begin() + indices_to_delete[i]);
        }
    }

}

QString NetworkController::get_network_status_string() {
    auto children = mw->findChildren<QNetworkReply*>();
    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isRunning()) {
            QVariant status_message = children.at(i)->property("sioyek_network_status_string");
            if (!status_message.isNull()) {
                return status_message.toString();
            }
        }
    }
    return "";
}

QByteArray NetworkController::perform_network_request(QString url, QString method, QString json_data){
    bool is_done = false;
    QByteArray res;

    QMetaObject::invokeMethod(mw, [&, url]() {
            QNetworkRequest req;
            req.setUrl(url);
            if (json_data.size() > 0) {
                req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            }
            QNetworkReply* reply = nullptr;
            if (method == "get") {
                reply = sioyek_network_manager->get_network_manager()->get(req);
            }
            else if (method == "post") {
                reply = sioyek_network_manager->get_network_manager()->post(req, json_data.toUtf8());
            }

            if (reply) {
                reply->setProperty("sioyek_js_extension", "true");
                QObject::connect(reply, &QNetworkReply::finished, [&, reply]() {
                    //res = QString::fromUtf8(reply->readAll());
                    reply->deleteLater();
                    res = reply->readAll();
                    is_done = true;
                    });
            }
        });
    while (!is_done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };
    return res;
}

void NetworkController::handle_sync_open_document() {
    if (sioyek_network_manager->ACCESS_TOKEN.size() > 0) {
        // check if the server's document location is different from the local location
        if (doc() && doc()->get_checksum_fast()) {
            sioyek_network_manager->get_opened_book_data_from_checksum(mw, QString::fromStdString(doc()->get_checksum_fast().value()), [&, document=doc()](QJsonObject obj) {
                if (document != doc()) return;

                if (obj["status"] == "OK") {
                    float server_offset_y = obj["result"].toObject()["offset_y"].toDouble();
                    if (std::abs(server_offset_y - mdv()->get_offset_y()) > SERVER_AND_LOCAL_DOCUMENT_MISMATCH_THRESHOLD) {
                        handle_server_document_location_mismatch(mdv()->get_offset_y(), server_offset_y);
                    }
                }
                else if (obj["status"] == "DELETED") {
                    doc()->set_is_synced(false);
                }

                });
        }
    }
}

void NetworkController::handle_server_document_location_mismatch(float local_offset_y, float server_offset_y) {
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::Local) return;
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::Server) {
        mw->long_jump_to_destination(server_offset_y);
    }
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::Ask) {
        int res = show_option_buttons(L"Do you want to move to server location?", { L"Yes", L"No" });
        if (res == 0) {
            mw->long_jump_to_destination(server_offset_y);
        }
    }
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::ShowButton) {
        if (mdv()) {
            if (doc() && doc()->get_checksum_fast()) {
                sioyek_network_manager->set_last_server_location(doc()->get_checksum_fast().value(), server_offset_y);
                mw->resume_to_server_position_button->show();
            }
        }
    }

}

QString NetworkController::get_login_status_string(){
    return sioyek_network_manager->get_login_status_string(doc());
}

bool NetworkController::is_logged_in(){
    return sioyek_network_manager->status == ServerStatus::LoggedIn;
}

ServerStatus NetworkController::get_status(){
    return sioyek_network_manager->status;
}

bool NetworkController::try_download_file_with_hash(QString hash, std::function<void(QString)>&& fn){
    if (sioyek_network_manager->is_checksum_available_on_server(hash.toStdString())){
        sioyek_network_manager->download_file_with_hash(mw, hash, [this, fn=std::move(fn)](QString path) {
            fn(path);
        });
        return true;
    }
    return false;
}

void NetworkController::upload_drawings(bool wait_for_send) {
    std::optional<std::string> checksum = doc()->get_checksum_fast();
    if (checksum) {
        std::wstring file_path = doc()->get_drawings_file_path();
        QMap<int, PageFreehandDrawing> drawings = doc()->page_freehand_drawings;

        QNetworkReply* reply = sioyek_network_manager->upload_drawings(mw, checksum.value(), file_path, [file_path, drawings]() mutable {
            for (int page : drawings.keys()){
                for (auto& drawing : drawings[page].drawings){
                    drawing.is_synced = true;
                }
            }
            QFile output_file(QString::fromStdWString(file_path));
            if (output_file.open(QIODeviceBase::WriteOnly)){
                save_drawings_to_file(output_file, drawings);
                output_file.close();
            }
            else{
                qDebug() << "could not open the drawings file";
            }

            });
        if (reply && wait_for_send) {
            block_for_send(reply);
        }
    }
}


void NetworkController::perform_sync_operations_when_document_is_closed(bool wait_for_send, bool sync_drawings) {
    if (is_logged_in() && doc() && doc()->get_is_synced()) {
        sync_current_file_location_to_servers(wait_for_send);
        if (sync_drawings) {
            upload_drawings(wait_for_send);
        }
    }
}

void NetworkController::sync_current_file_location_to_servers(bool wait_for_send) {
    return sync_document_location_to_servers(doc(), mdv()->get_offset_y(), wait_for_send);
}

void NetworkController::sync_document_location_to_servers(Document* document, float offset_y, bool wait_for_send) {
    QDateTime current_datetime = QDateTime::currentDateTime();

    if (document && document->get_checksum_fast()) {
        QNetworkReply* reply = sioyek_network_manager->sync_file_location(
            QString::fromStdString(document->get_checksum_fast().value()),
            QString::fromStdWString(document->detect_paper_name()),
            current_datetime.toString(Qt::DateFormat::ISODate),
            offset_y
        );

        if (wait_for_send) {
            block_for_send(reply);
        }
    }
}

QNetworkReply* NetworkController::download_paper_with_url(std::wstring paper_url, bool use_archive_url, PaperDownloadFinishedAction action) {
    QNetworkReply* reply = sioyek_network_manager->download_paper_with_url(paper_url, use_archive_url, action);
    QObject::connect(reply, &QNetworkReply::finished, mw, [this, reply]() {
        on_paper_downloaded(reply);
        });
    return reply;
}

void NetworkController::on_paper_downloaded(QNetworkReply* reply) {
    QByteArray pdf_data = reply->readAll();
    QString header = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    if ((pdf_data.size() == 0) || (!header.startsWith("application/pdf"))) {
        return;
    }

    QString file_name = reply->url().fileName();
    if (!file_name.endsWith(".pdf") && header.startsWith("application/pdf")) {
        file_name = file_name + ".pdf";
    }

    if (AUTO_RENAME_DOWNLOADED_PAPERS && (!reply->property("sioyek_actual_paper_name").isNull())) {
        QString detected_file_name = get_file_name_from_paper_name(reply->property("sioyek_actual_paper_name").toString());

        if (detected_file_name.size() > 0) {
            QString extension = file_name.split(".").back();
            file_name = detected_file_name + "." + extension;
        }
    }
    //reply->property("sioyek_paper_name").toString().replace("/", "_")

    PaperDownloadFinishedAction finish_action = get_paper_download_action_from_string(reply->property("sioyek_finish_action").toString());
    QString path = QString::fromStdWString(downloaded_papers_path.slash(file_name.toStdWString()).get_path());
    QDir dir;
    dir.mkpath(QString::fromStdWString(downloaded_papers_path.get_path()));

    QFile file(path);
    bool opened = file.open(QIODeviceBase::WriteOnly);
    if (opened) {
        file.write(pdf_data);
        file.close();
        if (finish_action == PaperDownloadFinishedAction::Portal) {
            //std::string checksum = this->checksummer->get_checksum(path.toStdWString());

            std::string new_portal_uuid = mdv()->finish_pending_download_portal(
                reply->property("sioyek_paper_name").toString().toStdWString(),
                path.toStdWString()
            );

            if (new_portal_uuid.size() > 0){
                mw->on_new_portal_added(new_portal_uuid);
            }

        }
        else {
#ifdef SIOYEK_MOBILE
            // todo: maybe show a dialog asking the user if they want to open the downloaded document
            mw->push_state();
            mw->open_document(path.toStdWString());
#else
            MainWidget* new_window = mw->handle_new_window();
            new_window->open_document(path.toStdWString());
#endif
        }
    }

}

void NetworkController::handle_login(std::wstring email, std::wstring password) {
    sioyek_network_manager->login(email, password);
}

void NetworkController::upload_current_file(Document* document_to_upload) {
    if (!doc()) return;

    if (document_to_upload == nullptr){
        document_to_upload = doc();
    }

    float offset_y = mdv()->get_offset_y();

    QString status_id = QString::fromStdWString(new_uuid());

    sioyek_network_manager->upload_file(
        mw,
        QString::fromStdWString(document_to_upload->get_path()),
        QString::fromStdString(document_to_upload->get_checksum()),
        [&, offset_y, status_id,document=document_to_upload]() {
            document->set_is_synced(true);
            sioyek_network_manager->sync_document_annotations_to_server(mw, document, [this]() {mw->invalidate_render(); });
            //sync_annotations_with_server();
            sync_document_location_to_servers(document, offset_y, false);
            mw->set_status_message(L"", status_id);
        },
        [&, document=document_to_upload](){
            std::string old_checksum = document->get_checksum();
            std::string new_checksum = compute_checksum(QString::fromStdWString(document->get_path()), QCryptographicHash::Md5);
            if (old_checksum != new_checksum){
                mw->document_manager->update_checksum(old_checksum, new_checksum);
                upload_current_file(document);
            }
        },
        [&, status_id](int uploaded, int total) {
            QString uploaded_human_readable = file_size_to_human_readable_string(uploaded);
            QString total_human_readable = file_size_to_human_readable_string(total);
            mw->set_status_message(("Uploading " + uploaded_human_readable + " / " + total_human_readable).toStdWString(), status_id);
        }
    );
}

void NetworkController::auto_login() {
    if (AUTO_LOGIN_ON_STARTUP) {
        sioyek_network_manager->load_access_token();
    }
}

void NetworkController::download_and_open(std::string checksum, float offset_y) {

    sioyek_network_manager->download_file_with_hash(mw, QString::fromStdString(checksum), [&, checksum, offset_y](QString file_path) {
        mw->checksummer->set_checksum(checksum, file_path.toStdWString());

        std::optional<float> offset_x = {};
        mw->push_state();
        mw->open_document(file_path.toStdWString(), offset_x, offset_y, {}, checksum);
        //doc()->annotations_are_freshly_loaded = true;
        mw->invalidate_render();
        });
}

void NetworkController::delete_current_file_from_server() {
    if (doc() && doc()->get_checksum_fast()) {
        doc()->set_is_synced(false);
        sioyek_network_manager->delete_file_with_checksum(QString::fromStdString(doc()->get_checksum_fast().value()));
    }
}

void NetworkController::do_synchronize() {
    doc()->set_is_synced(true);
    handle_sync_open_document();
    sync_annotations_with_server();
}

void NetworkController::synchronize_if_desynchronized() {
    if (doc()) {
        if (is_current_document_available_on_server()) {
            if (!doc()->get_is_synced()) {
                do_synchronize();
            }
        }
    }
}

void NetworkController::preload_next_page_for_tts(float rate) {
    int next_page_number = mw->get_current_page_number() + 1;
    if (next_page_number < doc()->num_pages()) {
        std::vector<PagelessDocumentRect> dummy_next_lines;
        std::vector<PagelessDocumentRect> dummy_next_chars;
        std::wstring next_page_text;
        doc()->get_page_text_and_line_rects_after_rect(next_page_number, INT_MAX, fz_empty_rect, next_page_text, dummy_next_lines, dummy_next_chars);
        sioyek_network_manager->tts(mw,
            next_page_text,
            doc()->get_checksum(),
            next_page_number,
            rate,
            [](QString path, std::vector<float> timestamps) {},
            [](QString checksum) {}
        );
    }
}

void NetworkController::on_paper_download_begin(QNetworkReply* reply, std::string pending_portal_handle) {

    QObject::connect(reply, &QNetworkReply::downloadProgress, [this, pending_portal_handle](qint64 received, qint64 total) {
        int pending_index = mdv()->get_pending_portal_index_with_handle(pending_portal_handle);
        float ratio = static_cast<float>(received) / total;
        mdv()->pending_download_portals[pending_index].downloaded_fraction = ratio;
        mw->invalidate_render();
        });
}

QNetworkReply* NetworkController::download_paper_with_name(std::wstring name, QString full_bib_text, std::optional<PaperDownloadFinishedAction> action, std::string pending_portal_handle) {
    QNetworkReply* reply = sioyek_network_manager->download_paper_with_name(mw, name, full_bib_text,
        action.value_or(get_default_paper_download_finish_action()),
        [this, pending_portal_handle](QNetworkReply* reply) {
            on_paper_download_begin(reply, pending_portal_handle);
        },
        [this](QNetworkReply* reply) {
            on_paper_downloaded(reply);
        });
    return reply;
}

PaperDownloadFinishedAction NetworkController::get_default_paper_download_finish_action() {
    if (PAPER_DOWNLOAD_CREATE_PORTAL) {
        return PaperDownloadFinishedAction::Portal;
    }

    return PaperDownloadFinishedAction::OpenInNewWindow;
}

void NetworkController::show_citers_with_paper_name(std::wstring paper_name) {
    sioyek_network_manager->get_citers_with_name(mw, paper_name, [this](std::vector<QString> citer_urls, std::vector<QString> citer_titles, std::vector<QString> citer_descriptions) {

        auto selector_widget = ItemWithDescriptionSelectorWidget::from_items(std::move(citer_titles), std::move(citer_descriptions), std::move(citer_urls), mw);
        selector_widget->set_select_fn([this, selector_widget](int index) {
            QString url = selector_widget->item_model->metadatas[index];
            auto reply = download_paper_with_url(url.toStdWString(), false, PaperDownloadFinishedAction::OpenInNewWindow);
            reply->setProperty("sioyek_downloading", true);
            reply->setProperty("sioyek_network_message", "starting download");
            QObject::connect(reply, &QNetworkReply::downloadProgress, mw, [this, reply](qint64 received, qint64 total) {
                if (total > 0) {
                    float ratio = static_cast<float>(received) / total;
                    int percent = static_cast<int>(ratio * 100);
                    reply->setProperty("sioyek_network_message", "downloading ... " + QString::number(percent) + "% (cancel)");
                }
                else {
                    reply->setProperty("sioyek_network_message", "downloading ... " + QString::number(received) + "B (cancel)");

                }
                //main_document_view->pending_download_portals[pending_index].downloaded_fraction = ratio;
                mw->invalidate_ui();
                });
            mw->pop_current_widget();
            mw->invalidate_ui();

            });
        mw->set_current_widget(selector_widget);
        mw->show_current_widget();

        });

}

void NetworkController::show_citers_of_current_paper() {
    std::wstring paper_name = doc()->detect_paper_name();
    show_citers_with_paper_name(paper_name);
}

QString NetworkController::perform_network_request_with_headers(QString method, QString url, QJsonObject headers, QJsonObject request, bool* is_done, QByteArray* response){
    bool is_on_main_thread = QThread::currentThread() == QApplication::instance()->thread();
    if (is_on_main_thread) {

        QNetworkRequest req;

        for (auto header : headers.keys()) {
            QString header_name = header;
            QString header_value = headers[header].toString();
            req.setRawHeader(header.toUtf8(), headers[header].toString().toUtf8());
        }

        req.setUrl(QUrl(url));
        QString req_content = QJsonDocument(request).toJson();
        qDebug() << req_content;
        QNetworkReply* reply = nullptr;

        if (method.toLower() == "post") {
            reply = sioyek_network_manager->get_network_manager()->post(req, QJsonDocument(request).toJson());
        }
        else {
            reply = sioyek_network_manager->get_network_manager()->get(req, QJsonDocument(request).toJson());
        }

        QObject::connect(reply, &QNetworkReply::readyRead, [reply, is_done, response]() {
            auto content = reply->readAll();
            if (is_done) {
                *is_done = true;
            }
            if (response) {
                *response = content;
            }
            });

        QObject::connect(reply, &QNetworkReply::finished, [reply, is_done]() {
            if (is_done) {
                *is_done = true;
            }
            reply->deleteLater();
            });
        return "";

    }
    else {

        bool done = false;
        QByteArray resp;
        QMetaObject::invokeMethod(mw,
            "perform_network_request_with_headers",
            Qt::BlockingQueuedConnection,
            Q_ARG(QString, method),
            Q_ARG(QString, url),
            Q_ARG(QJsonObject, headers),
            Q_ARG(QJsonObject, request),
            Q_ARG(bool*, &done),
            Q_ARG(QByteArray*, &resp)
        );

        while (!done) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return QString::fromUtf8(resp);

    }


}

void NetworkController::semantic_ask_with_image(
    QObject * parent,
    const QPixmap& pixmap,
    std::function<void(QString)>&& on_done
    ){
    return sioyek_network_manager->semantic_ask_with_image(parent, pixmap, std::move(on_done));
}

QJsonObject NetworkController::get_current_user(){
    if (sioyek_network_manager){
        return sioyek_network_manager->current_user;
    }
    return QJsonObject();
}

void NetworkController::sync_deleted_annot(const std::string& annot_type, const std::string& uuid) {
    sioyek_network_manager->sync_deleted_annot(mw, doc(), annot_type, uuid);
}

void NetworkController::handle_open_server_only_file() {

    std::vector<std::string> local_checksums;
    mw->db_manager->get_all_local_checksums(local_checksums);

    std::vector<std::vector<std::wstring>> values;
    std::vector<OpenedBookInfo> keys;

    values.push_back({});

    for (auto&  opened_file_info : sioyek_network_manager->get_excluded_opened_files(local_checksums)) {
        //values.push_back({});
        values.back().push_back(opened_file_info.file_name.toStdWString());
        keys.push_back(opened_file_info);
    }

    set_filtered_select_menu<OpenedBookInfo>(mw->widget_controller.get(), false, true, values, keys, -1, [this](OpenedBookInfo* val) {

        download_and_open(val->checksum, val->offset_y);
        },
        [](OpenedBookInfo* val) {

        });
    mw->show_current_widget();
}

void NetworkController::manage_last_document_checksum() {
    if (doc()) {
        if (doc()->checksum_is_new && sioyek_network_manager->server_hashes_loaded) {
            doc()->checksum_is_new = false;
            mw->on_checksum_computed();
        }
    }
}

std::vector<OpenedBookInfo> NetworkController::get_excluded_opened_files(std::vector<std::string>& excluded_checksums){
    return sioyek_network_manager->get_excluded_opened_files(excluded_checksums);
}

void NetworkController::delete_file_from_server(QObject* parent, std::string checksum, std::function<void()> on_done){
    return sioyek_network_manager->delete_file_from_server(parent, checksum, std::move(on_done));
}

