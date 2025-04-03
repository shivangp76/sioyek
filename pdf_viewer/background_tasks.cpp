#include <unordered_set>
#include <qpixmap.h>
#include <qpainter.h>
#include <qtextdocument.h>
#include <qtextformat.h>
#include <qtextcursor.h>
#include <qabstracttextdocumentlayout.h>
#include <qdir.h>
#include <qthread.h>

#include "background_tasks.h"
#include "utils.h"
#include "path.h"

#ifdef SIOYEK_MICROTEX
#include "latex.h"
#include "platform/qt/graphic_qt.h"
#include "core/formula.h"
#endif


extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern float QUESTION_BOOKMARK_TEXT_COLOR[3];
extern float HIGHLIGHT_COLORS[26 * 3];
extern bool VERBOSE;
extern Path standard_data_path;

extern QString computer_modern_font_family;
extern std::wstring BOOKMARK_FONT_FACE;
extern int BACKGROUND_BOOKMARKS_PIXEL_BUDGET;


void BackgroundBookmarkRenderer::initialize_latex() {
#ifdef SIOYEK_MICROTEX
    if (!is_latex_initialized) {
        is_latex_initialized = true;
        copy_microtex_files();
        std::string root_dir = standard_data_path.slash(L"microtex_resources").get_path_utf8();
        tex::LaTeX::init(root_dir);
    }
#endif
}

void prepare_text_document_for_bookmark_markdown(QTextDocument& td, QString text, QRect window_qrect, const QFont& font) {
    auto formats = td.allFormats();
    QTextCharFormat old_format = formats[0].toCharFormat();

    td.setMarkdown(text, QTextDocument::MarkdownFeature::MarkdownDialectGitHub);

    td.setTextWidth(window_qrect.width());
    td.setDefaultFont(font);

}

float BackgroundBookmarkRenderer::draw_markdown_text(QPainter& painter, QString text, QRect window_qrect, float scroll_amount, bool is_from_main_thread, const QFont& font) {
    painter.save();
    QPoint top_left = QPoint(window_qrect.topLeft().x(), window_qrect.topLeft().y() - scroll_amount);
    QPoint clip_top_left = QPoint(window_qrect.topLeft().x(), window_qrect.topLeft().y());
    QRect clip_rect = QRect(clip_top_left, window_qrect.size());
    painter.setClipRect(window_qrect);

    painter.translate(top_left);

    QTextDocument td;
    prepare_text_document_for_bookmark_markdown(td, text, window_qrect, font);

    //auto formats = td.allFormats();
    //QTextCharFormat old_format = formats[0].toCharFormat();

    //td.setMarkdown(text, QTextDocument::MarkdownFeature::MarkdownDialectGitHub);

    //td.setTextWidth(window_qrect.width());
    //td.setDefaultFont(font);

    QTextCursor cursor(&td);
    QTextCharFormat format;
    cursor.select(QTextCursor::Document);
    QAbstractTextDocumentLayout::PaintContext ctx;
    window_qrect = QRect(0, 0, window_qrect.width(), window_qrect.height());
    ctx.palette.setColor(QPalette::ColorRole::Text, painter.pen().color());
    td.documentLayout()->draw(&painter, ctx);
    //td.documentLayout().ob
    QSizeF size = td.documentLayout()->documentSize();

    painter.restore();
    return size.height();
}

