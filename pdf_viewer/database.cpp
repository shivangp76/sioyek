#include "database.h";
#include <sstream>
#include <cassert>
#include <utility>
#include <set>
#include <optional>
#include <unordered_map>
#include <variant>
#include <iomanip>

#include <QDebug>
#include <qfile.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <quuid.h>

#include "checksum.h"
#include "utils.h"

extern bool DEBUG;
extern float HIGHLIGHT_DELETE_THRESHOLD;
extern int DATABASE_VERSION;

std::string column_std_string(sqlite3_stmt* stmt, int index) {
    const unsigned char* text = sqlite3_column_text(stmt, index);
    return QString::fromUtf8(text).toStdString();
}

std::wstring column_std_wstring(sqlite3_stmt* stmt, int index) {
    const unsigned char* text = sqlite3_column_text(stmt, index);
    return QString::fromUtf8(text).toStdWString();
}

QString column_qstring(sqlite3_stmt* stmt, int index) {
    const unsigned char* text = sqlite3_column_text(stmt, index);
    return QString::fromUtf8(text);
}

std::wstring esc(const std::wstring& inp) {
    char* data = sqlite3_mprintf("%q", utf8_encode(inp).c_str());
    std::wstring escaped_string = utf8_decode(data);
    sqlite3_free(data);
    return escaped_string;
}

std::wstring esc(const std::string& inp) {
    return esc(utf8_decode(inp));
}

static int null_callback(void* notused, int argc, char** argv, char** col_name) {
    return 0;
}

static int count_callback(void* res_ptr, int argc, char** argv, char** col_name) {
    *(int*)res_ptr = atoi(argv[0]);
    return 0;
}

static int id_callback(void* res_vector, int argc, char** argv, char** col_name) {
    std::vector<int>* res = (std::vector<int>*) res_vector;

    if (argc != 1) {
        std::cerr << "Error in file " << __FILE__ << " " << "Line: " << __LINE__ << std::endl;
    }

    res->push_back(atoi(argv[0]));

    return 0;
}

static int opened_book_callback(void* res_vector, int argc, char** argv, char** col_name) {
    std::vector<OpenedBookState>* res = (std::vector<OpenedBookState>*) res_vector;

    if (argc != 3) {
        std::cerr << "Error in file " << __FILE__ << " " << "Line: " << __LINE__ << std::endl;
    }

    float zoom_level = atof(argv[0]);
    float offset_x = atof(argv[1]);
    float offset_y = atof(argv[2]);

    res->push_back(OpenedBookState{ zoom_level, offset_x, offset_y });
    return 0;
}


static int prev_doc_callback(void* res_vector, int argc, char** argv, char** col_name) {
    std::vector<std::wstring>* res = (std::vector<std::wstring>*) res_vector;

    if (argc != 1) {
        std::cerr << "Error in file " << __FILE__ << " " << "Line: " << __LINE__ << std::endl;
    }

    res->push_back(utf8_decode(argv[0]));
    return 0;
}

static int prev_doc_with_name_callback(void* res_vector, int argc, char** argv, char** col_name) {
    std::vector<std::pair<std::wstring, std::wstring>>* res = (std::vector<std::pair<std::wstring, std::wstring>>*) res_vector;

    assert(argc == 2);

    std::wstring path = utf8_decode(argv[0]);
    std::wstring doc_name = L"";
    if (argv[1]) {
        doc_name = utf8_decode(argv[1]);
    }
    res->push_back(std::make_pair(path, doc_name));
    return 0;
}

//static int opened_books_callback(void* res_vector, int argc, char** argv, char** col_name) {
//    std::vector<OpenedBookInfo>* res = (std::vector<OpenedBookInfo>*) res_vector;
//
//    assert(argc == 3);
//
//    OpenedBookInfo info;
//
//    info.checksum = argv[0];
//    info.document_title = "";
//    if (argv[1]) {
//        info.document_title = argv[1];
//    }
//    if (argv[2]) {
//        info.last_access_time = QDateTime::fromString(QString(argv[2]), Qt::ISODate);
//    }
//
//    res->push_back(info);
//    return 0;
//}

static int mark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<Mark>* res = (std::vector<Mark>*)res_vector;
    assert(argc == 8);

    char symbol = argv[0][0];
    float offset_y = atof(argv[1]);
    std::optional<float> offset_x = {};
    std::optional<float> zoom_level = {};
    if (argv[2]) offset_x = atof(argv[2]);
    if (argv[3]) zoom_level = atof(argv[3]);
    std::string uuid = argv[4];
    std::string creation_time = argv[5];
    std::string modification_time = argv[6];
    bool is_synced = atoi(argv[7]);

    Mark m;
    m.y_offset = offset_y;
    m.symbol = symbol;
    m.uuid = uuid;
    m.creation_time = creation_time;
    m.modification_time = modification_time;
    m.x_offset = offset_x;
    m.zoom_level = zoom_level;
    m.is_synced = is_synced;

    res->push_back(m);
    return 0;
}

static int global_mark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<std::pair<std::string, float>>* res = (std::vector<std::pair<std::string, float>>*)res_vector;
    assert(argc == 2);

    //char symbol = argv[0][0];
    std::string path = argv[0];
    float offset_y = atof(argv[1]);

    res->push_back(std::make_pair(path, offset_y));
    return 0;
}

static int global_bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<std::pair<std::string, BookMark>>* res = (std::vector<std::pair<std::string, BookMark>>*)res_vector;
    assert(argc == 11);

    std::string path = argv[0];
    std::wstring desc = utf8_decode(argv[1]);
    float offset_y = -1;
    float begin_x = -1;
    float begin_y = -1;
    float end_x = -1;
    float end_y = -1;
    bool offset_y_is_set = false;

    if (argv[2]) {
        offset_y = atof(argv[2]);
        offset_y_is_set = true;
    }
    if (argv[3]) {
        begin_x = atof(argv[3]);
    }
    if (argv[4]) {
        begin_y = atof(argv[4]);
        if (!offset_y_is_set) {
            offset_y = begin_y;
        }
    }
    if (argv[5]) {
        end_x = atof(argv[5]);
    }
    if (argv[6]) {
        end_y = atof(argv[6]);
    }
    std::string uuid = argv[7];
    std::string creation_time = argv[8];
    std::string modification_time = argv[9];
    bool is_synced = atoi(argv[10]);

    BookMark bm;
    bm.description = desc;
    bm.y_offset_ = offset_y;
    bm.uuid = uuid;
    bm.creation_time = creation_time;
    bm.modification_time = modification_time;
    bm.begin_x = begin_x;
    bm.begin_y = begin_y;
    bm.end_x = end_x;
    bm.end_y = end_y;
    bm.is_synced = is_synced;

    res->push_back(std::make_pair(path, bm));
    return 0;
}

static int global_highlight_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<std::pair<std::string, Highlight>>* res = (std::vector<std::pair<std::string, Highlight>>*)res_vector;
    assert(argc == 12);

    std::string path = argv[0];
    std::wstring desc = utf8_decode(argv[1]);
    std::wstring text_annot;

    if (argv[2] != nullptr) {
        text_annot = utf8_decode(argv[2]);
    }

    char type = argv[3][0];
    float begin_x = atof(argv[4]);
    float begin_y = atof(argv[5]);
    float end_x = atof(argv[6]);
    float end_y = atof(argv[7]);
    std::string uuid = argv[8];
    std::string creation_time = argv[9];
    std::string modification_time = argv[10];
    bool is_synced = atoi(argv[11]);

    Highlight highlight;
    highlight.description = desc;
    highlight.text_annot = text_annot;
    highlight.type = type;
    highlight.selection_begin.x = begin_x;
    highlight.selection_begin.y = begin_y;
    highlight.selection_end.x = end_x;
    highlight.selection_end.y = end_y;
    highlight.uuid = uuid;
    highlight.creation_time = creation_time;
    highlight.modification_time = modification_time;
    highlight.is_synced = is_synced;

    res->push_back(std::make_pair(path, highlight));
    return 0;
}

static int bookmark_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<BookMark>* res = (std::vector<BookMark>*)res_vector;
    assert(argc == 15);

    std::wstring desc = utf8_decode(argv[0]);
    float offset_y = -1;
    float begin_x = -1;
    float begin_y = -1;
    float end_x = -1;
    float end_y = -1;
    float color_red = 0;
    float color_green = 0;
    float color_blue = 0;
    float font_size = -1;
    std::wstring font_face = L"";

    if (argv[1]) {
        offset_y = atof(argv[1]);
    }
    if (argv[2]) {
        begin_x = atof(argv[2]);
    }
    if (argv[3]) {
        begin_y = atof(argv[3]);
        if (begin_y != -1) {
            offset_y = begin_y;
        }
    }
    if (argv[4]) {
        end_x = atof(argv[4]);
    }
    if (argv[5]) {
        end_y = atof(argv[5]);
    }
    if (argv[6]) {
        color_red = atof(argv[6]);
    }
    if (argv[7]) {
        color_green = atof(argv[7]);
    }
    if (argv[8]) {
        color_blue = atof(argv[8]);
    }

    if (argv[9]) {
        font_size = atof(argv[9]);
    }
    if (argv[10]) {
        font_face = utf8_decode(argv[10]);
    }
    std::string uuid = argv[11];
    std::string creation_time = argv[12];
    std::string modification_time = argv[13];
    bool is_synced = atoi(argv[14]);

    BookMark bm;
    bm.y_offset_ = offset_y;
    bm.description = desc;
    bm.uuid = uuid;
    bm.creation_time = creation_time;
    bm.modification_time = modification_time;
    bm.begin_x = begin_x;
    bm.begin_y = begin_y;
    bm.end_x = end_x;
    bm.end_y = end_y;
    bm.color[0] = color_red;
    bm.color[1] = color_green;
    bm.color[2] = color_blue;
    bm.font_size = font_size;
    bm.font_face = font_face;
    bm.is_synced = is_synced;

    res->push_back(bm);
    return 0;
}

static int wstring_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<std::wstring>* res = (std::vector<std::wstring>*)res_vector;
    assert(argc == 1);

    std::wstring desc = utf8_decode(argv[0]);

    res->push_back(desc);
    return 0;
}

static int string_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<std::string>* res = (std::vector<std::string>*)res_vector;
    assert(argc == 1);

    std::string desc = argv[0];

    res->push_back(desc);
    return 0;
}

static int wstring_pair_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<std::pair<std::wstring, std::wstring>>* res = (std::vector<std::pair<std::wstring, std::wstring>>*)res_vector;
    assert(argc == 2);

    std::wstring first = utf8_decode(argv[0]);
    std::wstring second = utf8_decode(argv[1]);

    res->push_back(std::make_pair(first, second));
    return 0;
}

static int version_callback(void* res, int argc, char** argv, char** col_name) {

    //std::vector<std::pair<std::wstring, std::wstring>>* res = (std::vector<std::pair<std::wstring, std::wstring>>*)res_vector;
    assert(argc == 1);
    *(int*)res = atoi(argv[0]);
    //std::wstring first = utf8_decode(argv[0]);
    //std::wstring second = utf8_decode(argv[1]); 
    //res->push_back(std::make_pair(first, second));

    return 0;
}

