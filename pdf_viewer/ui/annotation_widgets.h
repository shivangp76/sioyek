#pragma once
#include "ui/common_ui.h"
#include "ui/base_delegate.h"
#include <QTextDocument>

class BookmarkModel;
class HighlightModel;

class BookmarkSearchItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument bookmark_document;
    mutable QTextDocument file_name_document;
    mutable std::unordered_map<int, float> cached_sizes;

    BookmarkSearchItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    //QString get_display_text(const QString& highlight_text, int highlight_type, QString type_text_color="#000000", QString type_label_bg = "#ffffff") const;
    void clear_cache();
};

class BookmarkSelectorWidget : public BaseCustomSelectorWidget{
private:
    BookmarkSelectorWidget(
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );
public:

    static BookmarkSelectorWidget* from_bookmarks(std::vector<BookMark>&& bookmarks, MainWidget* parent, std::vector<QString>&& doc_names = {}, std::vector<QString>&& doc_checksums = {});

    BookmarkModel* bookmark_model = nullptr;
};

class HighlightSearchItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument highlight_document;
    mutable QTextDocument file_name_document;
    mutable QTextDocument comment_document;
    mutable std::unordered_map<int, float> cached_sizes;

    HighlightSearchItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QString get_display_text(const QString& highlight_text, int highlight_type, QString type_text_color="#000000", QString type_label_bg = "#ffffff") const;
    void clear_cache();
    void set_pattern(QString p) override;
};

class HighlightSelectorWidget : public BaseCustomSelectorWidget{
private:
    HighlightSelectorWidget(
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );
public:

    static HighlightSelectorWidget* from_highlights(std::vector<Highlight>&& highlights, MainWidget* parent, std::vector<QString>&& doc_names = {}, std::vector<QString>&& doc_checksums = {});

    HighlightModel* highlight_model = nullptr;

    //bool on_text_change(const QString& text) override;
};