void BackgroundBookmarkRenderer::render_freetext_bookmark(const BookMark& bookmark, QPainter* painter, float zoom_level, float scroll_amount, float pixel_ratio, QRect window_qrect, ColorPalette palette, bool is_from_main_thread) {
    QString desc_qstring = QString::fromStdWString(bookmark.description);

    //painter->setPen(convert_float3_to_qcolor(&bookmark.color[0]));
    painter->setPen(qconvert_color3(bookmark.color, palette));

    if (is_from_main_thread) {
        float bookmark_color[3];
        get_color_for_mode(palette, bookmark.color, bookmark_color);
        painter->setPen(convert_float3_to_qcolor(&bookmark_color[0]));
    }

    QFont font = bookmark.get_font(zoom_level);
    painter->setFont(font);

    std::optional<char> text_color_type = bookmark.get_text_type();


    if (desc_qstring.startsWith("#markdown")) {
        float height = draw_markdown_text(*painter, bookmark.get_render_text(), window_qrect, scroll_amount, is_from_main_thread, font);
        cached_bookmark_heights_mutex.lock();
        cached_bookmark_heights_[bookmark.uuid] = height;
        cached_bookmark_heights_mutex.unlock();
    }
#ifdef SIOYEK_MICROTEX
    else if (desc_qstring.startsWith("#latex")) {
        tex::color foreground_color = tex::black;

        if (text_color_type && text_color_type.value() >= 'a' && text_color_type.value() <= 'z') {
            QColor pen_color = qconvert_color3(&HIGHLIGHT_COLORS[3 * (text_color_type.value() - 'a')], palette);
            painter->setPen(pen_color);
            foreground_color = tex::argb(pen_color.alpha(), pen_color.red(), pen_color.green(), pen_color.blue());
        }

        // rendering latex can take a while, so we never do it from the main thread
        if (is_from_main_thread){
            return;
        }

        latex_lock.lock();
        initialize_latex();

        // QString latex_text = desc_qstring.mid(6).trimmed();
        QString latex_text = bookmark.get_render_text();

        std::wstring code = latex_text.toStdWString();

        try {
            tex::Formula formula;
            tex::TeXRenderBuilder builder;
            formula.setLaTeX(code);
            formula.PIXELS_PER_POINT = pixel_ratio;

            float text_size = 10 * zoom_level;
            int width_in_pixels = window_qrect.width();
            auto r = builder
                .setStyle(tex::TexStyle::display)
                .setTextSize(text_size)
                .setWidth(tex::UnitType::pixel, width_in_pixels, tex::Alignment::left)
                .setIsMaxWidth(false)
                .setLineSpace(tex::UnitType::pixel, 10)
                .setForeground(foreground_color)
                .build(formula);

            painter->save();
            painter->translate(0, -scroll_amount);
            tex::Graphics2D_qt g2(painter);

            r->draw(g2, window_qrect.x(), window_qrect.y());
            painter->restore();
            cached_bookmark_heights_mutex.lock();
            cached_bookmark_heights_[bookmark.uuid] = r->getHeight();
            cached_bookmark_heights_mutex.unlock();

            delete r;
        }
        catch (std::exception& e) {
            if (VERBOSE) {
                qDebug() << "Error in rendering latex: " << e.what();
            }
        }
        latex_lock.unlock();
    }
#endif
    else {
        if (bookmark.is_question() || bookmark.is_summary()) {
            //QColor question_text_color = convert_float3_to_qcolor(QUESTION_BOOKMARK_TEXT_COLOR);
            QColor question_text_color = qconvert_color3(QUESTION_BOOKMARK_TEXT_COLOR, palette);
            painter->setPen(question_text_color);
            //painter.drawText(window_qrect, flags, QString::fromStdWString(bookmarks[i].description).right(bookmarks[i].description.size() - 2));
            float height = draw_markdown_text(*painter, bookmark.get_question_or_summary_markdown(), window_qrect, scroll_amount, is_from_main_thread, font);
            cached_bookmark_heights_mutex.lock();
            cached_bookmark_heights_[bookmark.uuid] = height;
            cached_bookmark_heights_mutex.unlock();
        }
        else {
            int flags = Qt::TextWordWrap;
            if (is_text_rtl(bookmark.description)) {
                flags |= Qt::AlignRight;
            }
            else {
                flags |= Qt::AlignLeft;
            }
            painter->save();
            painter->setClipRect(window_qrect);

            QRect r = QRect(window_qrect.left(), window_qrect.top() - scroll_amount, window_qrect.width(), window_qrect.height() + scroll_amount);


            QRect bounding_rect;
            painter->drawText(r, flags, bookmark.get_render_text(), &bounding_rect);
            cached_bookmark_heights_mutex.lock();
            cached_bookmark_heights_[bookmark.uuid] = bounding_rect.height();
            cached_bookmark_heights_mutex.unlock();

            painter->restore();
        }
    }
}


