#pragma once

#include <QString>
#include <string>

#include "controllers/base_controller.h"
#include "types/common_types.h"
#include "book.h"

class SioyekNetworkManager;
class QNetworkReply;
class QPixmap;

class NetworkController : public BaseController{
    int next_pending_drawing_request_id = 0;
public:
    SioyekNetworkManager* sioyek_network_manager = nullptr;
    NetworkController(MainWidget* parent, SioyekNetworkManager* manager);
    void handle_resume_to_server_location();
    void handle_server_actions_button_pressed();
    bool is_current_document_available_on_server();
    bool is_network_manager_running(bool* is_downloading = nullptr, std::wstring* message=nullptr);
    void sync_annotations_with_server();
    void cleanup_expired_pending_portals();
    QString get_network_status_string();
    QByteArray perform_network_request(QString url, QString method = "get", QString json_data = "");
    void handle_sync_open_document();
    void handle_server_document_location_mismatch(float local_offset_y, float server_offset_y);
    QString get_login_status_string();
    bool is_logged_in();
    ServerStatus get_status();
    bool try_download_file_with_hash(QString hash, std::function<void(QString)>&& fn);
    void upload_drawings(bool wait_for_send = false);
    void perform_sync_operations_when_document_is_closed(bool wait_for_send, bool sync_drawings);
    void sync_current_file_location_to_servers(bool wait_for_send=false);
    void sync_document_location_to_servers(Document* doc, float offset_y, bool wait_for_send=false);
    QNetworkReply* download_paper_with_url(std::wstring paper_url, bool use_archive_url, PaperDownloadFinishedAction action);
    void on_paper_downloaded(QNetworkReply* reply);
    void handle_login(std::wstring email, std::wstring password);
    void upload_current_file(Document* document_to_upload=nullptr);
    void auto_login();
    void download_and_open(std::string checksum, float offset_y);
    void delete_current_file_from_server();
    void do_synchronize();
    void synchronize_if_desynchronized();
    void preload_next_page_for_tts(float rate);
    QNetworkReply* download_paper_with_name(std::wstring name, QString full_bib_text, std::optional<PaperDownloadFinishedAction> action = {}, std::string pending_portal_handle="");
    PaperDownloadFinishedAction get_default_paper_download_finish_action();
    void on_paper_download_begin(QNetworkReply* reply, std::string pending_portal_handle);
    void show_citers_with_paper_name(std::wstring paper_name);
    void show_citers_of_current_paper();
    QString perform_network_request_with_headers(QString method, QString url, QJsonObject headers, QJsonObject request, bool* is_done=nullptr, QByteArray* response=nullptr);
    // void download_file_with_hash(QObject* parent, QString hash, std::function<void(QString)> fn);
    void semantic_ask_with_image(
        QObject * parent,
        const QPixmap& pixmap,
        std::function<void(QString)>&& on_done
    );
    QJsonObject get_current_user();
    void sync_deleted_annot(const std::string& annot_type, const std::string& uuid);
    void sync_edited_annot(const std::string& annot_type, const std::string& uuid);
    void handle_open_server_only_file();
    void manage_last_document_checksum();
    std::vector<OpenedBookInfo> get_excluded_opened_files(std::vector<std::string>& excluded_checksums);
    void delete_file_from_server(QObject* parent, std::string checksum, std::function<void()> on_done);
};
