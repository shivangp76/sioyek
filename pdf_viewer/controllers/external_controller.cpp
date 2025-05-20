#include <QProcess>
#include <QTemporaryFile>
#include <QFileInfo>

#include "synctex/synctex_parser.h"

#include "controllers/external_controller.h"
#include "document_view.h"
#include "main_widget.h"

extern bool USE_RULER_TO_HIGHLIGHT_SYNCTEX_LINE;
extern bool DONT_FOCUS_IF_SYNCTEX_RECT_IS_VISIBLE;
extern Path local_database_file_path;
extern Path global_database_file_path;

ExternalController::ExternalController(MainWidget* parent) : BaseController(parent){
}

std::wstring ExternalController::handle_synctex_to_ruler() {
    std::optional<NormalizedWindowRect> ruler_rect = mdv()->get_ruler_window_rect();
    if (ruler_rect.has_value()){
        fz_irect ruler_irect = mdv()->normalized_to_window_rect(ruler_rect.value());

        WindowPos mid_window_pos;
        mid_window_pos.x = (ruler_irect.x0 + ruler_irect.x1) / 2;
        mid_window_pos.y = (ruler_irect.y0 + ruler_irect.y1) / 2;

        return synctex_under_pos(mid_window_pos);
    }
    return L"";
}

std::wstring ExternalController::synctex_under_pos(WindowPos position) {
    std::wstring res = L"";
#ifndef SIOYEK_MOBILE
    auto [page, doc_x, doc_y] = mdv()->window_to_document_pos(position);
    std::wstring docpath = mdv()->get_document()->get_path();
    std::string docpath_utf8 = utf8_encode(docpath);
    synctex_scanner_p scanner = synctex_scanner_new_with_output_file(docpath_utf8.c_str(), nullptr, 1);

    int stat = synctex_edit_query(scanner, page + 1, doc_x, doc_y);

    if (stat > 0) {
        synctex_node_p node;
        while ((node = synctex_scanner_next_result(scanner))) {
            int line = synctex_node_line(node);
            int column = synctex_node_column(node);
            if (column < 0) column = 0;
            int tag = synctex_node_tag(node);
            const char* file_name = synctex_scanner_get_name(scanner, tag);
            QString new_path;
#ifdef Q_OS_WIN
            // the path returned by synctex is formatted in unix style, for example it is something like this
            // in windows: d:/some/path/file.pdf
            // this doesn't work with Vimtex for some reason, so here we have to convert the path separators
            // to windows style and make sure the driver letter is capitalized
            QDir file_path = QDir(file_name);
            new_path = QDir::toNativeSeparators(file_path.absolutePath());
            new_path[0] = new_path[0].toUpper();
            if (VIMTEX_WSL_FIX) {
                new_path = file_name;
            }

#else
            new_path = file_name;
#endif

            std::string line_string = std::to_string(line);
            std::string column_string = std::to_string(column);

            if (mw->inverse_search_command.size() > 0) {
#ifdef Q_OS_WIN
                QString command = QString::fromStdWString(mw->inverse_search_command).arg(new_path, line_string.c_str(), column_string.c_str());
#else
                QString command = QString::fromStdWString(mw->inverse_search_command).arg(file_name, line_string.c_str(), column_string.c_str());
#endif
                QStringList args = QProcess::splitCommand(command);
                QProcess::startDetached(args[0], args.mid(1));
            }
            else {
                show_error_message(L"inverse_search_command is not set in prefs_user.config");
            }

        }

    }
    synctex_scanner_free(scanner);

#endif
    return res;
}