void BackgroundTaskManager::start_worker_thread() {

    //MyThread(
    //    bool* should_stop_,
    //    std::mutex* pending_task_mutex_,
    //    std::condition_variable* pending_task_cv_,
    //    std::deque<std::pair<QObject*, std::function<void()>>>* pending_tasks_,
    //    QObject** current_task_parent_);
    should_stop = false;
    worker_thread = std::make_unique<MyThread>(
        &should_stop,
        &pending_tasks_mutex,
        &pending_taks_cv,
        &pending_tasks,
        &current_task_parent);
    worker_thread->start();

    //worker_thread = std::thread([&]() {
    //    while (!should_stop) {
    //        std::unique_lock<std::mutex> lock(pending_tasks_mutex);
    //        pending_taks_cv.wait(lock, [&]{
    //            return pending_tasks.size() > 0 || should_stop;
    //            });
    //        if (should_stop) {
    //            break;
    //        }
    //        auto [parent, task] = std::move(pending_tasks.front());
    //        pending_tasks.pop_front();
    //        current_task_parent = parent;
    //        lock.unlock();
    //        task();
    //        current_task_parent = nullptr;
    //    }
    //    });
}

void BackgroundTaskManager::stop_worker_thread(){
    if (worker_thread != nullptr) {
        should_stop = true;
        pending_taks_cv.notify_one();
        worker_thread->wait();
        worker_thread = {};
    }
}

BackgroundTaskManager::~BackgroundTaskManager() {
    if (is_worker_thread_started) {
        should_stop = true;
        pending_taks_cv.notify_one();
        worker_thread->wait();
        //worker_thread->join
        //worker_thread.join();
    }
}

void BackgroundTaskManager::delete_tasks_with_parent(QObject* parent) {
    if (is_worker_thread_started) {
        while (parent == current_task_parent) {};

        pending_tasks_mutex.lock();
        auto new_end = std::remove_if(pending_tasks.begin(), pending_tasks.end(), [parent](const std::pair<QObject*, std::function<void()>>& task) {
            return task.first == parent;
            });
        pending_tasks.erase(new_end, pending_tasks.end());
        pending_tasks_mutex.unlock();
    }
}

void BackgroundTaskManager::add_task(std::function<void()>&& fn, QObject* parent) {
    if (!is_worker_thread_started) {
        start_worker_thread();
        is_worker_thread_started = true;
    }

    pending_tasks.push_back(std::make_pair(parent, std::move(fn)));
    pending_taks_cv.notify_one();
}


bool BackgroundBookmarkRenderer::are_bookmarks_the_same_for_render(const BookMark& bm1, const BookMark& bm2) {
    return (bm1.uuid == bm2.uuid) &&
        (bm1.description == bm2.description) &&
        (std::abs(bm1.get_rectangle()->width() - bm2.get_rectangle()->width()) < 0.001f) &&
        (std::abs(bm1.get_rectangle()->height() - bm2.get_rectangle()->height()) < 0.001f);
}

std::vector<int> BackgroundBookmarkRenderer::get_request_indices(const std::vector<RenderedBookmark>& list, const BookMark& bm, float zoom_level, float scroll_amount, ColorPalette palette, bool compare_zoom_level) {
    std::vector<int> res;
    for (int i = 0; i < list.size(); i++) {
        if (are_bookmarks_the_same_for_render(list[i].bookmark, bm) && (list[i].color_palette == palette) && (list[i].scroll_amount == scroll_amount)) {
            if (!compare_zoom_level || (compare_zoom_level && (list[i].zoom_level == zoom_level))) {
                res.push_back(i);
            }
        }
    }
    return res;
}

bool BackgroundBookmarkRenderer::does_request_exist(const std::vector<RenderedBookmark>& list, const BookMark& bm, float zoom_level, float scroll_amount, ColorPalette palette) {
    for (auto& l : list) {
        if (are_bookmarks_the_same_for_render(l.bookmark, bm) && (l.color_palette == palette) && (l.scroll_amount == scroll_amount) && (l.zoom_level == zoom_level)) {
            return true;
        }
    }
    return false;
}