static int highlight_select_callback(void* res_vector, int argc, char** argv, char** col_name) {

    std::vector<Highlight>* res = (std::vector<Highlight>*)res_vector;
    assert(argc == 11);

    std::wstring desc = utf8_decode(argv[0]);
    std::wstring text_annot = L"";

    if (argv[1] != nullptr) {
        text_annot = utf8_decode(argv[1]);
    }

    float begin_x = atof(argv[2]);
    float begin_y = atof(argv[3]);
    float end_x = atof(argv[4]);
    float end_y = atof(argv[5]);
    char type = argv[6][0];
    std::string uuid = argv[7];
    std::string creation_time = argv[8];
    std::string modification_time = argv[9];
    bool is_synced = atoi(argv[10]);

    Highlight highlight;
    highlight.description = desc;
    highlight.text_annot = text_annot;
    highlight.type = type;
    highlight.selection_begin = { begin_x, begin_y };
    highlight.selection_end = { end_x, end_y };
    highlight.creation_time = creation_time;
    highlight.modification_time = modification_time;
    highlight.uuid = uuid;
    highlight.is_synced = is_synced;
    res->push_back(highlight);

    return 0;
}

//static int link_select_callback(void* res_vector, int argc, char** argv, char** col_name) {
//
//    std::vector<Portal>* res = (std::vector<Portal>*)res_vector;
//    assert(argc == 10);
//
//    bool is_visible = false;
//
//    std::string dst_path = argv[0];
//    float src_offset_y = atof(argv[1]);
//    std::optional<float> src_offset_x = {};
//
//    if (argv[2]) {
//        src_offset_x = atof(argv[2]);
//    }
//
//    float dst_offset_x = atof(argv[3]);
//    float dst_offset_y = atof(argv[4]);
//    float dst_zoom_level = atof(argv[5]);
//    std::string uuid = argv[6];
//    std::string creation_time = argv[7];
//    std::string modification_time = argv[8];
//    bool is_synced = atoi(argv[9]);
//
//    Portal link;
//    link.dst.document_checksum = dst_path;
//    link.src_offset_y = src_offset_y;
//    link.src_offset_x = src_offset_x;
//    link.dst.book_state.offset_x = dst_offset_x;
//    link.dst.book_state.offset_y = dst_offset_y;
//    link.dst.book_state.zoom_level = dst_zoom_level;
//    link.uuid = uuid;
//    link.creation_time = creation_time;
//    link.modification_time = modification_time;
//    link.is_synced = is_synced;
//
//    res->push_back(link);
//    return 0;
//}

//template<typename T>
//T parse_single(char* inp) {
//	return T();
//}
//
//template<>
//int parse_single<int>(char* inp) {
//	return std::stoi(inp);
//}
//
//template<>
//float parse_single<float>(char* inp) {
//	return std::stof(inp);
//}
//
//template<>
//std::string parse_single<std::string>(char* inp) {
//	return std::string(inp);
//}
//
//template<>
//std::wstring parse_single<std::wstring>(char* inp) {
//	return utf8_decode(std::string(inp));
//}
//
//template<>
//char parse_single<char>(char* inp) {
//	return *inp;
//}
//
//template <size_t i>
//std::tuple<> unpack_helper(void* res_vector, int argc, char** argv, char** col_name) {
//	return std::tuple<>();
//}
//
//template <size_t i, typename T, typename... Types>
//std::tuple<T, Types...> unpack_helper(void* res_vector, int argc, char** argv, char** col_name) {
//	return std::tuple_cat(std::tuple<T>(parse_single<T>(argv[i])), unpack_helper<i + 1, Types...>(res_vector, argc, argv, col_name));
//}
//
//template<typename... Types>
//std::tuple<Types...> unpack(void* res_vector, int argc, char** argv, char** col_name) {
//	auto item = unpack_helper<0, Types...>(res_vector, argc, argv, col_name);
//	(static_cast<std::vector<std::tuple<Types...>>*>(res_vector))->push_back(item);
//}

//template<typename T>
//T::tuple_type unpack_object(void* res_vector, int argc, char** argv, char** col_name) {
//	//using std::tuple<Types...> = T::tuple_type;
//	auto item = unpack_helper<Types...>(res_vector, argc, argv, col_name);
//	(static_cast<std::vector<std::tuple<Types...>>*>(res_vector))->push_back(item);
//}

//template<typename T>
//T unpack(void* res_vector, int argc, char** argv, char** col_name) {
//	auto item = unpack_helper<Types...>(res_vector, argc, argv, col_name);
//	(static_cast<std::vector<std::tuple<Types...>>*>(res_vector))->push_back(item);
//}

bool handle_error(const QString& func_name, int error_code, char* error_message) {
    if (error_code != SQLITE_OK) {
        qDebug() << "SQL Error in " << func_name << " : " << error_message << "\n";
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

bool DatabaseManager::open(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path) {

    sqlite3_config(SQLITE_CONFIG_SERIALIZED);

    std::string local_database_file_path_utf8 = utf8_encode(local_db_file_path);
    int local_rc = sqlite3_open(local_database_file_path_utf8.c_str(), &local_db);

    if (local_rc) {
        std::cerr << "could not create local database" << sqlite3_errmsg(local_db) << std::endl;
        local_db = nullptr;
        global_db = nullptr;
        return false;
    }

    if (local_db_file_path != global_db_file_path) {
        std::string global_database_file_path_utf8 = utf8_encode(global_db_file_path);
        int global_rc = sqlite3_open(global_database_file_path_utf8.c_str(), &global_db);

        if (global_rc) {
            std::cerr << "could not create global database" << sqlite3_errmsg(global_db) << std::endl;
            local_db = nullptr;
            global_db = nullptr;
            return false;
        }
    }
    else {
        global_db = local_db;
    }

    return true;
}

bool DatabaseManager::create_opened_books_table() {

    const char* create_opened_books_sql = "CREATE TABLE IF NOT EXISTS opened_books ("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"\
        "path TEXT UNIQUE,"\
        "document_name TEXT,"\
        "zoom_level REAL,"\
        "offset_x REAL,"\
        "offset_y REAL,"\
        "last_access_time TEXT,"\
        "is_synced BOOLEAN DEFAULT 0); ";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, create_opened_books_sql, null_callback, 0, &error_message);
    return handle_error("create_opened_books_table", error_code, error_message);
}

bool DatabaseManager::create_server_update_time_table() {

    const char* create_update_times_table = "CREATE TABLE IF NOT EXISTS update_times ("\
        "checksum TEXT UNIQUE,"\
        "last_update_time TIMESTAMP);";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, create_update_times_table, null_callback, 0, &error_message);
    return handle_error("create_update_times_table", error_code, error_message);
}

bool DatabaseManager::create_unsynced_deletions_table() {
    const char* create_update_times_table = "CREATE TABLE IF NOT EXISTS local_unsynced_deletions ("\
        "type TEXT NOT NULL,"\
        "checksum TEXT NOT NULL,"\
        "uuid TEXT UNIQUE NOT NULL);";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, create_update_times_table, null_callback, 0, &error_message);
    return handle_error("create_unsynced_deletions_table", error_code, error_message);
}
//
//bool DatabaseManager::create_unsynced_additions_table() {
//    const char* create_update_times_table = "CREATE TABLE IF NOT EXISTS local_unsynced_additions ("\
//        "type TEXT NOT NULL,"\
//        "checksum TEXT NOT NULL,"\
//        "uuid TEXT UNIQUE NOT NULL);";
//
//    char* error_message = nullptr;
//    int error_code = sqlite3_exec(local_db, create_update_times_table, null_callback, 0, &error_message);
//    return handle_error("create_unsynced_additions_table", error_code, error_message);
//}

bool DatabaseManager::create_marks_table() {
    const char* create_marks_sql = "CREATE TABLE IF NOT EXISTS marks ("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT," \
        "document_path TEXT,"\
        "symbol CHAR,"\
        "offset_x real,"\
        "offset_y real,"\
        "zoom_level real,"\
        "creation_time timestamp,"\
        "modification_time timestamp,"\
        "uuid TEXT,"\
        "is_synced BOOLEAN DEFAULT 0,"\
        "UNIQUE(document_path, symbol));";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, create_marks_sql, null_callback, 0, &error_message);
    return handle_error(
        "create_marks_table",
        error_code,
        error_message);
}

bool DatabaseManager::create_bookmarks_table() {
    const char* create_bookmarks_sql = "CREATE TABLE IF NOT EXISTS bookmarks ("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT," \
        "document_path TEXT,"\
        "desc TEXT,"\
        "creation_time timestamp,"\
        "modification_time timestamp,"\
        "uuid TEXT,"\
        "is_synced BOOLEAN DEFAULT 0,"\
        "font_size integer DEFAULT -1,"\
        "color_red real DEFAULT 0,"\
        "color_green real DEFAULT 0,"\
        "color_blue real DEFAULT 0,"\
        "font_face TEXT,"\
        "begin_x real DEFAULT -1,"\
        "begin_y real DEFAULT -1,"\
        "end_x real DEFAULT -1,"\
        "end_y real DEFAULT -1,"\
        "offset_y real);";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, create_bookmarks_sql, null_callback, 0, &error_message);
    return handle_error(
        "create_bookmarks_table",
        error_code,
        error_message);
}

bool DatabaseManager::create_highlights_table() {
    const char* create_highlights_sql = "CREATE TABLE IF NOT EXISTS highlights ("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT," \
        "document_path TEXT,"\
        "desc TEXT,"\
        "text_annot TEXT,"\
        "type char,"\
        "creation_time timestamp,"\
        "modification_time timestamp,"\
        "uuid TEXT,"\
        "is_synced BOOLEAN DEFAULT 0,"\
        "begin_x real,"\
        "begin_y real,"\
        "end_x real,"\
        "end_y real);";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, create_highlights_sql, null_callback, 0, &error_message);
    return handle_error(
        "create_highlights_table",
        error_code,
        error_message);
}

bool DatabaseManager::create_document_hash_table() {
    const char* create_document_hash_sql = "CREATE TABLE IF NOT EXISTS document_hash ("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT," \
        "path TEXT,"\
        "hash TEXT,"\
        "UNIQUE(path, hash));";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, create_document_hash_sql, null_callback, 0, &error_message);
    return handle_error(
        "create_document_hash_table",
        error_code,
        error_message);
}

bool DatabaseManager::create_links_table() {
    const char* create_marks_sql = "CREATE TABLE IF NOT EXISTS links ("\
        "id INTEGER PRIMARY KEY AUTOINCREMENT," \
        "creation_time timestamp,"\
        "modification_time timestamp,"\
        "uuid TEXT,"\
        "is_synced BOOLEAN DEFAULT 0,"\
        "src_document TEXT,"\
        "dst_document TEXT,"\
        "src_offset_y REAL,"\
        "src_offset_x REAL,"\
        "dst_offset_x REAL,"\
        "dst_offset_y REAL,"\
        "dst_zoom_level REAL);";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, create_marks_sql, null_callback, 0, &error_message);
    return handle_error(
        "create_links_table",
        error_code,
        error_message);
}

//bool DatabaseManager::insert_book(const std::string& path, float zoom_level, float offset_x, float offset_y) {
//	const char* insert_books_sql = ""\
//		"INSERT INTO opened_books (PATH, zoom_level, offset_x, offset_y, last_access_time) VALUES ";
//
//	std::wstringstream ss;
//	ss << insert_books_sql << "'" << esc(path) << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now');";
//	
//	char* error_message = nullptr;
//	int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
//	return handle_error(
//		error_code,
//		error_message);
//}