void ExternalController::do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line, int column) {
#ifndef SIOYEK_MOBILE

    std::wstring latex_file_path_with_redundant_dot = add_redundant_dot_to_path(latex_file_path.get_path());

    std::string latex_file_string = latex_file_path.get_path_utf8();
    std::string latex_file_with_redundant_dot_string = utf8_encode(latex_file_path_with_redundant_dot);
    std::string pdf_file_string = pdf_file_path.get_path_utf8();

    synctex_scanner_p scanner = synctex_scanner_new_with_output_file(pdf_file_string.c_str(), nullptr, 1);

    int stat = synctex_display_query(scanner, latex_file_string.c_str(), line, column, 0);
    int target_page = -1;

    if (stat <= 0) {
        stat = synctex_display_query(scanner, latex_file_with_redundant_dot_string.c_str(), line, column, 0);
    }

    if (stat > 0) {
        synctex_node_p node;

        std::vector<DocumentRect> highlight_rects;

        while ((node = synctex_scanner_next_result(scanner))) {
            int page = synctex_node_page(node);
            target_page = page - 1;

            float x = synctex_node_box_visible_h(node);
            float y = synctex_node_box_visible_v(node);
            float w = synctex_node_box_visible_width(node);
            float h = synctex_node_box_visible_height(node);

            highlight_rects.push_back(DocumentRect({x, y, x + w, y - h}, target_page));

            break; // todo: handle this properly
        }
        if (target_page != -1) {

            if ((mdv()->get_document() == nullptr) ||
                (pdf_file_path.get_path() != mdv()->get_document()->get_path())) {

                mw->open_document(pdf_file_path);

            }

            if (!USE_RULER_TO_HIGHLIGHT_SYNCTEX_LINE) {

                std::optional<AbsoluteRect> line_rect_absolute = doc()->get_page_intersecting_rect(highlight_rects[0]);
                if (line_rect_absolute){
                    DocumentRect line_rect = line_rect_absolute->to_document(doc());
                    bool should_recenter = true;
                    if (DONT_FOCUS_IF_SYNCTEX_RECT_IS_VISIBLE){
                        NormalizedWindowRect line_window_rect = line_rect.to_window_normalized(mdv());
                        if (line_window_rect.is_visible()){
                            should_recenter = false;
                        }
                    }

                    mdv()->set_synctex_highlights({ line_rect });
                    if (highlight_rects.size() == 0) {
                        mdv()->goto_page(target_page);
                    }
                    else {
                        if (should_recenter){
                            mdv()->goto_offset_within_page(target_page, highlight_rects[0].rect.y0);
                        }
                    }
                }
            }
            else {
                if (highlight_rects.size() > 0) {
                    mdv()->focus_rect(highlight_rects[0]);
                }
            }
        }
    }
    else {
        mw->open_document(pdf_file_path);
    }
    synctex_scanner_free(scanner);
#endif
}