QPixmap* BackgroundBookmarkRenderer::get_rendered_bookmark(const BookMark& bm, float zoom_level, float scroll_amount, ColorPalette palette) {
    for (int i = 0; i < rendered_bookmarks.size(); i++) {
        if (
            are_bookmarks_the_same_for_render(bm, rendered_bookmarks[i].bookmark) &&
            (rendered_bookmarks[i].zoom_level == zoom_level) &&
            (rendered_bookmarks[i].scroll_amount == scroll_amount) &&
            (rendered_bookmarks[i].color_palette == palette)
            ) {
            rendered_bookmarks[i].last_access_time = QDateTime::currentDateTime();
            return rendered_bookmarks[i].pixmap;
        }

    }
    return nullptr;
}

BackgroundBookmarkRenderer::BackgroundBookmarkRenderer(BackgroundTaskManager* background_task_manager) : QObject(nullptr), task_manager(background_task_manager) {

}

std::pair<QPixmap*, bool> BackgroundBookmarkRenderer::request_rendered_bookmark(const BookMark& bm, float zoom_level, float scroll_amount, float pixel_ratio, ColorPalette palette) {
    float bm_width = bm.get_rectangle()->width();
    float bm_height = bm.get_rectangle()->height();
    float area = bm_width * bm_height * zoom_level * zoom_level * pixel_ratio * pixel_ratio;
    while (area > BACKGROUND_BOOKMARKS_PIXEL_BUDGET) {
        zoom_level /= 2;
        area /= 4;
    }

    rendered_bookmarks_mutex.lock_shared();
    QPixmap* res = get_rendered_bookmark(bm, zoom_level, scroll_amount, palette);
    rendered_bookmarks_mutex.unlock_shared();

    if (res != nullptr) {
        return std::make_pair(res, true);
    }
    else {
        rendered_bookmarks_mutex.lock();
        bool should_add_request = true;
        if (does_request_exist(rendered_bookmarks, bm, zoom_level, scroll_amount, palette)) {
            should_add_request = false;
        }

        if (should_add_request) {
            for (int i = 0; i < rendered_bookmarks.size(); i++) {
                if (rendered_bookmarks[i].bookmark.uuid == bm.uuid) {
                    rendered_bookmarks[i].canceled = true;
                }
            }

            RenderedBookmark req;
            req.bookmark = bm;
            req.zoom_level = zoom_level;
            req.scroll_amount = scroll_amount;
            req.color_palette = palette;
            req.pixel_ratio = pixel_ratio;
            req.pixmap = nullptr;
            req.last_access_time = QDateTime::currentDateTime();
            req.request_id = next_request_id++;
            rendered_bookmarks.push_back(req);

            task_manager->add_task([this, id=req.request_id, pixel_ratio]() {
                std::optional<RenderedBookmark> req_ = get_request_with_id(id);
                //req.bookmark.get_rectangle().to_window()
                if (req_ && req_->canceled) {
                    erase_request_with_id(id);
                }
                if (req_ && (!req_->canceled)) {
                    RenderedBookmark req = req_.value();

                    int width = req.bookmark.get_rectangle()->width() * req.zoom_level * req.pixel_ratio;
                    int height = req.bookmark.get_rectangle()->height() * req.zoom_level * req.pixel_ratio;
                    QRect rect = QRect(0, 0, width, height);

                    QPixmap* rendered_pixmap = new QPixmap(width, height);
                    rendered_pixmap->fill(QColor(255, 255, 255, 0));
                    { // we want the QPainter to be destroyed before we manually delete the pixmap later
                        QPainter painter(rendered_pixmap);
                        QRect window_rect = rendered_pixmap->rect();
                        if (pixel_ratio > 1){
                            painter.scale(pixel_ratio, pixel_ratio);
                            window_rect = QRect(0, 0, width / pixel_ratio, height / pixel_ratio);

                        }
                        //painter.translate(0, -req.scroll_amount);
                        render_freetext_bookmark(req.bookmark, &painter, req.zoom_level, req.scroll_amount, req.pixel_ratio, window_rect, req.color_palette);
                    }

                    bool is_latex = req.bookmark.is_latex();

                    // we can't set the text and background color using microtex (?)
                    // so here we convert the pixels directly
                    if (is_latex && (req.color_palette != ColorPalette::Normal)) {


                        QImage image = rendered_pixmap->toImage();
                        image = image.convertToFormat(QImage::Format_RGBA8888);
                        delete rendered_pixmap;

                        unsigned char* image_data = image.bits();
                        int stride = image.bytesPerLine();
                        int channels = image.depth() / 8;
                        if (req.color_palette == ColorPalette::Dark) {
                            convert_pixels_with_converter(image_data, image.width(), image.height(), stride, channels, [](unsigned char* pixel) {
                                convert_pixel_to_dark_mode(pixel);
                                });
                        }
                        else if (req.color_palette == ColorPalette::Custom) {

                            float transform_matrix[16];
                            get_custom_color_transform_matrix(transform_matrix);
                            convert_pixels_with_converter(image_data, image.width(), image.height(), stride, channels, [&](unsigned char* pixel) {
                                convert_pixel_to_custom_color(pixel, transform_matrix);
                                });
                        }

                        rendered_pixmap = new QPixmap(QPixmap::fromImage(image));
                    }

                    rendered_bookmarks_mutex.lock();
                    std::vector<int> pending_indices = get_request_indices(rendered_bookmarks, req.bookmark, req.zoom_level, req.scroll_amount, req.color_palette);
                    assert(pending_indices.size() <= 1);
                    if (pending_indices.size() == 1) {
                        rendered_bookmarks[pending_indices[0]].pixmap = rendered_pixmap;
                    }
                    rendered_bookmarks_mutex.unlock();
                    emit bookmark_rendered();
                }
                }, nullptr);

            //pending_render_bookmarks.push_back(req);
        }

        rendered_bookmarks_mutex.unlock();
        if (should_add_request) {
            cleanup_bookmarks();
        }

        // if we can't find the pixmap with exact zoom level, we can return one with different zoom level
        rendered_bookmarks_mutex.lock_shared();
        std::vector<int> indices = get_request_indices(rendered_bookmarks, bm, zoom_level, scroll_amount, palette, false);
        for (int i = rendered_bookmarks.size() - 1; i >= 0; i--) {
            if (rendered_bookmarks[i].pixmap != nullptr && (rendered_bookmarks[i].bookmark.uuid == bm.uuid)) {
                QPixmap* res = rendered_bookmarks[i].pixmap;
                rendered_bookmarks_mutex.unlock_shared();
                return std::make_pair(res, false);
            }
        }
        rendered_bookmarks_mutex.unlock_shared();
        return std::make_pair(nullptr, false);

    }


}