bool DatabaseManager::insert_document_hash(const std::wstring& path, const std::string& checksum) {

    const char* delete_doc_sql = ""\
        "DELETE FROM document_hash WHERE path=";

    const char* insert_doc_hash_sql = ""\
        "INSERT INTO document_hash (path, hash) VALUES (";

    std::wstringstream insert_ss;
    insert_ss << insert_doc_hash_sql << "'" << esc(path) << "', '" << esc(checksum) << "');";

    std::wstringstream delete_ss;
    delete_ss << delete_doc_sql << "'" << esc(path) << "';";

    char* delete_error_message = nullptr;
    int delete_error_code = sqlite3_exec(local_db, utf8_encode(delete_ss.str()).c_str(), null_callback, 0, &delete_error_message);
    handle_error(
        "insert_document_hash",
        delete_error_code, delete_error_message);

    char* insert_error_message = nullptr;
    int insert_error_code = sqlite3_exec(local_db, utf8_encode(insert_ss.str()).c_str(), null_callback, 0, &insert_error_message);
    return handle_error("insert_document_hash", insert_error_code, insert_error_message);
}

bool DatabaseManager::update_book(const std::string& path, bool is_synced, float zoom_level, float offset_x, float offset_y, std::wstring actual_name) {

    std::wstringstream ss;
    ss << "insert or replace into opened_books(path, zoom_level, offset_x, offset_y, last_access_time, document_name, is_synced) values ('" <<
        esc(path) << "', " << zoom_level << ", " << offset_x << ", " << offset_y << ", datetime('now'), '" << esc(actual_name) << "', " << (is_synced ? 1 : 0) << ");";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "update_book",
        error_code,
        error_message);
}

bool DatabaseManager::insert_mark(
    const std::string& document_path,
    char symbol,
    float offset_y,
    std::wstring uuid,
    std::optional<float> offset_x,
    std::optional<float> zoom_level) {

    //todo: probably should escape symbol too
    std::wstringstream ss;
    if (!offset_x.has_value()) {
        ss << "INSERT INTO marks (document_path, symbol, offset_y, uuid, creation_time, modification_time) VALUES ('" << esc(document_path) << "', '" << symbol << "', " << offset_y << ", '" << esc(uuid) << "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
    }
    else {
        ss << "INSERT INTO marks (document_path, symbol, offset_y, uuid, creation_time, modification_time, offset_x, zoom_level) VALUES ('"
            << esc(document_path) <<
            "', '" << symbol <<
            "', " << offset_y <<
            ", '" << esc(uuid) <<
            "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, "
            << offset_x.value() <<
            ", " << zoom_level.value() << ");";
    }
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_mark",
        error_code,
        error_message);
}

bool DatabaseManager::delete_mark_with_symbol(char symbol) {

    std::wstringstream ss;
    ss << "DELETE FROM marks where symbol='" << symbol << "';";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "delete_mark_with_symbol",
        error_code,
        error_message);
}

bool DatabaseManager::insert_bookmark(const std::string& document_path, const std::wstring& desc, float offset_y, std::wstring uuid) {

    std::wstringstream ss;
    ss << "INSERT INTO bookmarks (document_path, desc, offset_y, uuid, creation_time, modification_time) VALUES ('" << esc(document_path) << "', '" << esc(desc) << "', " << offset_y << ", '" << esc(uuid) << "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_bookmark",
        error_code,
        error_message);
}

bool DatabaseManager::insert_bookmark_marked(const std::string& document_path, const std::wstring& desc, float offset_x, float offset_y, std::wstring uuid) {

    std::wstringstream ss;
    ss << "INSERT INTO bookmarks (document_path, desc, begin_x, begin_y, uuid, creation_time, modification_time) VALUES ('" << esc(document_path) << "', '" << esc(desc) << "', " << offset_x << " , " << offset_y << ", '" << esc(uuid) << "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_bookmark_marked",
        error_code,
        error_message);
}

bool DatabaseManager::insert_bookmark_freetext(const std::string& document_path, const BookMark& bm) {

    std::wstringstream ss;
    ss << "INSERT INTO bookmarks (document_path, desc, begin_x, begin_y, end_x, end_y, color_red, color_green, color_blue, font_size, font_face, uuid, creation_time, modification_time) VALUES ('"
        << esc(document_path) << "', '"
        << esc(bm.description) << "', "
        << bm.begin_x << " , "
        << bm.begin_y << " , "
        << bm.end_x << " , "
        << bm.end_y << ", "
        << bm.color[0] << ", "
        << bm.color[1] << ", "
        << bm.color[2] << ", "
        << bm.font_size << ", '"
        << bm.font_face << "', '"
        << esc(bm.uuid) << "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_bookmark_freetext",
        error_code,
        error_message);
}

bool DatabaseManager::insert_bookmark_synced(const std::string& document_path, const BookMark& bm) {

    std::wstringstream ss;
    ss << "INSERT INTO bookmarks (document_path, desc, begin_x, begin_y, end_x, end_y, color_red, color_green, color_blue, font_size, font_face, uuid, creation_time, modification_time, is_synced) VALUES ('"
        << esc(document_path) << "', '"
        << esc(bm.description) << "', "
        << bm.begin_x << " , "
        << bm.begin_y << " , "
        << bm.end_x << " , "
        << bm.end_y << ", "
        << bm.color[0] << ", "
        << bm.color[1] << ", "
        << bm.color[2] << ", "
        << bm.font_size << ", '"
        << bm.font_face << "', '"
        << esc(bm.uuid) << "', '" << esc(bm.creation_time) << "', '" << esc(bm.modification_time) << "', 1);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_bookmark_synced",
        error_code,
        error_message);
}

bool DatabaseManager::insert_portal_synced(const std::string& document_path, const Portal& portal) {

    std::wstring src_offset_x_string = portal.src_offset_x ? QString::number(portal.src_offset_x.value()).toStdWString() : L"NULL";
    std::wstringstream ss;
    ss << "INSERT INTO links (src_document, dst_document, src_offset_x, src_offset_y, dst_offset_x, dst_offset_y, dst_zoom_level, uuid, creation_time, modification_time, is_synced) VALUES ('"
        << esc(document_path) << "', '"
        << esc(portal.dst.document_checksum) << "', "
        << src_offset_x_string << " , "
        << portal.src_offset_y << " , "
        << portal.dst.book_state.offset_x << " , "
        << portal.dst.book_state.offset_y << ", "
        << portal.dst.book_state.zoom_level << ", '"
        << esc(portal.uuid) << "', '" << esc(portal.creation_time) << "', '" << esc(portal.modification_time) << "', 1);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_bookmark_freetext",
        error_code,
        error_message);
}

bool DatabaseManager::insert_highlight(const std::string& document_path,
    const std::wstring& desc,
    float begin_x,
    float begin_y,
    float end_x,
    float end_y,
    char type,
    std::wstring uuid) {

    std::wstringstream ss;
    ss << "INSERT INTO highlights (document_path, desc, type, begin_x, begin_y, end_x, end_y, uuid, creation_time, modification_time) VALUES ('" << esc(document_path) << "', '" << esc(desc) << "', '" <<
        type << "' , " <<
        begin_x << " , " <<
        begin_y << " , " <<
        end_x << " , " <<
        end_y << ", '" <<
        esc(uuid) << "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_highlight",
        error_code,
        error_message);
}

bool DatabaseManager::insert_highlight_with_annotation(const std::string& document_path,
    const std::wstring& desc,
    const std::wstring& annot,
    float begin_x,
    float begin_y,
    float end_x,
    float end_y,
    char type,
    std::wstring uuid) {

    std::wstringstream ss;
    ss << "INSERT INTO highlights (document_path, desc, text_annot, type, begin_x, begin_y, end_x, end_y, uuid, creation_time, modification_time) VALUES ('" <<
        esc(document_path) << "', '" <<
        esc(desc) << "', '" <<
        esc(annot) << "', '" <<
        type << "' , " <<
        begin_x << " , " <<
        begin_y << " , " <<
        end_x << " , " <<
        end_y << ", '" <<
        esc(uuid) << "', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_highlight_with_annotation",
        error_code,
        error_message);
}

bool DatabaseManager::insert_highlight_synced(const std::string& document_path,
    const Highlight& hl) {

    std::wstringstream ss;
    ss << "INSERT INTO highlights (document_path, desc, text_annot, type, begin_x, begin_y, end_x, end_y, uuid, creation_time, modification_time, is_synced) VALUES ('" <<
        esc(document_path) << "', '" <<
        esc(hl.description) << "', '" <<
        esc(hl.text_annot) << "', '" <<
        hl.type << "' , " <<
        hl.selection_begin.x << " , " <<
        hl.selection_begin.y << " , " <<
        hl.selection_end.x << " , " <<
        hl.selection_end.y << ", '" <<
        esc(hl.uuid) << "', '" << esc(hl.creation_time) << "', '" << esc(hl.modification_time) << "', 1);";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_highlight_with_annotation_synced",
        error_code,
        error_message);
}


bool DatabaseManager::insert_unsynced_deletion(const std::string& type, const std::string& uuid, const std::string& checksum) {
    return generic_insert_run_query("local_unsynced_deletions",
        {
            { "type", QString::fromStdString(type) },
            { "uuid", QString::fromStdString(uuid) },
            { "checksum", QString::fromStdString(checksum) }
        },
        local_db
    );
}

//bool DatabaseManager::insert_unsynced_addition(const std::string& type, const std::string& uuid, const std::string& checksum) {
//    return generic_insert_run_query("local_unsynced_additions",
//        {
//            { "type", QString::fromStdString(type) },
//            { "uuid", QString::fromStdString(uuid) },
//            { "checksum", QString::fromStdString(checksum) }
//        },
//        local_db
//    );
//}

bool DatabaseManager::get_all_unsynced_deletions(const std::string& checksum, std::vector<std::pair<std::wstring, std::wstring>>& out_results) {
    std::wstringstream ss;
    ss << "SELECT type, uuid FROM local_unsynced_deletions WHERE checksum='" << esc(checksum) << "';";
    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_pair_select_callback, &out_results, &error_message);
    return handle_error(
        "get_all_unsynced_deletions",
        error_code,
        error_message);
}

//bool DatabaseManager::get_all_unsynced_additions(const std::string& checksum, std::vector<std::pair<std::wstring, std::wstring>>& out_results) {
//    std::wstringstream ss;
//    ss << "SELECT type, uuid FROM local_unsynced_additions WHERE checksum='" << esc(checksum) << "';";
//    char* error_message = nullptr;
//    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_pair_select_callback, &out_results, &error_message);
//    return handle_error(
//        "get_all_unsynced_additions",
//        error_code,
//        error_message);
//}


bool DatabaseManager::clear_unsynced_deletions(const std::string& checksum) {
    std::wstringstream ss;
    ss << "DELETE FROM local_unsynced_deletions WHERE checksum='" << esc(checksum) << "';";
    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), nullptr, nullptr, &error_message);
    return handle_error(
        "clear_unsynced_deletions",
        error_code,
        error_message);
}

//bool DatabaseManager::clear_unsynced_additions(const std::string& checksum) {
//    std::wstringstream ss;
//    ss << "DELETE FROM local_unsynced_additions WHERE checksum='" << esc(checksum) << "';";
//    char* error_message = nullptr;
//    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), nullptr, nullptr, &error_message);
//    return handle_error(
//        "clear_unsynced_additions",
//        error_code,
//        error_message);
//}

bool DatabaseManager::get_document_unsynced_table_uuids(const std::string& table_name, const std::string& checksum, std::vector<std::string>& out_uuids) {
    std::wstringstream ss;
    ss << "SELECT uuid FROM " << esc(table_name) << " WHERE document_path='" << esc(checksum) << "' AND is_synced=0;";
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), string_select_callback, &out_uuids, &error_message);
    return handle_error(
        "get_all_unsynced_highlight_uuids",
        error_code,
        error_message);
}

//bool DatabaseManager::get_document_unsynced_bookmark_uuids(const std::string& checksum, std::vector<std::string>& out_uuids) {
//    std::wstringstream ss;
//    ss << "SELECT uuid FROM bookmarks WHERE document_path='" << esc(checksum) << "' AND is_synced=0;";
//    char* error_message = nullptr;
//    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), string_select_callback, &out_uuids, &error_message);
//    return handle_error(
//        "get_all_unsynced_highlight_uuids",
//        error_code,
//        error_message);
//}

