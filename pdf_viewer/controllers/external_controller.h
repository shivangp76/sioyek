#pragma once

#include <string>

#include "book.h"
#include "controllers/base_controller.h"


class Path;
class QTemporaryFile;
class QFileSystemWatcher;
class QProcess;

struct ShellOutputBookmark{
    std::string uuid;
    qint64 pid;
    QTemporaryFile* output_file;
    QTemporaryFile* document_content_file = nullptr;
    QTemporaryFile* image_file = nullptr;
    QFileSystemWatcher* watcher;
    QProcess* process;
    QString style_string = "";
    QDateTime last_update_time;
};

class ExternalController : public BaseController{
private:
    std::vector<ShellOutputBookmark> shell_output_bookmarks;
public:
    ExternalController(MainWidget* parent);
    std::wstring handle_synctex_to_ruler();
    std::wstring synctex_under_pos(WindowPos position);
    void do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line, int column);
    void execute_command(std::wstring command, std::wstring text = L"", bool wait = false);
    void handle_bookmark_shell_command(QString bookmark_text, std::string uuid, QString text_arg="");
    void update_shell_bookmarks();
    void on_bookmark_shell_output_updated(std::string bookmark_uuid, QString file_path);
    std::optional<ShellOutputBookmark> get_shell_output_bookmark_with_uuid(std::string uuid);
    void remove_finished_shell_bookmark_with_index(int index);
    void remove_finished_shell_bookmarks();
    void handle_shell_bookmark_deleted(std::string uuid);

};