void BackgroundBookmarkRenderer::cleanup_bookmarks() {
    rendered_bookmarks_mutex.lock_shared();

    int current_pixel_size = 0;
    for (int i = 0; i < rendered_bookmarks.size(); i++) {
        if (rendered_bookmarks[i].pixmap) {
            QSize size = rendered_bookmarks[i].pixmap->size();
            current_pixel_size += size.width() * size.height();
        }
    }

    rendered_bookmarks_mutex.unlock_shared();

    if (current_pixel_size > 2 * BACKGROUND_BOOKMARKS_PIXEL_BUDGET) {
        rendered_bookmarks_mutex.lock();
        //std::unordered_set<int> indices_to_keep;
        int keep_size = current_pixel_size;
        std::sort(rendered_bookmarks.begin(), rendered_bookmarks.end(), [](const RenderedBookmark& a, const RenderedBookmark& b) {
            return a.last_access_time < b.last_access_time;
        });

        int index = 0;
        while ((keep_size > BACKGROUND_BOOKMARKS_PIXEL_BUDGET) && (index < rendered_bookmarks.size())) {
            RenderedBookmark& last = rendered_bookmarks[index];
            if (last.pixmap) {
                keep_size -= last.pixmap->size().width() * last.pixmap->size().height();
                delete last.pixmap;
            }
            index++;
            //rendered_bookmarks.pop_back();
        }
        rendered_bookmarks.erase(rendered_bookmarks.begin(), rendered_bookmarks.begin() + index);

        rendered_bookmarks_mutex.unlock();
    }

}