bool DatabaseManager::insert_portal(const std::string& src_document_path,
    const std::string& dst_document_path,
    float dst_offset_x,
    float dst_offset_y,
    float dst_zoom_level,
    float src_offset_y,
    std::wstring uuid) {

    return generic_insert_run_query("links", { {"src_document", QString::fromStdString(src_document_path)},
                                              {"dst_document", QString::fromStdString(dst_document_path)},
                                              {"src_offset_y", src_offset_y},
                                              {"dst_offset_x", dst_offset_x},
                                              {"dst_offset_y", dst_offset_y},
                                              {"dst_zoom_level", dst_zoom_level},
                                              {"uuid", QString::fromStdWString(uuid)},
                                              {"creation_time", "CURRENT_TIMESTAMP"},
                                              {"modification_time", "CURRENT_TIMESTAMP"}
        });

}

bool DatabaseManager::insert_visible_portal(const std::string& src_checksum,
    const std::string& dst_checksum,
    float dst_offset_x,
    float dst_offset_y,
    float dst_zoom_level,
    float src_offset_x,
    float src_offset_y,
    std::wstring uuid) {

    return generic_insert_run_query("links", { {"src_document", QString::fromStdString(src_checksum)},
                                                     {"dst_document", QString::fromStdString(dst_checksum)},
                                                     {"src_offset_y", src_offset_y},
                                                     {"src_offset_x", src_offset_x},
                                                     {"dst_offset_x", dst_offset_x},
                                                     {"dst_offset_y", dst_offset_y},
                                                     {"dst_zoom_level", dst_zoom_level},
                                                     {"uuid", QString::fromStdWString(uuid)},
                                                     {"creation_time", "CURRENT_TIMESTAMP"},
                                                     {"modification_time", "CURRENT_TIMESTAMP"}
               });
}

bool DatabaseManager::update_portal(const std::string& uuid, float dst_offset_x, float dst_offset_y, float dst_zoom_level){
    std::wstringstream ss;
    ss << "UPDATE links SET dst_offset_x=" << dst_offset_x << ", dst_offset_y=" << dst_offset_y <<
        ", dst_zoom_level=" << dst_zoom_level << ", modification_time=CURRENT_TIMESTAMP WHERE uuid='" <<
        esc(uuid) << "';";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "update_portal",
        error_code,
        error_message);
}

bool DatabaseManager::delete_portal(const std::string& uuid) {

    std::wstringstream ss;
    ss << "DELETE FROM links where uuid='" << esc(uuid) << "';";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "delete_portal",
        error_code,
        error_message);
}

bool DatabaseManager::delete_bookmark(const std::string& uuid) {

    std::wstringstream ss;
    ss << "DELETE FROM bookmarks where uuid='" << esc(uuid) << "';";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "delete_bookmark",
        error_code,
        error_message);
}

bool DatabaseManager::delete_highlight(const std::string& uuid) {

    std::wstringstream ss;
    std::wstring threshold = QString::number(HIGHLIGHT_DELETE_THRESHOLD).toStdWString();
    ss << std::setprecision(10) << "DELETE FROM highlights where uuid='" << esc(uuid) << "';";

    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "delete_highlight",
        error_code,
        error_message);
}

bool DatabaseManager::update_mark(const std::string& document_path, char symbol, float offset_y, std::optional<float> offset_x, std::optional<float> zoom_level) {

    std::wstringstream ss;
    if (!offset_x.has_value()) {
        ss << "UPDATE marks set offset_y=" << offset_y << ", modification_time=CURRENT_TIMESTAMP where document_path='" << esc(document_path) << "' AND symbol='" << symbol << "';";
    }
    else {
        ss << "UPDATE marks set offset_y=" << offset_y << ", offset_x=" << offset_x.value() << ", zoom_level=" << zoom_level.value() << ", modification_time=CURRENT_TIMESTAMP where document_path='" << esc(document_path) << "' AND symbol='" << symbol << "';";
    }

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "update_mark",
        error_code,
        error_message);
}

bool DatabaseManager::set_actual_document_name(const std::string& checksum, const std::wstring& actual_name) {
    std::wstringstream ss;
    ss << "UPDATE opened_books set document_name='" << esc(actual_name) << "' where path='" << esc(checksum) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "set_actual_document_name",
        error_code,
        error_message);
}


bool DatabaseManager::select_opened_book(const std::string& book_path, std::vector<OpenedBookState>& out_result) {
    std::wstringstream ss;
    ss << "select zoom_level, offset_x, offset_y from opened_books where path='" << esc(book_path) << "'";
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), opened_book_callback, &out_result, &error_message);
    return handle_error(
        "select_opened_book",
        error_code,
        error_message);
}

//bool delete_mark_with_symbol(sqlite3* db, char symbol) {
//
//	std::wstringstream ss;
//	ss << "DELETE FROM marks where symbol='" << symbol << "';";
//	char* error_message = nullptr;
//
//	return handle_error(
//		sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message),
//		error_message);
//}

bool DatabaseManager::delete_opened_book(const std::string& book_path) {
    std::wstringstream ss;
    ss << "DELETE FROM opened_books where path='" << esc(book_path) << "'";
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "delete_opened_book",
        error_code,
        error_message);
}


bool DatabaseManager::select_opened_books_path_values(std::vector<std::wstring>& out_result) {
    std::wstringstream ss;
    ss << "SELECT path FROM opened_books order by datetime(last_access_time) desc;";
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), prev_doc_callback, &out_result, &error_message);
    return handle_error(
        "select_opened_books_path_values",
        error_code,
        error_message);
}

//bool DatabaseManager::select_opened_books_path_and_doc_names(std::vector<std::pair<std::wstring, std::wstring>>& out_result) {
//    std::wstringstream ss;
//    ss << "SELECT path, document_name FROM opened_books order by datetime(last_access_time) desc;";
//    char* error_message = nullptr;
//    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), prev_doc_with_name_callback, &out_result, &error_message);
//    return handle_error(
//        "select_opened_books_path_and_doc_names",
//        error_code,
//        error_message);
//}

bool DatabaseManager::generic_prepared_statement_run(sqlite3* db, sqlite3_stmt** stmt, const std::string& query, std::function<void()> on_init, std::function<void()> bind_params, std::function<void()> on_row) {

    if (*stmt == nullptr) {
        auto is_ok = sqlite3_prepare_v2(db, query.c_str(), query.size(), stmt, nullptr);
        if (!(is_ok == SQLITE_OK)) {
            qDebug() << QString::fromUtf8(sqlite3_errmsg(db));
            return false;
            //return handle_error("select_opened_books", is_ok, sqlite3_errmsg(global_db));
        }
        on_init();
    }

    bind_params();
    while (true) {
        auto state = sqlite3_step(*stmt);
        if (state != SQLITE_ROW) {
            break;
        }
        on_row();

    }
    sqlite3_reset(*stmt);
    return true;
}

bool DatabaseManager::select_opened_books(std::vector<OpenedBookInfo>& out_result) {
    std::string query = "SELECT path, document_name, last_access_time FROM opened_books ORDER BY datetime(last_access_time) DESC;"; //std::wstringstream ss;
    return generic_prepared_statement_run(global_db, &select_opened_books_stmt, query,
        []() {}, []() {},
        [&]() {
            OpenedBookInfo row;

            row.checksum = column_std_string(select_opened_books_stmt, 0);
            row.document_title = column_qstring(select_opened_books_stmt, 1);
            row.last_access_time = QDateTime::fromString(column_qstring(select_opened_books_stmt, 2), Qt::ISODate);

            out_result.push_back(row);
        });

    //if (select_opened_books_stmt == nullptr) {
    //    auto is_ok = sqlite3_prepare_v2(global_db, query.c_str(), query.size(), &select_opened_books_stmt, nullptr);
    //    if (!(is_ok == SQLITE_OK)) {
    //        return false;
    //        //return handle_error("select_opened_books", is_ok, sqlite3_errmsg(global_db));
    //    }
    //}

    //while (true) {
    //    auto state = sqlite3_step(select_opened_books_stmt);
    //    if (state != SQLITE_ROW) {
    //        break;
    //    }
    //    OpenedBookInfo row;

    //    const unsigned char* checksum_utf8 = sqlite3_column_text(select_opened_books_stmt, 0);
    //    const unsigned char* document_name_utf8 = sqlite3_column_text(select_opened_books_stmt, 1);
    //    const unsigned char* last_access_time_utf8 = sqlite3_column_text(select_opened_books_stmt, 2);

    //    row.checksum = QString::fromUtf8(checksum_utf8).toStdString();
    //    row.document_title = QString::fromUtf8(document_name_utf8);
    //    row.last_access_time = QDateTime::fromString(QString::fromUtf8(last_access_time_utf8));
    //    out_result.push_back(row);

    //}
    //return true;



    //ss << "SELECT path, document_name, last_access_time FROM opened_books ORDER BY datetime(last_access_time) DESC;";
    //char* error_message = nullptr;
    //int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), opened_books_callback, &out_result, &error_message);
    //return handle_error(
    //    "select_opened_books",
    //    error_code,
    //    error_message);
}

std::optional<std::string> DatabaseManager::get_document_last_access_time(const std::string& checksum) {
    std::wstringstream ss;
    ss << "SELECT last_access_time FROM opened_books WHERE path='" << esc(checksum) << "';";
    std::vector<std::string> out_result;
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), string_select_callback, &out_result, &error_message);
    bool res = handle_error(
        "get_document_last_access_time",
        error_code,
        error_message);
    if (res && out_result.size() > 0) {
        return out_result[0];
    }
    else {
        return {};
    }

}

bool DatabaseManager::insert_update_time(const std::string& checksum){
    std::wstringstream ss;
    ss << "INSERT OR REPLACE INTO update_times(checksum, last_update_time) VALUES ('" <<
        esc(checksum) << "', CURRENT_TIMESTAMP);";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "insert_update_time",
        error_code,
        error_message);
}

std::optional<std::string> DatabaseManager::get_update_time(const std::string& checksum) {
    std::wstringstream ss;
    ss << "SELECT last_update_time FROM update_times WHERE checksum='" << esc(checksum) << "';";

    std::vector<std::string> res;
    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), string_select_callback, &res, &error_message);
    bool success = handle_error(
        "get_update_time",
        error_code,
        error_message);
    if (success) {
        if (res.size() > 0) {
            return res[0];
        }
    }
    return {};
}

//bool DatabaseManager::select_opened_books_hashes_and_names(std::vector<std::pair<std::wstring, std::wstring>> &out_result) {
//	std::vector<std::wstring> hashes;
//	select_opened_books_path_values(hashes);
//	for (const auto& hash : hashes) {
//		std::vector<std::wstring> paths;
//		get_path_from_hash(utf8_encode(hash), paths);
//		if (paths.size() > 0) {
//			out_result.push_back(std::make_pair(hash, paths.back()));
//		}
//	}
//	return true;
//	//this->select_
//	//this->sele
//		//std::wstringstream ss;
//		//ss << "SELECT opened_books.path, document_hash.path FROM opened_books, document_hash where opened_books.path=document_hash.hash order by datetime(opened_books.last_access_time) desc;";
//		//char* error_message = nullptr;
//		//int error_code = sqlite3_exec(db, utf8_encode(ss.str()).c_str(), wstring_pair_select_callback, &out_result, &error_message);
//		//return handle_error(
//		//	error_code,
//		//	error_message);
//}