void ExternalController::execute_command(std::wstring command, std::wstring text, bool wait) {
#ifndef SIOYEK_MOBILE

    std::wstring file_path = mdv()->get_document()->get_path();
    QString qfile_path = QString::fromStdWString(file_path);
    std::vector<std::wstring> path_parts;
    split_path(file_path, path_parts);
    std::wstring file_name = path_parts.back();
    QString qfile_name = QString::fromStdWString(file_name);

    QString qtext = QString::fromStdWString(command);

#ifdef SIOYEK_QT6
    // QStringList command_parts_ = qtext.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    QStringList command_parts_ = QProcess::splitCommand(qtext);
#else
    QStringList command_parts_ = qtext.split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif

    QStringList command_parts;
    while (command_parts_.size() > 0) {
        if ((command_parts_.size() <= 1) || (!command_parts_.at(0).endsWith("\\"))) {
            command_parts.append(command_parts_.at(0));
            command_parts_.pop_front();
        }
        else {
            QString first_part = command_parts_.at(0);
            QString second_part = command_parts_.at(1);
            QString new_command_part = first_part.left(first_part.size() - 1) + " " + second_part;
            command_parts_.pop_front();
            command_parts_.replace(0, new_command_part);
        }
    }
    if (command_parts.size() > 0) {
        QString command_name = command_parts[0];
        QStringList command_args;

        command_parts.takeFirst();

        QPoint mouse_pos_ = mw->mapFromGlobal(mw->cursor_pos());
        WindowPos mouse_pos = { mouse_pos_.x(), mouse_pos_.y() };
        DocumentPos mouse_pos_document = mdv()->window_to_document_pos(mouse_pos);

        for (int i = 0; i < command_parts.size(); i++) {
            // lagacy number macros, now replaced with names ones
            command_parts[i].replace("%1", qfile_path);
            command_parts[i].replace("%2", qfile_name);
            command_parts[i].replace("%3", QString::fromStdWString(mdv()->get_selected_text()));
            command_parts[i].replace("%4", QString::number(mw->get_current_page_number()));
            command_parts[i].replace("%5", QString::fromStdWString(text));

            // new named macros
            command_parts[i].replace("%{file_path}", qfile_path);
            command_parts[i].replace("%{file_name}", qfile_name);
            command_parts[i].replace("%{selected_text}", QString::fromStdWString(mdv()->get_selected_text()));
            std::wstring current_selected_text = mdv()->get_selected_text();

            if (current_selected_text.size() > 0) {
                auto selection_begin_document = mdv()->get_document()->absolute_to_page_pos(dv()->selection_begin);
                command_parts[i].replace("%{selection_begin_document}",
                    QString::number(selection_begin_document.page) + " " + QString::number(selection_begin_document.x) + " " + QString::number(selection_begin_document.y));
                auto selection_end_document = mdv()->get_document()->absolute_to_page_pos(dv()->selection_end);
                command_parts[i].replace("%{selection_end_document}",
                    QString::number(selection_end_document.page) + " " + QString::number(selection_end_document.x) + " " + QString::number(selection_end_document.y));
            }
            command_parts[i].replace("%{page_number}", QString::number(mw->get_current_page_number()));
            command_parts[i].replace("%{command_text}", QString::fromStdWString(text));


            command_parts[i].replace("%{mouse_pos_window}", QString::number(mouse_pos.x) + " " + QString::number(mouse_pos.y));
            //command_parts[i].replace("%{mouse_pos_window}", QString("%1 %2").arg(mouse_pos.x, mouse_pos.y));
            command_parts[i].replace("%{mouse_pos_document}", QString::number(mouse_pos_document.page) + " " + QString::number(mouse_pos_document.x) + " " + QString::number(mouse_pos_document.y));
            //command_parts[i].replace("%{mouse_pos_document}", QString("%1 %2 %3").arg(mouse_pos_document.page, mouse_pos_document.x, mouse_pos_document.y));
            if (command_parts[i].indexOf("%{paper_name}") != -1) {
                std::optional<PaperNameWithRects> maybe_paper_name = mw->get_paper_name_under_cursor();
                if (maybe_paper_name) {
                    command_parts[i].replace("%{paper_name}", maybe_paper_name->paper_name);
                }
            }

            command_parts[i].replace("%{sioyek_path}", QCoreApplication::applicationFilePath());
            command_parts[i].replace("%{local_database}", QString::fromStdWString(local_database_file_path.get_path()));
            command_parts[i].replace("%{shared_database}", QString::fromStdWString(global_database_file_path.get_path()));

            int selected_rect_page = -1;
            std::optional<DocumentRect> selected_rect_document = mdv()->get_selected_rect_document();
            if (selected_rect_document) {
                selected_rect_page = selected_rect_document->page;
                QString format_string = "%1,%2,%3,%4,%5";
                QString rect_string = format_string
                    .arg(QString::number(selected_rect_page))
                    .arg(QString::number(selected_rect_document->rect.x0))
                    .arg(QString::number(selected_rect_document->rect.y0))
                    .arg(QString::number(selected_rect_document->rect.x1))
                    .arg(QString::number(selected_rect_document->rect.y1));
                command_parts[i].replace("%{selected_rect}", rect_string);
            }


            std::wstring selected_line_text;
            if (mdv()) {
                selected_line_text = mdv()->get_selected_line_text().value_or(L"");
                command_parts[i].replace("%{zoom_level}", QString::number(mdv()->get_zoom_level()));
                DocumentPos docpos = mdv()->get_offsets().to_document(doc());
                command_parts[i].replace("%{offset_x}", QString::number(mdv()->get_offset_x()));
                command_parts[i].replace("%{offset_y}", QString::number(mdv()->get_offset_y()));
                command_parts[i].replace("%{offset_x_document}", QString::number(docpos.x));
                command_parts[i].replace("%{offset_y_document}", QString::number(docpos.y));
            }

            if (selected_line_text.size() > 0) {
                command_parts[i].replace("%6", QString::fromStdWString(selected_line_text));
                command_parts[i].replace("%{line_text}", QString::fromStdWString(selected_line_text));
            }

            std::wstring command_parts_ = command_parts[i].toStdWString();
            command_args.push_back(command_parts[i]);
        }

        run_command(command_name.toStdWString(), command_args, wait);
    }
#endif
}