void BackgroundBookmarkRenderer::copy_microtex_files() {
    QString root_dir = QString::fromStdWString(standard_data_path.slash(L"microtex_resources").get_path()) + "/";

    if (!QDir(root_dir).exists()) {
        QDir().mkpath(root_dir + "");
        QDir().mkpath(root_dir + "cyrillic");
        QDir().mkpath(root_dir + "fonts/base");
        QDir().mkpath(root_dir + "fonts/euler");
        QDir().mkpath(root_dir + "fonts/latin");
        QDir().mkpath(root_dir + "fonts/latin/optional");
        QDir().mkpath(root_dir + "fonts/licences");
        QDir().mkpath(root_dir + "fonts/maths");
        QDir().mkpath(root_dir + "fonts/maths/optional");
        QDir().mkpath(root_dir + "greek");

        QFile::copy(":/microtex_resources/.clatexmath-res_root", root_dir + ".clatexmath-res_root");
        QFile::copy(":/microtex_resources/RES_README", root_dir + "RES_README");
        QFile::copy(":/microtex_resources/SAMPLES.tex", root_dir + "SAMPLES.tex");
        QFile::copy(":/microtex_resources/cyrillic/cyrillic.map.xml", root_dir + "cyrillic/cyrillic.map.xml");
        QFile::copy(":/microtex_resources/cyrillic/language_cyrillic.xml", root_dir + "cyrillic/language_cyrillic.xml");
        QFile::copy(":/microtex_resources/cyrillic/LICENSE", root_dir + "cyrillic/LICENSE");
        QFile::copy(":/microtex_resources/cyrillic/mappings_cyrillic.xml", root_dir + "cyrillic/mappings_cyrillic.xml");
        QFile::copy(":/microtex_resources/cyrillic/symbols_cyrillic.xml", root_dir + "cyrillic/symbols_cyrillic.xml");
        QFile::copy(":/microtex_resources/cyrillic/wnbx10.ttf", root_dir + "cyrillic/wnbx10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wnbx10.xml", root_dir + "cyrillic/wnbx10.xml");
        QFile::copy(":/microtex_resources/cyrillic/wnbxti10.ttf", root_dir + "cyrillic/wnbxti10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wnbxti10.xml", root_dir + "cyrillic/wnbxti10.xml");
        QFile::copy(":/microtex_resources/cyrillic/wnr10.ttf", root_dir + "cyrillic/wnr10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wnr10.xml", root_dir + "cyrillic/wnr10.xml");
        QFile::copy(":/microtex_resources/cyrillic/wnss10.ttf", root_dir + "cyrillic/wnss10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wnss10.xml", root_dir + "cyrillic/wnss10.xml");
        QFile::copy(":/microtex_resources/cyrillic/wnssbx10.ttf", root_dir + "cyrillic/wnssbx10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wnssbx10.xml", root_dir + "cyrillic/wnssbx10.xml");
        QFile::copy(":/microtex_resources/cyrillic/wnssi10.ttf", root_dir + "cyrillic/wnssi10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wnssi10.xml", root_dir + "cyrillic/wnssi10.xml");
        QFile::copy(":/microtex_resources/cyrillic/wnti10.ttf", root_dir + "cyrillic/wnti10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wnti10.xml", root_dir + "cyrillic/wnti10.xml");
        QFile::copy(":/microtex_resources/cyrillic/wntt10.ttf", root_dir + "cyrillic/wntt10.ttf");
        QFile::copy(":/microtex_resources/cyrillic/wntt10.xml", root_dir + "cyrillic/wntt10.xml");
        QFile::copy(":/microtex_resources/fonts/base/cmex10.ttf", root_dir + "fonts/base/cmex10.ttf");
        QFile::copy(":/microtex_resources/fonts/base/cmmi10.ttf", root_dir + "fonts/base/cmmi10.ttf");
        QFile::copy(":/microtex_resources/fonts/base/cmmib10.ttf", root_dir + "fonts/base/cmmib10.ttf");
        QFile::copy(":/microtex_resources/fonts/euler/eufb10.ttf", root_dir + "fonts/euler/eufb10.ttf");
        QFile::copy(":/microtex_resources/fonts/euler/eufm10.ttf", root_dir + "fonts/euler/eufm10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/bi10.ttf", root_dir + "fonts/latin/bi10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/bx10.ttf", root_dir + "fonts/latin/bx10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/cmr10.ttf", root_dir + "fonts/latin/cmr10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/i10.ttf", root_dir + "fonts/latin/i10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/r10.ttf", root_dir + "fonts/latin/r10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/sb10.ttf", root_dir + "fonts/latin/sb10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/sbi10.ttf", root_dir + "fonts/latin/sbi10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/si10.ttf", root_dir + "fonts/latin/si10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/ss10.ttf", root_dir + "fonts/latin/ss10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/tt10.ttf", root_dir + "fonts/latin/tt10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/optional/cmbx10.ttf", root_dir + "fonts/latin/optional/cmbx10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/optional/cmbxti10.ttf", root_dir + "fonts/latin/optional/cmbxti10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/optional/cmss10.ttf", root_dir + "fonts/latin/optional/cmss10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/optional/cmssbx10.ttf", root_dir + "fonts/latin/optional/cmssbx10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/optional/cmssi10.ttf", root_dir + "fonts/latin/optional/cmssi10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/optional/cmti10.ttf", root_dir + "fonts/latin/optional/cmti10.ttf");
        QFile::copy(":/microtex_resources/fonts/latin/optional/cmtt10.ttf", root_dir + "fonts/latin/optional/cmtt10.ttf");
        QFile::copy(":/microtex_resources/fonts/licences/Knuth_License.txt", root_dir + "fonts/licences/Knuth_License.txt");
        QFile::copy(":/microtex_resources/fonts/licences/License_for_dsrom.txt", root_dir + "fonts/licences/License_for_dsrom.txt");
        QFile::copy(":/microtex_resources/fonts/licences/OFL.txt", root_dir + "fonts/licences/OFL.txt");
        QFile::copy(":/microtex_resources/fonts/maths/cmbsy10.ttf", root_dir + "fonts/maths/cmbsy10.ttf");
        QFile::copy(":/microtex_resources/fonts/maths/cmsy10.ttf", root_dir + "fonts/maths/cmsy10.ttf");
        QFile::copy(":/microtex_resources/fonts/maths/msam10.ttf", root_dir + "fonts/maths/msam10.ttf");
        QFile::copy(":/microtex_resources/fonts/maths/msbm10.ttf", root_dir + "fonts/maths/msbm10.ttf");
        QFile::copy(":/microtex_resources/fonts/maths/rsfs10.ttf", root_dir + "fonts/maths/rsfs10.ttf");
        QFile::copy(":/microtex_resources/fonts/maths/special.ttf", root_dir + "fonts/maths/special.ttf");
        QFile::copy(":/microtex_resources/fonts/maths/stmary10.ttf", root_dir + "fonts/maths/stmary10.ttf");
        QFile::copy(":/microtex_resources/fonts/maths/optional/dsrom10.ttf", root_dir + "fonts/maths/optional/dsrom10.ttf");
        QFile::copy(":/microtex_resources/greek/fcmbipg.ttf", root_dir + "greek/fcmbipg.ttf");
        QFile::copy(":/microtex_resources/greek/fcmbipg.xml", root_dir + "greek/fcmbipg.xml");
        QFile::copy(":/microtex_resources/greek/fcmbpg.ttf", root_dir + "greek/fcmbpg.ttf");
        QFile::copy(":/microtex_resources/greek/fcmbpg.xml", root_dir + "greek/fcmbpg.xml");
        QFile::copy(":/microtex_resources/greek/fcmripg.ttf", root_dir + "greek/fcmripg.ttf");
        QFile::copy(":/microtex_resources/greek/fcmripg.xml", root_dir + "greek/fcmripg.xml");
        QFile::copy(":/microtex_resources/greek/fcmrpg.ttf", root_dir + "greek/fcmrpg.ttf");
        QFile::copy(":/microtex_resources/greek/fcmrpg.xml", root_dir + "greek/fcmrpg.xml");
        QFile::copy(":/microtex_resources/greek/fcsbpg.ttf", root_dir + "greek/fcsbpg.ttf");
        QFile::copy(":/microtex_resources/greek/fcsbpg.xml", root_dir + "greek/fcsbpg.xml");
        QFile::copy(":/microtex_resources/greek/fcsropg.ttf", root_dir + "greek/fcsropg.ttf");
        QFile::copy(":/microtex_resources/greek/fcsropg.xml", root_dir + "greek/fcsropg.xml");
        QFile::copy(":/microtex_resources/greek/fcsrpg.ttf", root_dir + "greek/fcsrpg.ttf");
        QFile::copy(":/microtex_resources/greek/fcsrpg.xml", root_dir + "greek/fcsrpg.xml");
        QFile::copy(":/microtex_resources/greek/fctrpg.ttf", root_dir + "greek/fctrpg.ttf");
        QFile::copy(":/microtex_resources/greek/fctrpg.xml", root_dir + "greek/fctrpg.xml");
        QFile::copy(":/microtex_resources/greek/greek.map.xml", root_dir + "greek/greek.map.xml");
        QFile::copy(":/microtex_resources/greek/language_greek.xml", root_dir + "greek/language_greek.xml");
        QFile::copy(":/microtex_resources/greek/LICENSE", root_dir + "greek/LICENSE");
        QFile::copy(":/microtex_resources/greek/mappings_greek.xml", root_dir + "greek/mappings_greek.xml");
        QFile::copy(":/microtex_resources/greek/symbols_greek.xml", root_dir + "greek/symbols_greek.xml");
    }

}

