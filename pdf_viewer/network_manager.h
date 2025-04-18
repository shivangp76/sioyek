#pragma once

#include <unordered_set>

#include <qobject.h>
#include <qnetworkaccessmanager.h>

#include "book.h"

class DatabaseManager;
class BackgroundTaskManager;
class DocumentManager;
class Document;


class SioyekNetworkManager{
private:
    DatabaseManager* db_manager = nullptr;
    BackgroundTaskManager* background_task_manager = nullptr;
    DocumentManager* document_manager = nullptr;
    bool already_downloaded_new_annotations = false;
    std::optional<QDateTime> last_server_sync_time;
    std::optional<QJsonObject> sioyek_json_data = {};
public:
    QNetworkAccessManager* network_manager_ = nullptr;
    QJsonObject current_user;
    std::string ACCESS_TOKEN;
    //const std::wstring SIOYEK_HOST = L"http://127.0.0.1:8081/";
    const std::wstring SIOYEK_TOKEN_URL_ = L"token";
    const std::wstring SIOYEK_PAPER_URL_URL_ = L"get_paper_url";
    const std::wstring SIOYEK_PAPER_CITERS_URL_ = L"get_paper_citers";
    const std::wstring SIOYEK_ECHO_URL_ = L"echo_user";
    const std::wstring SIOYEK_UPLOAD_URL_ = L"upload_file";
    const std::wstring SIOYEK_UPDATE_CHECKSUM_URL_ = L"checksum_updated";
    const std::wstring SIOYEK_USER_FILE_HASH_SET_URL_ = L"user_hash_set";
    const std::wstring SIOYEK_DOWNLOAD_FILE_WITH_HASH_PATH_ = L"download_hash";
    const std::wstring SIOYEK_SYNC_OPENED_BOOK_URL_ = L"sync_opened_book";
    const std::wstring SIOYEK_GET_OPENED_BOOK_DATA_URL_ = L"get_opened_book";
    const std::wstring SIOYEK_GET_OPENED_BOOKS_DATA_URL_ = L"get_opened_books";
    const std::wstring SIOYEK_GET_DOCUMENT_ANNOTATIONS_URL_ = L"get_annotations_for_document";
    const std::wstring SIOYEK_SYNC_DELETES_URL_ = L"sync_deletes";
    const std::wstring SIOYEK_ADD_HIGHLIGHT_URL_ = L"add_highlight";
    const std::wstring SIOYEK_DELETE_ANNOT_URL_ = L"delete_annot";
    const std::wstring SIOYEK_ADD_BOOKMARK_URL_ = L"add_bookmark";
    const std::wstring SIOYEK_ADD_PORTAL_URL_ = L"add_portal";
    const std::wstring SIOYEK_DELETE_FILE_URL_ = L"delete_file";
    const std::wstring SIOYEK_TTS_URL_ = L"tts";
    //const std::wstring SIOYEK_GOOGLE_TTS_URL_ = L"google_tts";
    const std::wstring SIOYEK_DEBUG_URL_ = L"debug";
    const std::wstring SIOYEK_SEMANTIC_SEARCH_URL_ = L"semantic_search";
    const std::wstring SIOYEK_SEMANTIC_ASK_GEMINI_URL_ = L"semantic_ask_gemini";
    const std::wstring SIOYEK_SEMANTIC_ASK_URL_ = L"semantic_ask";
    const std::wstring SIOYEK_SUMMARIZE_URL_ = L"summarize";
    const std::wstring SIOYEK_UPLOAD_INDEX_URL_ = L"upload_index";
    const std::wstring SIOYEK_STREAM_TEST_URL_ = L"stream";
    const std::wstring SIOYEK_EXTRACT_TABLE_URL_ = L"extract_table";
    const std::wstring SIOYEK_GENERIC_LLM_URL_ = L"generic_llm";
    //const std::wstring SIOYEK_CONVERT_TO_LATEX_URL_ = L"convert_to_latex";
    const std::wstring SIOYEK_DELETE_FILE_CHECKSUM_URL_ = L"delete_checksum";
    const std::wstring SIOYEK_UPLOAD_DRAWINGS_URL_ = L"upload_drawings";
    const std::wstring SIOYEK_DOWNLOAD_DRAWINGS_URL_ = L"download_drawings";
    const std::wstring SIOYEK_GET_LAST_DRAWING_MODIFICATION_TIME_URL_ = L"last_drawing_modification_time";
    const std::wstring SIOYEK_GET_NEW_ANNOTATIONS_URL_ = L"get_annotations_after";
    const std::wstring SIOYEK_DOES_INDEX_EXIST_URL_ = L"does_index_exist";
    const std::wstring SIOYEK_API_SEARCH_URL_ = L"api/search";
    const std::wstring SIOYEK_DASHBOARD_URL_ = L"app/dashboard";
    const std::wstring SIOYEK_OCR_URL_ = L"ocr";
    const std::wstring SIOYEK_SEMANTIC_ASK_WITH_IMAGE_URL_ = L"semantic_ask_with_image";