bool DatabaseManager::select_mark(const std::string& book_path, std::vector<Mark>& out_result) {
    std::wstringstream ss;
    ss << "select symbol, offset_y, offset_x, zoom_level, uuid, creation_time, modification_time, is_synced from marks where document_path='" << esc(book_path) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), mark_select_callback, &out_result, &error_message);
    return handle_error(
        "select_mark",
        error_code,
        error_message);
}

bool DatabaseManager::select_global_mark(char symbol, std::vector<std::pair<std::string, float>>& out_result) {
    std::wstringstream ss;
    ss << "select document_path, offset_y from marks where symbol='" << symbol << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), global_mark_select_callback, &out_result, &error_message);
    return handle_error(
        "select_global_mark",
        error_code,
        error_message);
}

bool DatabaseManager::select_bookmark(const std::string& book_path, std::vector<BookMark>& out_result) {
    std::wstringstream ss;
    ss << "select desc, offset_y, begin_x, begin_y, end_x, end_y, color_red, color_green, color_blue, font_size, font_face, uuid, creation_time, modification_time, is_synced from bookmarks where document_path='" << esc(book_path) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), bookmark_select_callback, &out_result, &error_message);
    return handle_error(
        "select_bookmark",
        error_code,
        error_message);
}

bool DatabaseManager::get_path_from_hash(const std::string& checksum, std::vector<std::wstring>& out_paths) {
    std::wstringstream ss;
    ss << "select path from document_hash where hash='" << esc(checksum) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_select_callback, &out_paths, &error_message);
    return handle_error(
        "get_path_from_hash",
        error_code,
        error_message);
}

bool DatabaseManager::get_hash_from_path(const std::string& path, std::vector<std::wstring>& out_checksum) {
    std::wstringstream ss;
    ss << "select hash from document_hash where path='" << esc(path) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_select_callback, &out_checksum, &error_message);
    return handle_error(
        "get_hash_from_path",
        error_code,
        error_message);
}

bool DatabaseManager::get_prev_path_hash_pairs(std::vector<std::pair<std::wstring, std::wstring>>& out_pairs) {
    std::wstringstream ss;
    ss << "select path, hash from document_hash;";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(ss.str()).c_str(), wstring_pair_select_callback, &out_pairs, &error_message);
    return handle_error(
        "get_prev_path_hash_pairs",
        error_code,
        error_message);
}

bool DatabaseManager::get_all_local_checksums(std::vector<std::string>& out_checksum) {
    std::wstringstream ss;
    ss << "SELECT DISTINCT path FROM opened_books;";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), string_select_callback, &out_checksum, &error_message);
    return handle_error(
        "get_all_local_checksums",
        error_code,
        error_message);
}

bool DatabaseManager::select_highlight(const std::string& book_path, std::vector<Highlight>& out_result) {
    std::wstringstream ss;
    ss << "select desc, text_annot, begin_x, begin_y, end_x, end_y, type, uuid, creation_time, modification_time, is_synced from highlights where document_path='" << esc(book_path) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), highlight_select_callback, &out_result, &error_message);
    return handle_error(
        "select_highlight",
        error_code,
        error_message);
}

bool DatabaseManager::select_highlight_with_type(const std::string& book_path, char type, std::vector<Highlight>& out_result) {
    std::wstringstream ss;
    ss << "select desc, begin_x, begin_y, end_x, end_y, type, uuid, creation_time, modification_time, is_synced from highlights where document_path='" << esc(book_path) << "' AND type='" << type << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), highlight_select_callback, &out_result, &error_message);
    return handle_error(
        "select_highlight_with_type",
        error_code,
        error_message);
}

bool DatabaseManager::global_select_highlight(std::vector<std::pair<std::string, Highlight>>& out_result) {
    std::wstringstream ss;
    ss << "select document_path, desc, text_annot, type, begin_x, begin_y, end_x, end_y, uuid, creation_time, modification_time, is_synced from highlights;";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), global_highlight_select_callback, &out_result, &error_message);
    return handle_error(
        "global_select_highlight",
        error_code,
        error_message);
}

bool DatabaseManager::global_select_bookmark(std::vector<std::pair<std::string, BookMark>>& out_result) {
    std::wstringstream ss;
    ss << "select document_path, desc, offset_y, begin_x, begin_y, end_x, end_y, uuid, creation_time, modification_time, is_synced from bookmarks;";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), global_bookmark_select_callback, &out_result, &error_message);
    return handle_error(
        "global_select_bookmark",
        error_code,
        error_message);
}

bool DatabaseManager::select_links(const std::string& src_document_path, std::vector<Portal>& out_result) {
    std::string query = "select dst_document, src_offset_y, src_offset_x, dst_offset_x, dst_offset_y, dst_zoom_level, uuid, creation_time, modification_time, is_synced from links where src_document =:src_document;";
    return generic_prepared_statement_run(global_db, &select_document_portals_stmt, query,
        [&]() { // on init
            select_document_portals_stmt_src_document_index = sqlite3_bind_parameter_index(select_document_portals_stmt, ":src_document");
        },
        [&]() { // bind params
            sqlite3_bind_text(
                select_document_portals_stmt,
                select_document_portals_stmt_src_document_index,
                src_document_path.c_str(),
                src_document_path.size(),
                nullptr
            );
        },
        [&]() { // on row
            Portal row;

            row.dst.document_checksum = column_std_string(select_document_portals_stmt, 0);
            row.src_offset_y = sqlite3_column_double(select_document_portals_stmt, 1);
            if (sqlite3_column_type(select_document_portals_stmt, 2) != SQLITE_NULL) {
                row.src_offset_x = sqlite3_column_double(select_document_portals_stmt, 2);
            }
            row.dst.book_state.offset_x = sqlite3_column_double(select_document_portals_stmt, 3);
            row.dst.book_state.offset_y = sqlite3_column_double(select_document_portals_stmt, 4);
            row.dst.book_state.zoom_level = sqlite3_column_double(select_document_portals_stmt, 5);
            row.uuid = column_std_string(select_document_portals_stmt, 6);
            row.creation_time = column_std_string(select_document_portals_stmt, 7);
            row.modification_time = column_std_string(select_document_portals_stmt, 8);
            row.is_synced = sqlite3_column_int(select_document_portals_stmt, 9);
            out_result.push_back(row);

        });

    //std::wstringstream ss;
    //ss << "select dst_document, src_offset_y, src_offset_x, dst_offset_x, dst_offset_y, dst_zoom_level, uuid, creation_time, modification_time, is_synced from links where src_document='" << esc(src_document_path) << "';";

    //char* error_message = nullptr;
    //int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), link_select_callback, &out_result, &error_message);
    //return handle_error(
    //    "select_links",
    //    error_code,
    //    error_message);
}

void DatabaseManager::create_tables() {
    create_opened_books_table();
    create_marks_table();
    create_bookmarks_table();
    create_highlights_table();
    create_links_table();
    create_document_hash_table();
    create_server_update_time_table();
    create_unsynced_deletions_table();
    //create_unsynced_additions_table();
    if (!has_column("highlights", "is_synced")) {
        add_synced_columns();
    }

    if (!has_column("opened_books", "is_synced")) {
        add_document_sync_columns();
    }
}

void DatabaseManager::add_synced_columns() {
    const char* add_sync_to_highlights_sql = "ALTER TABLE highlights ADD COLUMN is_synced BOOLEAN DEFAULT 0";
    const char* add_sync_to_bookmarks_sql = "ALTER TABLE bookmarks ADD COLUMN is_synced BOOLEAN DEFAULT 0";
    const char* add_sync_to_marks_sql = "ALTER TABLE marks ADD COLUMN is_synced BOOLEAN DEFAULT 0";
    const char* add_sync_to_portals_sql = "ALTER TABLE links ADD COLUMN is_synced BOOLEAN DEFAULT 0";

    char* error_message = nullptr;
    sqlite3_exec(global_db, add_sync_to_highlights_sql, null_callback, 0, &error_message);
    sqlite3_exec(global_db, add_sync_to_bookmarks_sql, null_callback, 0, &error_message);
    sqlite3_exec(global_db, add_sync_to_marks_sql, null_callback, 0, &error_message);
    sqlite3_exec(global_db, add_sync_to_portals_sql, null_callback, 0, &error_message);
}

void DatabaseManager::add_document_sync_columns() {

    char* error_message = nullptr;
    const char* add_sync_to_portals_sql = "ALTER TABLE opened_books ADD COLUMN is_synced BOOLEAN DEFAULT 0";
    sqlite3_exec(global_db, add_sync_to_portals_sql, null_callback, 0, &error_message);
}

bool update_string_value(sqlite3* db,
    const std::wstring& table_name,
    const std::wstring& field_name,
    const std::wstring& old_value,
    const std::wstring& new_value) {

    std::wstringstream ss;
    ss << "UPDATE " << table_name << " set " << field_name << "='" << esc(new_value) << "' where " << field_name << "='" << esc(old_value) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "update_string_value",
        error_code,
        error_message);

}
bool update_mark_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
    return update_string_value(db, L"marks", L"document_path", path, new_path);
}


bool update_opened_book_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
    return update_string_value(db, L"opened_books", L"path", path, new_path);
}

bool update_bookmark_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
    return update_string_value(db, L"bookmarks", L"document_path", path, new_path);
}

bool update_highlight_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
    return update_string_value(db, L"highlights", L"document_path", path, new_path);
}

bool update_portal_path(sqlite3* db, const std::wstring& path, const std::wstring& new_path) {
    return update_string_value(db, L"links", L"src_document", path, new_path) && update_string_value(db, L"links", L"dst_document", path, new_path);
}

void DatabaseManager::upgrade_database_hashes() {
    CachedChecksummer checksummer({});

    std::vector<std::wstring> prev_doc_paths;
    select_opened_books_path_values(prev_doc_paths);

    for (const auto& doc_path : prev_doc_paths) {
        std::string checksum = checksummer.get_checksum(doc_path);
        if (checksum.size() > 0) {
            std::wstring uchecksum = utf8_decode(checksum);
            insert_document_hash(doc_path, checksum);

            //update_mark_path(local_db, doc_path, uchecksum);
            //update_bookmark_path(local_db, doc_path, uchecksum);
            //update_highlight_path(local_db, doc_path, uchecksum);
            //update_portal_path(local_db, doc_path, uchecksum);
            //update_opened_book_path(local_db, doc_path, uchecksum);
        }
    }
}