void ExternalController::handle_bookmark_shell_command(QString text, std::string pending_uuid, QString text_arg){
#ifndef SIOYEK_MOBILE

    if (text.startsWith("#shell")){
        text = text.mid(QString("#shell").size() + 1); // also skip the space
    }

    QTemporaryFile* file_content_temp_file = nullptr;
    QTemporaryFile* image_temp_file = nullptr;
    QString style_string = "";
    if (text.startsWith("#") && (!text.startsWith("#shell"))){
        int first_space_index = text.indexOf(" ");
        if (first_space_index >= 0){
            style_string = text.mid(0, first_space_index).trimmed();
            text = text.mid(first_space_index + 1);
        }
    }

    if (text.indexOf("%{text}") != -1){
        text = text.replace("%{text}", text_arg);
    }
    if (text.indexOf("%{document_text_file}") != -1){
        file_content_temp_file = new QTemporaryFile();
        file_content_temp_file->open();
        if (doc()->get_super_fast_index().size() > 0){
            file_content_temp_file->write(utf8_encode(doc()->get_super_fast_index()).c_str());
            text = text.replace("%{document_text_file}", file_content_temp_file->fileName());
        }
    }
    if (text.indexOf("%{current_page_begin_index}") != -1){
        int current_page = mdv()->get_current_page_number();
        int page_begin_index = doc()->get_super_fast_page_begin_indices()[current_page];
        text = text.replace("%{current_page_begin_index}", QString::number(page_begin_index));
    }
    if (text.indexOf("%{current_page_end_index}") != -1){
        int next_page = mdv()->get_current_page_number() + 1;

        if (next_page < doc()->get_super_fast_page_begin_indices().size()){
            int page_begin_index = doc()->get_super_fast_page_begin_indices()[next_page];
            text = text.replace("%{current_page_end_index}", QString::number(page_begin_index));
        }
        else{
            text = text.replace("%{current_page_end_index}", QString::number(doc()->get_super_fast_index().size()));
        }
    }
    if (text.indexOf("%{bookmark_image_file}") != -1){
        BookMark* bm = doc()->get_bookmark_with_uuid(pending_uuid);
        if (bm){
            std::optional<AbsoluteRect> rect = bm->get_rectangle();
            if (rect){
                WindowRect window_rect = rect->to_window(dv());
                QRect window_qrect = window_rect.to_qrect();
                QPixmap pixmap;

                float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
                pixmap = QPixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
                pixmap.setDevicePixelRatio(ratio);

                mw->render(&pixmap, QPoint(), QRegion(window_qrect));

                image_temp_file = new QTemporaryFile();
                image_temp_file->open();
                image_temp_file->setFileName(image_temp_file->fileName() + ".png");
                pixmap.save(image_temp_file->fileName());

                text = text.replace("%{bookmark_image_file}", image_temp_file->fileName());
            }
        }

    }
    if (text.indexOf("%{selected_text}") != -1){
        text = text.replace("%{selected_text}", QString::fromStdWString(mdv()->get_selected_text()));
    }

    // QString command = text.mid(QString("#shell").size()).trimmed();
    QStringList command_parts = QProcess::splitCommand(text);
    if (command_parts.size() > 0){

        QString command_name = command_parts.at(0);
        QStringList command_args = command_parts.mid(1);
        // qint64 pid = -1;
        // QProcess::startDetached(command_name, command_args, QString(), &pid);


        QTemporaryFile* temp_file = new QTemporaryFile();
        temp_file->open();
        QFileSystemWatcher* watcher = new QFileSystemWatcher();
        watcher->addPath(temp_file->fileName());
        QObject::connect(watcher, &QFileSystemWatcher::fileChanged, [this, pending_uuid](const QString& path) {

                on_bookmark_shell_output_updated(pending_uuid, path);
                });

        qint64 pid = -1;
        QProcess* process = new QProcess();
        process->setProgram(command_name);
        process->setArguments(command_args);
        process->setStandardOutputFile(temp_file->fileName());
        process->startDetached(&pid);

        ShellOutputBookmark shell_output_bookmark;
        shell_output_bookmark.pid = pid;
        shell_output_bookmark.uuid = pending_uuid;
        shell_output_bookmark.output_file = std::move(temp_file);
        shell_output_bookmark.watcher = watcher;
        shell_output_bookmark.process = process;
        shell_output_bookmark.document_content_file = file_content_temp_file;
        shell_output_bookmark.image_file = image_temp_file;
        shell_output_bookmark.style_string = style_string;
        shell_output_bookmark.last_update_time = QDateTime::currentDateTime();

        shell_output_bookmarks.push_back(shell_output_bookmark);

    }
#endif
}

