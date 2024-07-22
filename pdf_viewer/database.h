#pragma once

#include <iostream>
#include <vector>
#include <iostream>
#include <string>
#include "sqlite3.h"
#include "book.h"

class CachedChecksummer;

struct DatabaseColumnMetadata {
    int cid;
    QString name;
    QString type;
    bool notnull;
    QString dflt_value;
    bool pk;

    void print();
};

struct DatabaseTableMetadata {
    QString table_name;
    std::vector<DatabaseColumnMetadata> columns;

    void print();
    std::vector<std::string> get_column_names();
    DatabaseColumnMetadata get_column_with_name(QString name);
};



class DatabaseManager {
private:
    sqlite3_stmt* select_opened_books_stmt = nullptr;

    sqlite3_stmt* select_document_portals_stmt = nullptr;
    int select_document_portals_stmt_src_document_index = -1;

    sqlite3* local_db;
    bool create_opened_books_table();
    bool create_marks_table();
    bool create_bookmarks_table();
    bool create_links_table();
    void create_tables();
    bool create_document_hash_table();
    bool create_full_text_search_table();
    bool create_document_fulltext_indexed_table();
    bool create_server_update_time_table();
    bool create_unsynced_deletions_table();
    //bool create_unsynced_additions_table();
public:
    sqlite3* global_db;
    bool create_highlights_table(sqlite3* db);

    bool open(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path);
    bool select_opened_book(const std::string& book_path, std::vector<OpenedBookState>& out_result);
    bool insert_mark(const std::string& checksum,
        char symbol,
        float offset_y,
        std::wstring uuid,
        std::optional<float> offset_x = {},
        std::optional<float> zoom_level = {});
    bool update_mark(const std::string& checksum, char symbol, float offset_y, std::optional<float> offset_x, std::optional<float> zoom_level);
    bool update_book(const std::string& path, bool is_synced, float zoom_level, float offset_x, float offset_y, std::wstring actual_name=L"");
    bool select_mark(const std::string& checksum, std::vector<Mark>& out_result);
    bool insert_bookmark(const std::string& checksum, const std::wstring& desc, float offset_y, std::wstring uuid);
    bool insert_bookmark_marked(const std::string& checksum, const std::wstring& desc, float offset_x, float offset_y, std::wstring uuid);
    //bool insert_bookmark_freetext(const std::string& checksum, const std::wstring& desc, float begin_x, float begin_y, float end_x, float end_y, float color_red, float color_green, float color_blue, float font_size, std::string font_face, std::wstring uuid);
    bool insert_bookmark_freetext(const std::string& checksum, const BookMark& bm);
    bool insert_bookmark_synced(const std::string& checksum, const BookMark& bm);
    bool insert_or_update_bookmark_synced(bool or_update, const std::string& checksum, const BookMark& bm);
    bool insert_portal_synced(const std::string& checksum, const Portal& portal);
    bool insert_or_update_portal_synced(bool or_udpate, const std::string& checksum, const Portal& portal);
    bool select_bookmark(const std::string& checksum, std::vector<BookMark>& out_result);
    bool insert_portal(const std::string& src_checksum,
        const std::string& dst_checksum,
        float dst_offset_y,
        float dst_offset_x,
        float dst_zoom_level,
        float src_offset_y,
        std::wstring uuid);

    bool insert_visible_portal(const std::string& src_checksum,
        const std::string& dst_checksum,
        float dst_offset_y,
        float dst_offset_x,
        float dst_zoom_level,
        float src_offet_x,
        float src_offset_y,
        std::wstring uuid);

    bool select_links(const std::string& src_checksum, std::vector<Portal>& out_result);
    bool delete_portal(const std::string& uuid);
    bool delete_bookmark(const std::string& uuid);
    bool global_select_bookmark(std::vector<std::pair<std::string, BookMark>>& out_result);
    bool global_select_highlight(std::vector<std::pair<std::string, Highlight>>& out_result);
    //bool update_portal(const std::string& checksum, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y);
    bool update_portal(const std::string& uuid, float dst_offset_x, float dst_offset_y, float dst_zoom_level);
    bool update_highlight_with_server_highlight(const Highlight& server_highlight);
    bool update_bookmark_with_server_bookmark(const BookMark& server_highlight);
    bool update_portal_with_server_portal(const Portal& server_portal);
    bool update_annot_with_server_annot(const Annotation* server_annot);
    bool update_highlight_add_annotation(const std::string& uuid, const std::wstring& text_annot);
    bool update_highlight_type(const std::string& uuid, char new_type);
    bool update_bookmark_change_text(const std::string& uuid, const std::wstring& new_text, float new_font_size);
    bool update_bookmark_change_position(const std::string& uuid, AbsoluteDocumentPos new_begin, AbsoluteDocumentPos new_end);
    bool update_portal_change_src_position(const std::string& uuid, AbsoluteDocumentPos new_pos);
    bool select_opened_books_path_values(std::vector<std::wstring>& out_result);
    //bool select_opened_books_path_and_doc_names(std::vector<std::pair<std::wstring, std::wstring>>& out_result);
    bool select_opened_books(std::vector<OpenedBookInfo>& out_result);
    std::optional<std::string> get_document_last_access_time(const std::string& checksum);