void DatabaseManager::split_database(const std::wstring& local_database_path, const std::wstring& global_database_path, bool was_using_hashes) {

    //we should only split when we have the same local and global database
    assert(local_db == global_db);

    // ---------------------- EXPORT PREVIOUS DATABASE ----------------------------
    std::vector<std::pair<std::wstring, std::wstring>> path_hash;
    get_prev_path_hash_pairs(path_hash);
    std::vector<std::pair<std::string, OpenedBookState>> opened_book_states;
    std::vector<std::pair<std::string, Mark>> marks;
    std::vector<std::pair<std::string, BookMark>> bookmarks;
    std::vector<std::pair<std::string, Highlight>> highlights;
    std::vector<std::pair<std::string, Portal>> portals;

    std::unordered_map<std::wstring, std::wstring> path_to_hash;

    for (const auto& [path, hash] : path_hash) {
        path_to_hash[path] = hash;
    }

    for (const auto& [path, hash_] : path_hash) {
        std::string hash = utf8_encode(hash_);
        std::string key;
        if (was_using_hashes) {
            key = hash;
        }
        else {
            key = utf8_encode(path);
        }
        std::vector<OpenedBookState> current_book_state;
        std::vector<Mark> current_marks;
        std::vector<BookMark> current_bookmarks;
        std::vector<Highlight> current_highlights;
        std::vector<Portal> current_portals;

        select_opened_book(key, current_book_state);
        if (current_book_state.size() > 0) {
            opened_book_states.push_back(std::make_pair(hash, current_book_state[0]));
        }

        select_mark(key, current_marks);
        for (const auto& mark : current_marks) {
            marks.push_back(std::make_pair(hash, mark));
        }

        select_bookmark(key, current_bookmarks);
        for (const auto& bookmark : current_bookmarks) {
            bookmarks.push_back(std::make_pair(hash, bookmark));
        }

        select_highlight(key, current_highlights);
        for (const auto& highlight : current_highlights) {
            highlights.push_back(std::make_pair(hash, highlight));
        }

        select_links(key, current_portals);
        for (auto portal : current_portals) {
            if (!was_using_hashes) {
                if (path_to_hash.find(utf8_decode(portal.dst.document_checksum)) == path_to_hash.end()) {
                    continue;
                }
                else {
                    portal.dst.document_checksum = utf8_encode(path_to_hash[utf8_decode(portal.dst.document_checksum)]);
                }
            }

            portals.push_back(std::make_pair(hash, portal));
        }
    }
    sqlite3_close(local_db);


    // ---------------------- CREATE NEW DATABASE FILES ----------------------------

    if (!open(local_database_path, global_database_path)) {
        return;
    }

    // ---------------------- IMPORT PREVIOUS DATA ----------------------------
    create_tables();
    for (const auto& [path, hash] : path_hash) {
        insert_document_hash(path, utf8_encode(hash));
    }

    for (const auto& [hash, book_state] : opened_book_states) {
        update_book(hash, false, book_state.zoom_level, book_state.offset_x, book_state.offset_y);
    }
    for (const auto& [hash, mark] : marks) {
        insert_mark(hash, mark.symbol, mark.y_offset, new_uuid());
    }
    for (const auto& [hash, bookmark] : bookmarks) {
        insert_bookmark(hash, bookmark.description, bookmark.y_offset_, new_uuid());
    }
    for (const auto& [hash, highlight] : highlights) {
        insert_highlight(
            hash,
            highlight.description,
            highlight.selection_begin.x,
            highlight.selection_begin.y,
            highlight.selection_end.x,
            highlight.selection_end.y,
            highlight.type,
            new_uuid()
        );
    }
    for (const auto& [hash, portal] : portals) {
        insert_portal(
            hash,
            portal.dst.document_checksum,
            portal.dst.book_state.offset_x,
            portal.dst.book_state.offset_y,
            portal.dst.book_state.zoom_level,
            portal.src_offset_y,
            new_uuid()
        );
    }
    set_version();

}





void DatabaseManager::export_json(std::wstring json_file_path, CachedChecksummer* checksummer) {

    std::set<std::string> seen_checksums;

    std::vector<std::wstring> prev_doc_checksums;

    select_opened_books_path_values(prev_doc_checksums);

    QJsonArray document_data_array;

    for (size_t i = 0; i < prev_doc_checksums.size(); i++) {

        const auto& document_checksum = utf8_encode(prev_doc_checksums[i]);
        std::optional<std::wstring> path = checksummer->get_path(document_checksum);

        if ((!path) || (document_checksum.size() == 0) || (seen_checksums.find(document_checksum) != seen_checksums.end())) {
            continue;
        }

        std::vector<BookMark> bookmarks;
        std::vector<Highlight> highlights;
        std::vector<Mark> marks;
        std::vector<Portal> portals;
        std::vector<OpenedBookState> opened_book_state_;

        select_opened_book(document_checksum, opened_book_state_);
        if (opened_book_state_.size() != 1) {
            continue;
        }

        OpenedBookState opened_book_state = opened_book_state_[0];

        select_bookmark(document_checksum, bookmarks);
        select_mark(document_checksum, marks);
        select_highlight(document_checksum, highlights);
        select_links(document_checksum, portals);


        QJsonArray json_bookmarks = export_array(bookmarks, utf8_encode(prev_doc_checksums[i]));
        QJsonArray json_highlights = export_array(highlights, utf8_encode(prev_doc_checksums[i]));
        QJsonArray json_marks = export_array(marks, utf8_encode(prev_doc_checksums[i]));
        QJsonArray json_portals = export_array(portals, utf8_encode(prev_doc_checksums[i]));

        QJsonObject book_object;
        book_object["offset_x"] = opened_book_state.offset_x;
        book_object["offset_y"] = opened_book_state.offset_y;
        book_object["zoom_level"] = opened_book_state.zoom_level;
        book_object["checksum"] = QString::fromStdString(document_checksum);
        book_object["path"] = QString::fromStdWString(path.value());
        book_object["bookmarks"] = json_bookmarks;
        book_object["marks"] = json_marks;
        book_object["highlights"] = json_highlights;
        book_object["portals"] = json_portals;

        document_data_array.append(std::move(book_object));

        seen_checksums.insert(document_checksum);
    }

    QJsonObject exported_json;
    exported_json["documents"] = std::move(document_data_array);

    QJsonDocument json_document(exported_json);


    QFile output_file(QString::fromStdWString(json_file_path));
    output_file.open(QFile::WriteOnly);
    output_file.write(json_document.toJson());
    output_file.close();
}

template<typename T>
std::vector<T> get_new_elements(const std::vector<T>& prev_elements, const std::vector<T>& new_elements) {
    std::vector<T> res;
    for (const auto& new_elem : new_elements) {
        bool is_new = true;
        for (const auto& prev_elem : prev_elements) {
            if (new_elem == prev_elem) {
                is_new = false;
                break;
            }
        }
        if (is_new) {
            res.push_back(new_elem);
        }
    }
    return res;
}

void DatabaseManager::import_json(std::wstring json_file_path, CachedChecksummer* checksummer) {

    QFile json_file(QString::fromStdWString(json_file_path));
    json_file.open(QFile::ReadOnly);
    QJsonDocument json_document = QJsonDocument().fromJson(json_file.readAll());
    json_file.close();

    QJsonObject imported_json = json_document.object();
    QJsonArray documents_json_array = imported_json.value("documents").toArray();

    //std::vector<JsonDocumentData> imported_documents;

    for (int i = 0; i < documents_json_array.size(); i++) {

        QJsonObject current_json_doc = documents_json_array.at(i).toObject();

        std::string checksum = current_json_doc["checksum"].toString().toStdString();
        //std::wstring path = current_json_doc["path"].toString().toStdWString();
        float offset_x = current_json_doc["offset_x"].toDouble();
        float offset_y = current_json_doc["offset_y"].toDouble();
        float zoom_level = current_json_doc["zoom_level"].toDouble();

        auto bookmarks = std::move(load_from_json_array<BookMark>(current_json_doc["bookmarks"].toArray()));
        auto marks = load_from_json_array<Mark>(current_json_doc["marks"].toArray());
        auto highlights = load_from_json_array<Highlight>(current_json_doc["highlights"].toArray());
        auto portals = load_from_json_array<Portal>(current_json_doc["portals"].toArray());

        std::vector<BookMark> prev_bookmarks;
        std::vector<Mark> prev_marks;
        std::vector<Highlight> prev_highlights;
        std::vector<Portal> prev_portals;


        select_bookmark(checksum, prev_bookmarks);
        select_mark(checksum, prev_marks);
        select_highlight(checksum, prev_highlights);
        select_links(checksum, prev_portals);

        std::vector<BookMark> new_bookmarks = get_new_elements(prev_bookmarks, bookmarks);
        std::vector<Mark> new_marks = get_new_elements(prev_marks, marks);
        std::vector<Highlight> new_highlights = get_new_elements(prev_highlights, highlights);
        std::vector<Portal> new_portals = get_new_elements(prev_portals, portals);

        std::optional<std::wstring> path = checksummer->get_path(checksum);

        if (path) {
            //update_book(db, path.value(), zoom_level, offset_x, offset_y);
            update_book(checksum, false, zoom_level, offset_x, offset_y);
        }

        for (const auto& bm : new_bookmarks) {
            insert_bookmark(checksum, bm.description, bm.y_offset_, utf8_decode(bm.uuid));
        }

        for (const auto& mark : new_marks) {
            insert_mark(checksum, mark.symbol, mark.y_offset, utf8_decode(mark.uuid), mark.x_offset, mark.zoom_level);
        }

        for (const auto& hl : new_highlights) {
            insert_highlight(checksum,
                hl.description,
                hl.selection_begin.x,
                hl.selection_begin.y,
                hl.selection_end.x,
                hl.selection_end.y,
                hl.type,
                utf8_decode(hl.uuid));
        }
        for (const auto& portal : new_portals) {
            insert_portal(checksum,
                portal.dst.document_checksum,
                portal.dst.book_state.offset_x,
                portal.dst.book_state.offset_y,
                portal.dst.book_state.zoom_level,
                portal.src_offset_y,
                utf8_decode(portal.uuid));
        }

    }
}

std::string create_select_query(std::string table_name,
    std::vector<std::string> selections,
    std::unordered_map<std::string, std::variant<std::wstring, std::string, int, char, float>> values) {
    std::wstringstream ss;

    ss << L"SELECT ";
    for (size_t i = 0; i < selections.size(); i++) {
        ss << utf8_decode(selections[i]);
        if ((size_t)i < (selections.size() - 1)) {
            ss << ", ";
        }
    }
    ss << " FROM " << utf8_decode(table_name) << " WHERE ";

    int index = 0;
    for (const auto& [key, value] : values) {

        std::wstring ukey = utf8_decode(key);

        if (std::holds_alternative<std::wstring>(value)) {
            ss << ukey << L"='" << esc(std::get<std::wstring>(value)) << L"'";
        }

        if (std::holds_alternative<std::string>(value)) {
            ss << ukey << L"='" << esc(std::get<std::string>(value)) << L"'";
        }

        if (std::holds_alternative<char>(value)) {
            ss << ukey << L"='" << std::get<char>(value) << L"'";
        }

        if (std::holds_alternative<int>(value)) {
            ss << ukey << L"=" << std::get<int>(value) << L"";
        }

        if (std::holds_alternative<float>(value)) {
            ss << ukey << L"=" << std::get<float>(value) << L"";
        }

        index++;
        if ((size_t)index != values.size()) {
            ss << ", ";
        }
    }
    ss << L";";
    return utf8_encode(ss.str());
}

void DatabaseManager::ensure_database_compatibility(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path) {
    create_tables();

    // if the database is still using absolute paths instead of checksums, update all paths to checksums
    std::vector<std::pair<std::wstring, std::wstring>> prev_path_hash_pairs;
    get_prev_path_hash_pairs(prev_path_hash_pairs);
    bool was_using_hashes = true;

    if (prev_path_hash_pairs.size() == 0) {
        was_using_hashes = false;
        upgrade_database_hashes();
    }

    //if we are still using a single database file instead of separate local and global database files, split the database.
    if (local_db == global_db) {
        split_database(local_db_file_path, global_db_file_path, was_using_hashes);
    }
}

int DatabaseManager::get_version() {
    char* error_message = nullptr;

    int version = -1;
    int error_code = sqlite3_exec(global_db, "PRAGMA user_version;", version_callback, &version, &error_message);
    handle_error("get_version", error_code, error_message);
    return version;
}

int DatabaseManager::set_version() {
    char* error_message = nullptr;

    std::string query = QString("PRAGMA user_version = %1;").arg(DATABASE_VERSION).toStdString();
    int error_code = sqlite3_exec(global_db, query.c_str(), null_callback, nullptr, &error_message);
    return handle_error("set_version", error_code, error_message);
}