    bool server_hashes_loaded = false;
    std::unordered_set<std::string> SERVER_HASHES = {};
    std::unordered_set<std::string> SERVER_DELETED_FILES = {};
    std::unordered_map<std::string, OpenedBookInfo> server_opened_files;
    std::unordered_map<std::string, float> last_server_location;
    ServerStatus status = ServerStatus::NotLoggedIn;
    bool one_time_network_operations_performed = false;

    QDateTime last_document_location_upload_time;

    SioyekNetworkManager(DatabaseManager* db_manager, BackgroundTaskManager* task_manager, DocumentManager* document_manager, QObject* parent=nullptr);
    void tts(QObject* parent,
        const std::wstring& text,
        const std::string& document_checksum,
        int page,
        float rate,
        std::function<void(QString file_path, std::vector<float>)> on_done,
        std::function<void(QString)> on_fail);
    void login(std::wstring email, std::wstring password);
    bool handle_network_reply_if_error(QNetworkReply* reply, bool show_message);
    void persist_access_token(std::string access_token);
    void load_access_token();
    void authorize_request(QNetworkRequest* req);
    void download_file_with_hash(QObject* parent, QString hash, std::function<void(QString)> fn);
    void upload_file(QObject* parent, QString path, QString hash, std::function<void()> on_done, std::optional<std::function<void(int, int)>> on_progress={});

    void ocr_file(QObject* parent, QString path, std::function<void(QNetworkReply*)> fn);
    void update_checksum(QObject* parent, QString path, QString old_checksum, QString new_checksum, std::function<void()> fn);
    QNetworkReply* get_user_file_hash_set_reply();
    void update_user_files_hash_set();
    std::optional<QJsonDocument> get_network_json_reply(QNetworkReply* reply);
    QNetworkReply* download_paper_with_name(QObject* parent, const std::wstring& name, QString full_bib_text, PaperDownloadFinishedAction action, std::function<void(QNetworkReply*)> begin_function, std::function<void(QNetworkReply*)> fn);
    QNetworkReply* get_citers_with_name(QObject* parent, const std::wstring& name, std::function<void(QNetworkReply*)> fn);

