#pragma once

#include <QTextBrowser>
#include "ui/common_ui.h"
#include "ui/base_delegate.h"

class MainWidget;
class FulltextResultModel;

class SioyekDocumentationTextBrowser : public QTextBrowser {
private:
    MainWidget* main_widget = nullptr;
protected:
    void wheelEvent(QWheelEvent* wevent) override;
public:
    SioyekDocumentationTextBrowser(MainWidget* parent);

    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void backward() override;
    void forward() override;
    void doSetSource(const QUrl& url, QTextDocument::ResourceType type = QTextDocument::UnknownResource) override;
};

class FulltextSearchWidget : public BaseCustomSelectorWidget{
public:
    DatabaseManager* db_manager = nullptr;
    MainWidget* main_widget = nullptr;
    std::wstring maybe_file_checksum = L"";
    std::wstring maybe_tag = L"";
    FulltextSearchWidget(
        DatabaseManager* manager,
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent,
        std::wstring checksum=L"",
        std::wstring tag=L""
    );
    ~FulltextSearchWidget();

    static FulltextSearchWidget* create(MainWidget* parent, std::wstring checksum=L"", std::wstring tag=L"");

    virtual void on_text_changed(const QString& text) override;
    virtual void on_select(QModelIndex value) override;
    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override;

    //QStringListModel* result_model = nullptr;
    FulltextResultModel* result_model = nullptr;
};

class DocumentationSearchWidget : public FulltextSearchWidget {
public:
    DocumentationSearchWidget(
        DatabaseManager* manager,
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );

    static DocumentationSearchWidget* create(MainWidget* parent);
    virtual void on_text_changed(const QString& text) override;
};

class SearchItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument snippet_document;
    mutable QTextDocument location_document;
    //mutable std::unordered_map<int, float> cached_sizes;

    SearchItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void clear_cache() override;
    QString highlight_match(QString match) const;
    QString get_location_string(const QModelIndex& index) const;
};