void BackgroundBookmarkRenderer::release_cache() {
    rendered_bookmarks_mutex.lock();
    for (auto bookmark : rendered_bookmarks) {
        if (bookmark.pixmap) {
            delete bookmark.pixmap;
            bookmark.pixmap = nullptr;
        }
    }
    rendered_bookmarks.clear();
    rendered_bookmarks_mutex.unlock();
}

std::optional<RenderedBookmark> BackgroundBookmarkRenderer::get_request_with_id(int id) {
    rendered_bookmarks_mutex.lock_shared();
    for (auto bookmark : rendered_bookmarks) {
        if (bookmark.request_id == id) {
            rendered_bookmarks_mutex.unlock_shared();
            return bookmark;
        }
    }

    rendered_bookmarks_mutex.unlock_shared();
    return {};
}

void BackgroundBookmarkRenderer::erase_request_with_id(int id) {
    rendered_bookmarks_mutex.lock();

    int index = -1;

    for (int i = 0; i < rendered_bookmarks.size(); i++) {
        if (rendered_bookmarks[i].request_id == id) {
            index = i;
            break;
        }

    }
    if (index >= 0) {
        rendered_bookmarks.erase(rendered_bookmarks.begin() + index);
    }
    rendered_bookmarks_mutex.unlock();
}