    void download_unsynced_files(QObject* parent, DatabaseManager* db_manager);
    QNetworkReply* download_paper_with_url(std::wstring paper_url, bool use_archive_url, PaperDownloadFinishedAction action);
    bool is_checksum_available_on_server(const std::string& checksum);
    QNetworkReply* sync_file_location(QString hash, QString document_title, QString timestamp, float offset_y);
    QNetworkReply* get_opened_book_data_from_checksum(QObject* parent, QString checksum, std::function<void(QJsonObject)> fn);
    bool should_sync_location();
    void set_last_server_location(std::string checksum, float offset_y);
    void download_opened_files_info(QObject* widget, std::function<void(QJsonObject)> fn);
    std::vector<OpenedBookInfo> get_excluded_opened_files(std::vector<std::string>& excluded_checksums);
    void handle_one_time_network_operations();
    //void upload_bookmark(MainWidget* parent, const QString& checksum, const BookMark& highlight, std::function<void()> on_success, std::function<void()> on_fail);
    void delete_annot(QObject* parent, const QString& file_checksum, const QString& annot_type, const QString& uuid, std::function<void()> on_success, std::function<void()> on_fail);
    void get_document_annotations(QObject* parent, const QString& document_checksum, std::function<void(std::vector<Highlight>&&, std::vector<BookMark>&&, std::vector<Portal>&&, std::optional<QDateTime> last_access_time)> fn);
    void get_annotations_after(QObject* parent, QDateTime last_update_date, std::function<void(std::vector<std::pair<std::string, Highlight>>&&, std::vector<std::pair<std::string, BookMark>>&&, std::vector<std::pair<std::string, Portal>>&&)> fn);
    void perform_unsynced_inserts_and_deletes(QObject* parent, Document* doc, const QString& checksum, std::function<void()> on_done);
    const std::wstring& get_url_for_annot_upload(const Annotation* annot);
    //const std::wstring& get_url_for_annot_delete(const Annotation* annot);
    void sync_deleted_annot(QObject* parent, Document* doc, const std::string& annot_type, const std::string& uuid);
    bool is_document_available_on_server(Document* doc);

    void upload_annot(QObject* parent, const QString& checksum, const Annotation& annot, std::function<void()> on_success, std::function<void()> on_fail);
    void delete_file_with_checksum(const QString& checksum);
    void debug(QObject* parent, std::function<void()> on_done);
    void semantic_search(QObject* parent, const QString& query, const std::wstring& index, std::function<void(QJsonObject response)> on_done);
    void semantic_search_extractive(QObject* parent, const QString& query, const std::wstring& index, std::function<void(QJsonObject response)> on_done);
    void semantic_ask(QObject* parent, const QString& query, const std::wstring& index, int first_page_end_index, std::function<void(QString)> on_chunk, std::function<void()> on_done);
    void does_index_exist(QObject* parent, const std::wstring& index, std::function<void(bool)> on_done);
    void summarize(QObject* parent, const std::wstring& index, int first_page_end_index, std::function<void(QString)> on_chunk, std::function<void()> on_done);
    void upload_document_index(QObject* parent, const std::wstring& document_content, std::function<void(QJsonObject)> on_done);
    void extract_table_data(QObject* parent, const QPixmap& pixmap, std::function<void(QString)> on_done, std::optional<QString> prompt = {});
    void perform_generic_llm_request(QObject* parent, const QString& system_prompt, const QString& user_prompt, const QPixmap* pixmap, std::function<void(QString)> on_done);
    void sync_document_annotations_to_server(QObject* parent, Document* doc, std::function<void()> on_done);
    void download_annotations_since_last_sync(bool force_all=false);
    std::optional<QDateTime> get_last_server_sync_time();
    void save_last_server_sync_time();
    std::optional<QJsonObject> get_sioyek_json_data();

    void delete_file_from_server(QObject* parent, std::string checksum, std::function<void()> on_done);
    QNetworkReply* upload_drawings(QObject* parent, std::string pdf_file_checksum, std::wstring drawing_file_path, std::function<void()> on_done);
    void get_last_drawing_modification_time(QObject* parent, std::string pdf_file_checksum, std::function<void(std::optional<QDateTime>)> on_done);
    void download_drawings(QObject* parent, std::string checksum, std::wstring target_path, std::function<void()> on_done);
    void download_new_annotations(QObject* parent, QDateTime last_update_time, bool force=false);
    void search_all_documents(QObject* parentm, QString query, std::function<void(std::vector<QString>)> on_done);
    QNetworkAccessManager* get_network_manager();
    bool should_sync_document_to_server(Document* doc);
    QString get_login_status_string(Document* current_document);
    void handle_logout();
    void cancel_all_downlods();
    void semantic_ask_with_image(
        QObject * parent,
        const std::wstring& document_content,
        const QPixmap& pixmap,
        std::function<void(QString)>&& on_chunk,
        std::function<void()>&& on_done
    );
};

void block_for_send(QNetworkReply* reply);
void block_for_sends(std::vector<QNetworkReply*> reply);