void ExternalController::update_shell_bookmarks(){
    for (auto shell_bookmark : shell_output_bookmarks){
        QString file_name = shell_bookmark.output_file->fileName();
        QFileInfo file_info(file_name);
        if (file_info.lastModified().msecsTo(shell_bookmark.last_update_time) < 0){
            on_bookmark_shell_output_updated(shell_bookmark.uuid, file_name);
        }
    }
}

void ExternalController::on_bookmark_shell_output_updated(std::string bookmark_uuid, QString file_path) {
    QFile file(file_path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString content = stream.readAll();
        file.close();
        BookMark* bm = doc()->get_bookmark_with_uuid(bookmark_uuid);
        if (bm) {
            bm->is_pending = false;
            std::optional<ShellOutputBookmark> shell_output_data = get_shell_output_bookmark_with_uuid(bookmark_uuid);
            if (shell_output_data) {
                bm->description = (shell_output_data->style_string + " " + content).toStdWString();
                doc()->update_bookmark_text(bookmark_uuid, bm->description, bm->font_size);
                mw->on_bookmark_edited(*bm, false, false);
                mw->invalidate_render();
            }
        }
    }
}

std::optional<ShellOutputBookmark> ExternalController::get_shell_output_bookmark_with_uuid(std::string uuid){
    for (int i = 0; i < shell_output_bookmarks.size(); i++){
        if (shell_output_bookmarks[i].uuid == uuid){
            return shell_output_bookmarks[i];
        }
    }
    return {};
}

void ExternalController::remove_finished_shell_bookmark_with_index(int index){
#ifndef SIOYEK_MOBILE
    if (index >= 0 && index < shell_output_bookmarks.size()){
        ShellOutputBookmark& shell_output_bookmark = shell_output_bookmarks[index];
        shell_output_bookmark.watcher->removePath(shell_output_bookmark.output_file->fileName());
        shell_output_bookmark.watcher->deleteLater();
        shell_output_bookmark.output_file->remove();
        shell_output_bookmark.output_file->deleteLater();

        if (shell_output_bookmark.document_content_file){
            shell_output_bookmark.document_content_file->remove();
            shell_output_bookmark.document_content_file->deleteLater();
        }

        if (shell_output_bookmark.image_file){
            shell_output_bookmark.image_file->remove();
            shell_output_bookmark.image_file->deleteLater();
        }

        shell_output_bookmark.process->deleteLater();
        shell_output_bookmarks.erase(shell_output_bookmarks.begin() + index);
    }
#endif
}

void ExternalController::remove_finished_shell_bookmarks(){
#ifndef SIOYEK_MOBILE
    std::vector<int> indices_to_delete;
    for (int i = 0; i < shell_output_bookmarks.size(); i++){
        if (!is_process_still_running(shell_output_bookmarks[i].pid)){
            on_bookmark_shell_output_updated(shell_output_bookmarks[i].uuid, shell_output_bookmarks[i].output_file->fileName());
            indices_to_delete.push_back(i);
        }
    }
    for (int j = indices_to_delete.size() - 1; j >= 0; j--){
        int index = indices_to_delete[j];
        remove_finished_shell_bookmark_with_index(index);
    }
#endif
}

void ExternalController::handle_shell_bookmark_deleted(std::string uuid){
    for (int i = 0; i < shell_output_bookmarks.size(); i++){
        if (shell_output_bookmarks[i].uuid == uuid) {
            kill_process(shell_output_bookmarks[i].pid);
            remove_finished_shell_bookmark_with_index(i);
            break;
        }
    }
}