float BackgroundBookmarkRenderer::get_cached_bookmark_height(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(cached_bookmark_heights_mutex);
    auto it = cached_bookmark_heights_.find(uuid);
    if (it != cached_bookmark_heights_.end()) {
        return it->second;
    }
    return 0;
}

MyThread::MyThread(
    bool* should_stop_,
    std::mutex* pending_task_mutex_,
    std::condition_variable* pending_task_cv_,
    std::deque<std::pair<QObject*, std::function<void()>>>* pending_tasks_,
    QObject** current_task_parent_) {
    should_stop = should_stop_;
    pending_tasks_mutex = pending_task_mutex_;
    pending_taks_cv = pending_task_cv_;
    pending_tasks = pending_tasks_;
    current_task_parent = current_task_parent_;

}

void MyThread::run() {
    while (!(*should_stop)) {
        std::unique_lock<std::mutex> lock(*pending_tasks_mutex);
        pending_taks_cv->wait(lock, [&] {
            return pending_tasks->size() > 0 || (*should_stop);
            });
        if ((*should_stop)) {
            break;
        }
        auto [parent, task] = std::move(pending_tasks->front());
        pending_tasks->pop_front();
        *current_task_parent = parent;
        lock.unlock();
        task();
        *current_task_parent = nullptr;
    }
}