void DatabaseManager::ensure_schema_compatibility() {
    int database_file_version = get_version();
    std::vector<std::function<void()>> migrations;
    migrations.push_back([this]() { migrate_version_0_to_1(); });
    migrations.push_back([this]() { migrate_version_1_to_2(); });

    assert(migrations.size() == DATABASE_VERSION);

    if (database_file_version != DATABASE_VERSION) {
        if (database_file_version >= migrations.size()) {
            qDebug() << "Error: Invalid database version";
            return;
        }

        for (int i = database_file_version; i < DATABASE_VERSION; i++) {
            migrations[i]();
        }

        set_version();
    }
}

bool DatabaseManager::run_schema_query(sqlite3* db, const char* query) {
    char* error_message = nullptr;
    int error_code = sqlite3_exec(db, query, null_callback, 0, &error_message);
    return handle_error("run_schema_query", error_code, error_message);
}

void DatabaseManager::migrate_version_1_to_2() {
    qDebug() << "Migrating database from version 1 to 2";

    std::vector<std::string> queries_to_run;

    queries_to_run.push_back("ALTER TABLE marks ADD COLUMN offset_x real;");
    queries_to_run.push_back("ALTER TABLE marks ADD COLUMN zoom_level real;");
    queries_to_run.push_back("ALTER TABLE opened_books ADD COLUMN document_name TEXT;");
    queries_to_run.push_back("ALTER TABLE opened_books ADD COLUMN is_synced BOOLEAN DEFAULT 0;");

    queries_to_run.push_back("ALTER TABLE highlights ADD COLUMN is_synced BOOLEAN DEFAULT 0;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN is_synced BOOLEAN DEFAULT 0;");
    queries_to_run.push_back("ALTER TABLE marks ADD COLUMN is_synced BOOLEAN DEFAULT 0;");
    queries_to_run.push_back("ALTER TABLE links ADD COLUMN is_synced BOOLEAN DEFAULT 0;");

    std::string transaction = "BEGIN TRANSACTION;\n";
    for (auto q : queries_to_run) {
        transaction += q + "\n";
    }

    transaction += "COMMIT;";

    if (!run_schema_query(global_db, transaction.c_str())) {
        qDebug() << "Error: Could not migrate database from version 1 to version 2, rolling back ...";
        run_schema_query(global_db, "ROLLBACK;");
    }
}

void DatabaseManager::migrate_version_0_to_1() {
    qDebug() << "Migrating database from version 0 to 1";

    std::vector<std::string> queries_to_run;

    std::vector<int> all_mark_ids;
    std::vector<int> all_bookmark_ids;
    std::vector<int> all_highlight_ids;
    std::vector<int> all_portal_ids;

    select_all_mark_ids(all_mark_ids);
    select_all_bookmark_ids(all_bookmark_ids);
    select_all_highlight_ids(all_highlight_ids);
    select_all_portal_ids(all_portal_ids);

    // add box columns to bookmarks table
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN begin_x real DEFAULT -1;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN begin_y real DEFAULT -1;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN end_x real DEFAULT -1;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN end_y real DEFAULT -1;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN font_size integer DEFAULT -1;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN color_red real DEFAULT 0;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN color_green real DEFAULT 0;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN color_blue real DEFAULT 0;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN font_face TEXT;");

    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN creation_time timestamp;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN modification_time timestamp;");
    queries_to_run.push_back("ALTER TABLE bookmarks ADD COLUMN uuid TEXT;");

    //add text annotation column to highlight table
    queries_to_run.push_back("ALTER TABLE highlights ADD COLUMN text_annot TEXT;");
    queries_to_run.push_back("ALTER TABLE highlights ADD COLUMN creation_time timestamp;");
    queries_to_run.push_back("ALTER TABLE highlights ADD COLUMN modification_time timestamp;");
    queries_to_run.push_back("ALTER TABLE highlights ADD COLUMN uuid TEXT;");

    // marks
    queries_to_run.push_back("ALTER TABLE marks ADD COLUMN creation_time timestamp;");
    queries_to_run.push_back("ALTER TABLE marks ADD COLUMN modification_time timestamp;");
    queries_to_run.push_back("ALTER TABLE marks ADD COLUMN uuid TEXT;");

    // portals
    queries_to_run.push_back("ALTER TABLE links ADD COLUMN src_offset_x REAL;");
    queries_to_run.push_back("ALTER TABLE links ADD COLUMN creation_time timestamp;");
    queries_to_run.push_back("ALTER TABLE links ADD COLUMN modification_time timestamp;");
    queries_to_run.push_back("ALTER TABLE links ADD COLUMN uuid TEXT;");

    queries_to_run.push_back("UPDATE marks set creation_time=CURRENT_TIMESTAMP;");
    queries_to_run.push_back("UPDATE bookmarks set creation_time=CURRENT_TIMESTAMP;");
    queries_to_run.push_back("UPDATE highlights set creation_time=CURRENT_TIMESTAMP;");
    queries_to_run.push_back("UPDATE links set creation_time=CURRENT_TIMESTAMP;");

    queries_to_run.push_back("UPDATE marks set modification_time=CURRENT_TIMESTAMP;");
    queries_to_run.push_back("UPDATE bookmarks set modification_time=CURRENT_TIMESTAMP;");
    queries_to_run.push_back("UPDATE highlights set modification_time=CURRENT_TIMESTAMP;");
    queries_to_run.push_back("UPDATE links set modification_time=CURRENT_TIMESTAMP;");

    for (auto mark_id : all_mark_ids) {
        queries_to_run.push_back("UPDATE marks set uuid='" + QUuid::createUuid().toString().toStdString() + "' where id=" + std::to_string(mark_id) + ";");
    }

    for (auto bookmark_id : all_bookmark_ids) {
        queries_to_run.push_back("UPDATE bookmarks set uuid='" + QUuid::createUuid().toString().toStdString() + "' where id=" + std::to_string(bookmark_id) + ";");
    }

    for (auto highlight_id : all_highlight_ids) {
        queries_to_run.push_back("UPDATE highlights set uuid='" + QUuid::createUuid().toString().toStdString() + "' where id=" + std::to_string(highlight_id) + ";");
    }

    for (auto portal_id : all_portal_ids) {
        queries_to_run.push_back("UPDATE links set uuid='" + QUuid::createUuid().toString().toStdString() + "' where id=" + std::to_string(portal_id) + ";");
    }

    std::string transaction = "BEGIN TRANSACTION;\n";
    for (auto q : queries_to_run) {
        transaction += q + "\n";
    }

    transaction += "COMMIT;";

    if (!run_schema_query(global_db, transaction.c_str())) {
        qDebug() << "Error: Could not migrate database from version 0 to version 1, rolling back ...";
        run_schema_query(global_db, "ROLLBACK;");
    }

    //for (int i = 0; i < queries_to_run.size(); i++) {
    //	if (!run_schema_query(queries_to_run[i].c_str())) {
    //		qDebug() << "Error: could not migrate database";
    //	}
    //}

    //std::vector<std::pair<std::wstring, std::wstring>> path_hashes;
    //get_prev_path_hash_pairs(path_hashes);


}

bool DatabaseManager::select_all_mark_ids(std::vector<int>& mark_ids) {
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, "select id from marks", id_callback, &mark_ids, &error_message);
    return handle_error(
        "select_all_mark_ids",
        error_code,
        error_message);
}

bool DatabaseManager::select_all_bookmark_ids(std::vector<int>& bookmark_ids) {
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, "select id from bookmarks", id_callback, &bookmark_ids, &error_message);
    return handle_error(
        "select_all_bookmark_ids",
        error_code,
        error_message);
}

bool DatabaseManager::select_all_highlight_ids(std::vector<int>& highlight_ids) {
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, "select id from highlights", id_callback, &highlight_ids, &error_message);
    return handle_error(
        "select_all_highlight_ids",
        error_code,
        error_message);
}

bool DatabaseManager::select_all_portal_ids(std::vector<int>& portal_ids) {
    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, "select id from links", id_callback, &portal_ids, &error_message);
    return handle_error(
        "select_all_portal_ids",
        error_code,
        error_message);
}


bool DatabaseManager::update_highlight_with_server_highlight(const Highlight& server_highlight) {
    return generic_update_run_query("highlights",
        {
            {"uuid", QString::fromStdString(server_highlight.uuid)},
        },
        {
            {"text_annot",QString::fromStdWString(server_highlight.text_annot)},
            {"type", QChar(server_highlight.type)},
            {"modification_time", QString::fromStdString(server_highlight.modification_time)},
        });
}

bool DatabaseManager::update_bookmark_with_server_bookmark(const BookMark& server_bookmark) {
    return generic_update_run_query("bookmarks",
        {
            {"uuid", QString::fromStdString(server_bookmark.uuid)},
        },
        {
            {"desc",QString::fromStdWString(server_bookmark.description)},
            {"font_size", server_bookmark.font_size},
            {"font_face", QString::fromStdWString(server_bookmark.font_face)},
            {"color_red", server_bookmark.color[0]},
            {"color_green", server_bookmark.color[1]},
            {"color_blue", server_bookmark.color[2]},
            {"begin_x", server_bookmark.begin_x},
            {"begin_y", server_bookmark.begin_y},
            {"end_x", server_bookmark.end_x},
            {"end_y", server_bookmark.end_y},
            {"offset_y", server_bookmark.y_offset_},
            {"modification_time", QString::fromStdString(server_bookmark.modification_time)},
        });
}

bool DatabaseManager::update_portal_with_server_portal(const Portal& server_portal){

    std::string src_offset_x_string = server_portal.src_offset_x ? QString::number(server_portal.src_offset_x.value()).toStdString() : "NULL";
    std::wstringstream ss;
    ss << "UPDATE links set creation_time='" << esc(server_portal.creation_time) <<
        "', modification_time='" << esc(server_portal.modification_time) <<
        "', dst_document='" << esc(server_portal.dst.document_checksum) <<
        "', src_offset_x=" << esc(src_offset_x_string) <<
        ", src_offset_y=" << server_portal.src_offset_y << 
        ", dst_offset_x=" << server_portal.dst.book_state.offset_x << 
        ", dst_offset_y=" << server_portal.dst.book_state.offset_y << 
        ", dst_zoom_level=" << server_portal.dst.book_state.zoom_level << 
        ", is_synced=1" <<
        " where uuid='" << esc(server_portal.uuid) << "';";

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "update_portal_with_server_portal",
        error_code,
        error_message);
}

bool DatabaseManager::update_annot_with_server_annot(const Annotation* server_annot) {

    if (dynamic_cast<const Highlight*>(server_annot)) {
        return update_highlight_with_server_highlight(*dynamic_cast<const Highlight*>(server_annot));
    }

    if (dynamic_cast<const BookMark*>(server_annot)) {
        return update_bookmark_with_server_bookmark(*dynamic_cast<const BookMark*>(server_annot));
    }

    if (dynamic_cast<const Portal*>(server_annot)) {
        return update_portal_with_server_portal(*dynamic_cast<const Portal*>(server_annot));
    }

    return false;
}

bool DatabaseManager::update_highlight_add_annotation(const std::string& uuid, const std::wstring& text_annot) {

    return generic_update_run_query("highlights",
        {
            {"uuid", QString::fromStdString(uuid)},
        },
        {
            {"text_annot",QString::fromStdWString(text_annot)},
            {"modification_time", "CURRENT_TIMESTAMP"},
        });
}

bool DatabaseManager::update_highlight_type(const std::string& uuid, char new_type) {

    return generic_update_run_query("highlights",
        {
            {"uuid", QString::fromStdString(uuid)},
        },
        {
            {"type", QChar(new_type)},
            {"modification_time", "CURRENT_TIMESTAMP"},
        });
}