    bool delete_mark_with_symbol(char symbol);
    bool select_global_mark(char symbol, std::vector<std::pair<std::string, float>>& out_result);
    bool delete_opened_book(const std::string& book_path);
    bool delete_highlight(const std::string& uuid);
    bool select_highlight(const std::string& checksum, std::vector<Highlight>& out_result);
    bool select_highlight_with_type(const std::string& checksum, char type, std::vector<Highlight>& out_result);
    bool set_actual_document_name(const std::string& checksum, const std::wstring& actual_name);
    bool insert_highlight(const std::string& checksum,
        const std::wstring& desc,
        float begin_x,
        float begin_y,
        float end_x,
        float end_y,
        char type,
        std::wstring uuid);
    bool insert_highlight_with_annotation(const std::string& checksum,
        const std::wstring& desc,
        const std::wstring& annot,
        float begin_x,
        float begin_y,
        float end_x,
        float end_y,
        char type,
        std::wstring uuid);

    bool insert_highlight_synced(const std::string& checksum, const Highlight& hl);
    bool insert_or_update_highlight_synced(bool or_update, const std::string& checksum, const Highlight& hl);

    bool get_path_from_hash(const std::string& checksum, std::vector<std::wstring>& out_paths);
    bool get_hash_from_path(const std::string& path, std::vector<std::wstring>& out_checksum);
    bool get_prev_path_hash_pairs(std::vector<std::pair<std::wstring, std::wstring>>& out_pairs);
    bool get_all_local_checksums(std::vector<std::string>& out_checksum);
    bool insert_document_hash(const std::wstring& path, const std::string& checksum);
    void upgrade_database_hashes();
    void split_database(const std::wstring& local_database_path, const std::wstring& global_database_path, bool was_using_hashes);
    void export_json(std::wstring json_file_path, CachedChecksummer* checksummer);
    void import_json(std::wstring json_file_path, CachedChecksummer* checksummer);
    //void ensure_database_compatibility(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path);
    //void ensure_schema_compatibility();
    int get_version();
    int set_version();
    bool run_schema_query(sqlite3* db, const char* query);
    void migrate_version_0_to_1();
    void migrate_version_1_to_2();
    bool select_all_mark_ids(std::vector<int>& mark_ids);
    bool select_all_bookmark_ids(std::vector<int>& mark_ids);
    bool select_all_highlight_ids(std::vector<int>& mark_ids);
    bool select_all_portal_ids(std::vector<int>& mark_ids);
    bool insert_update_time(const std::string& checksum);
    std::optional<std::string> get_update_time(const std::string& checksum);

    std::string get_annot_table_name(Annotation* annot);
    bool insert_unsynced_deletion(const std::string& type, const std::string& uuid, const std::string& checksum);
    //bool insert_unsynced_addition(const std::string& type, const std::string& uuid, const std::string& checksum);

    bool get_document_unsynced_table_uuids(const std::string& table_name, const std::string& checksum, std::vector<std::string>& out_uuids);
    //bool get_document_unsynced_bookmark_uuids(const std::string& checksum, std::vector<std::string>& out_uuids);
    bool get_all_unsynced_deletions(const std::string& checksum, std::vector<std::pair<std::wstring, std::wstring>>& out_results);
    //bool get_all_unsynced_additions(const std::string& checksum, std::vector<std::pair<std::wstring, std::wstring>>& out_results);

    bool clear_unsynced_deletions(const std::string& checksum);
    //bool clear_unsynced_additions(const std::string& checksum);

    bool insert_annotation(Annotation* annot, std::string document_hash);
    bool update_annotation(Annotation* annot);
    bool delete_annotation(Annotation* annot);
    bool update_file_name(std::wstring old_name, std::wstring new_name);

    std::wstring generic_update_create_query(std::string table_name,
        std::vector<std::pair<QString, QVariant>> selections,
        std::vector<std::pair<QString, QVariant>> updated_values);

    std::wstring generic_insert_create_query(std::string table_name,
        std::vector<std::pair<QString, QVariant>> values);

    bool generic_update_run_query(std::string table_name,
        std::vector<std::pair<QString, QVariant>> selections,
        std::vector<std::pair<QString, QVariant>> updated_values);

    bool generic_insert_run_query(std::string table_name,
        std::vector<std::pair<QString, QVariant>> values, sqlite3* db=nullptr);

    bool has_column(const std::string& table_name, const std::string& column_name);
    void add_synced_columns();
    void add_document_sync_columns();
    bool set_annot_uuids_to_synced_(const std::string& table_name, const std::vector<std::string>& uuids);
    //bool set_highlight_uuid_to_synced(const std::string& uuid);
    bool set_document_to_synced(const std::string& checksum);
    bool set_document_to_unsynced(const std::string& checksum);
    bool is_document_synced(const std::string& checksum);
    bool update_checksum(const std::string& old_checksum, const std::string& new_checksum);
    std::string get_table_name_for_annot_type(const std::string& annot_type);
    void debug();

    bool import_local(QString local_database_file_path);
    bool import_shared(QString shared_database_file_path);

    bool generic_prepared_statement_run(sqlite3* db, sqlite3_stmt** stmt, const std::string& query, std::function<void()> on_init,std::function<void()> bind_params, std::function<void()> on_row);
    void index_document(std::string document_checksum, const std::wstring& super_fast_search_index, const std::vector<int>& page_indices);
    bool is_document_indexed(std::string document_checksum);
    std::vector<FulltextSearchResult> perform_fulltext_search(const std::wstring& query, std::wstring file_checksum=L"");
};


void migrate_table(sqlite3* src_db, sqlite3* dst_db, std::string table_name);
void migrate_database(sqlite3* old_local, sqlite3* old_shared, sqlite3* new_local, sqlite3* new_shared);