bool DatabaseManager::update_bookmark_change_text(const std::string& uuid, const std::wstring& new_text, float new_font_size) {
    return generic_update_run_query("bookmarks",
        {
            {"uuid", QString::fromStdString(uuid)},
        },
        {
            {"desc", QString::fromStdWString(new_text)},
            {"font_size", new_font_size},
            {"modification_time", "CURRENT_TIMESTAMP"},
        });
}
bool DatabaseManager::update_bookmark_change_position(const std::string& uuid, AbsoluteDocumentPos new_begin, AbsoluteDocumentPos new_end) {
    std::wstringstream ss;

    return generic_update_run_query("bookmarks",
        {
            {"uuid", QString::fromStdString(uuid)},
        },
        {
            {"offset_y", new_begin.y},
            {"begin_x", new_begin.x},
            {"begin_y", new_begin.y},
            {"end_x", new_end.x},
            {"end_y", new_end.y},
            {"modification_time", "CURRENT_TIMESTAMP"},
        });
}

bool DatabaseManager::update_portal_change_src_position(const std::string& uuid, AbsoluteDocumentPos new_pos){
    std::wstringstream ss;

    return generic_update_run_query("links",
        {
            {"uuid", QString::fromStdString(uuid)},
        },
        {
            {"src_offset_x", new_pos.x},
            {"src_offset_y", new_pos.y},
            {"modification_time", "CURRENT_TIMESTAMP"},
        });

}
std::wstring encode_variant(QVariant var) {


    std::vector<QString> specials = { "CURRENT_TIMESTAMP", "datetime('now')" };
    if ((var.type() == QVariant::String) || (var.type() == QVariant::Char)) {
        if (std::find(specials.begin(), specials.end(), var.toString()) != specials.end()) {
            return var.toString().toStdWString();
        }
        return (L"'" + esc(var.toString().toStdWString()) + L"'");

    }
    else {
        return var.toString().toStdWString();
    }
}

bool DatabaseManager::generic_update_run_query(std::string table_name,
    std::vector<std::pair<QString, QVariant>> selections,
    std::vector<std::pair<QString, QVariant>> updated_values) {
    std::wstring query = generic_update_create_query(table_name, selections, updated_values);

    char* error_message = nullptr;
    int error_code = sqlite3_exec(global_db, utf8_encode(query).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "genetic_update_run_query",
        error_code,
        error_message);
}

bool DatabaseManager::generic_insert_run_query(std::string table_name,
    std::vector<std::pair<QString, QVariant>> values,
    sqlite3* db) {
    std::wstring query = generic_insert_create_query(table_name, values);

    char* error_message = nullptr;

    if (db == nullptr) {
        db = global_db;
    }

    int error_code = sqlite3_exec(db, utf8_encode(query).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "generic_insert_run_query",
        error_code,
        error_message);
}

std::wstring DatabaseManager::generic_insert_create_query(std::string table_name,
    std::vector<std::pair<QString, QVariant>> values) {
    std::wstringstream query;
    query << "INSERT INTO " << esc(table_name) << " ( ";

    for (int i = 0; i < values.size(); i++) {
        auto [column_name, _] = values[i];

        query << column_name.toStdWString();

        if (i < values.size() - 1) {
            query << L", ";
        }
    }
    query << L" ) VALUES (";

    for (int i = 0; i < values.size(); i++) {
        auto [_, column_value] = values[i];

        query << encode_variant(column_value);

        if (i < values.size() - 1) {
            query << L", ";
        }
    }
    query << ");";

    return query.str();
}

bool DatabaseManager::update_file_name(std::wstring old_name, std::wstring new_name) {

    std::wstring query = generic_update_create_query("document_hash",
        { {"path", QString::fromStdWString(old_name)} },
        { {"path", QString::fromStdWString(new_name) } });

    char* error_message = nullptr;
    int error_code = sqlite3_exec(local_db, utf8_encode(query).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "update_file_name",
        error_code,
        error_message);
}

std::wstring DatabaseManager::generic_update_create_query(std::string table_name,
    std::vector<std::pair<QString, QVariant>> selections,
    std::vector<std::pair<QString, QVariant>> updated_values) {
    std::wstringstream query;
    query << "UPDATE " << esc(table_name) << " SET ";

    for (int i = 0; i < updated_values.size(); i++) {
        auto [column_name, column_value] = updated_values[i];

        query << column_name.toStdWString() << L" = ";
        query << encode_variant(column_value);

        if (i < updated_values.size() - 1) {
            query << L", ";
        }
    }
    query << L" WHERE ";

    for (int i = 0; i < selections.size(); i++) {
        auto [column_name, column_value] = selections[i];

        query << column_name.toStdWString() << L" = ";
        query << encode_variant(column_value);

        if (i < selections.size() - 1) {
            query << L", ";
        }
    }
    query << ";";

    return query.str();
}

std::string DatabaseManager::get_annot_table_name(Annotation* annot) {

    if (dynamic_cast<BookMark*>(annot)) return "bookmarks";
    if (dynamic_cast<Highlight*>(annot)) return "highlights";
    if (dynamic_cast<Mark*>(annot)) return "marks";
    if (dynamic_cast<Portal*>(annot)) return "links";
    return "";
}

bool DatabaseManager::insert_annotation(Annotation* annot, std::string document_hash) {
    auto fields = annot->to_tuples();
    if (dynamic_cast<Portal*>(annot)) {
        fields.push_back({ "src_document", QString::fromStdString(document_hash) });
    }
    else {
        fields.push_back({ "document_path", QString::fromStdString(document_hash) });
    }
    return generic_insert_run_query(get_annot_table_name(annot), fields);
}

bool DatabaseManager::update_annotation(Annotation* annot) {
    auto fields = annot->to_tuples();

    for (int i = 0; i < fields.size(); i++) {
        if (fields[i].first == "uuid") {
            fields.erase(fields.begin() + i);
            break;
        }
    }

    return generic_update_run_query(get_annot_table_name(annot), { { "uuid", QString::fromStdString(annot->uuid) } }, fields);
}

bool DatabaseManager::delete_annotation(Annotation* annot) {
    std::string table = get_annot_table_name(annot);
    std::wstringstream ss;
    ss << "DELETE FROM " << esc(table) << " where uuid='" << esc(annot->uuid) << "';";
    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "delete_annotation",
        error_code,
        error_message);
}

bool DatabaseManager::has_column(const std::string& table_name, const std::string& column_name) {
    char* error_message = nullptr;
    int count = -1;
    std::wstringstream ss;
    ss << "SELECT COUNT(*) FROM pragma_table_info('" << esc(table_name) << "') WHERE name='" << esc(column_name) <<"';";
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), count_callback, &count, &error_message);
    return count > 0;
}

//bool DatabaseManager::set_highlight_uuid_to_synced(const std::string& uuid) {
//    return set_annot_uuids_to_synced("highlights", { uuid });
//}

bool DatabaseManager::set_annot_uuids_to_synced_(const std::string& table_name, const std::vector<std::string>& uuids) {
    std::wstringstream ss;
    ss << "UPDATE " << esc(table_name) << " SET is_synced=1 WHERE uuid IN (";
    for (int i = 0; i < uuids.size(); i++) {
        ss << "'" << esc(uuids[i]) << "'";
        if (i < uuids.size() - 1) {
            ss << ",";
        }
    }
    ss << ");";

    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "set_annot_uuids_to_synced",
        error_code,
        error_message);
}

bool DatabaseManager::set_document_to_synced(const std::string& checksum) {
    std::wstringstream ss;
    ss << "UPDATE opened_books SET is_synced=1 WHERE path='" << esc(checksum) << "';";

    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "set_document_to_synced",
        error_code,
        error_message);
}

bool DatabaseManager::set_document_to_unsynced(const std::string& checksum) {
    std::wstringstream ss;
    ss << "UPDATE opened_books SET is_synced=0 WHERE path='" << esc(checksum) << "';";

    char* error_message = nullptr;

    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), null_callback, 0, &error_message);
    return handle_error(
        "set_document_to_unsynced",
        error_code,
        error_message);
}

bool DatabaseManager::is_document_synced(const std::string& checksum) {
    std::wstringstream ss;
    ss << "SELECT is_synced FROM opened_books WHERE path='" << esc(checksum) << "'";

    char* error_message = nullptr;

    int is_synced = 0;
    int error_code = sqlite3_exec(global_db, utf8_encode(ss.str()).c_str(), count_callback, &is_synced, &error_message);
    bool ok = handle_error(
        "set_document_to_unsynced",
        error_code,
        error_message);

    return is_synced;
}

std::string DatabaseManager::get_table_name_for_annot_type(const std::string& annot_type) {
    if (annot_type == "highlight") return "highlights";
    if (annot_type == "bookmark") return "bookmarks";
    if (annot_type == "portal") return "links";

    return "";

}

void DatabaseManager::debug() {
    std::string query = "SELECT document_name FROM opened_books WHERE id=:id;";
    sqlite3_stmt* statement;
    auto is_ok = sqlite3_prepare_v2(global_db, query.c_str(), query.size(), &statement, nullptr);
    int id_index = sqlite3_bind_parameter_index(statement, ":id");

    if (is_ok == SQLITE_OK) {
        sqlite3_bind_int(statement, id_index, 92);
        while (true) {
            auto state = sqlite3_step(statement);
            if (state != SQLITE_ROW) {
                break;
            }
            const unsigned char* document_name_utf8 = sqlite3_column_text(statement, 0);
            qDebug() << QString::fromUtf8(document_name_utf8);

        }

        sqlite3_reset(statement);

        sqlite3_bind_int(statement, id_index, 270);
        while (true) {
            auto state = sqlite3_step(statement);
            if (state != SQLITE_ROW) {
                break;
            }
            const unsigned char* document_name_utf8 = sqlite3_column_text(statement, 0);
            qDebug() << QString::fromUtf8(document_name_utf8);

        }
    }

}


bool DatabaseManager::update_checksum(const std::string& old_checksum, const std::string& new_checksum) {

    std::string global_transaction = "BEGIN TRANSACTION;\n";
    std::string local_transaction = "BEGIN TRANSACTION;\n";
    std::vector<std::string> shared_queries_to_run = {
        "UPDATE opened_books SET path='" + new_checksum + "' WHERE path='" + old_checksum + "';",
        "UPDATE highlights SET document_path='" + new_checksum + "' WHERE document_path='" + old_checksum + "';",
        "UPDATE bookmarks SET document_path='" + new_checksum + "' WHERE document_path='" + old_checksum + "';",
        "UPDATE links SET src_document='" + new_checksum + "' WHERE src_document='" + old_checksum + "';",
        "UPDATE links SET dst_document='" + new_checksum + "' WHERE dst_document='" + old_checksum + "';",
        "UPDATE marks SET document_path='" + new_checksum + "' WHERE document_path='" + old_checksum + "';",
    };
    std::vector<std::string> local_queries_to_run = {
        "UPDATE document_hash SET hash='" + new_checksum + "' WHERE hash='" + old_checksum + "';"
    };

    for (auto q : shared_queries_to_run) {
        global_transaction += q + "\n";
    }

    for (auto q : local_queries_to_run) {
        local_transaction += q + "\n";
    }

    global_transaction += "COMMIT;";
    local_transaction += "COMMIT;";

    std::cout << global_transaction;

    if (!run_schema_query(local_db, local_transaction.c_str())) {
        run_schema_query(local_db, "ROLLBACK;");
        return false;
    }

    if (!run_schema_query(global_db, global_transaction.c_str())) {
        run_schema_query(global_db, "ROLLBACK;");
        return false;
    }
    return true;
}
