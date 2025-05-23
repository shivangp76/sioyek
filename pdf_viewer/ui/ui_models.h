#pragma once
#include <QAbstractTableModel>
#include "book.h"
#include "types/annotation_types.h"

class CommandModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum CommandModelColumn{
        command_name = 0,
        keybind = 1,
        is_pro = 2,
        max_columns = 3
    };

    std::vector<QString> commands;
    std::vector<QStringList> keybinds;
    std::vector<bool> requires_pro;


    CommandModel(std::vector<QString> commands, std::vector<QStringList> keybinds, std::vector<bool> requires_pro);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};

class HighlightModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum HighlightModelColumn {
        text = 0,
        type = 1,
        description = 2,
        file_name = 3,
        checksum = 4,
        max_columns = 5,
    };

    std::vector<Highlight> highlights;
    std::vector<QString> documents;
    std::vector<QString> checksums;


    HighlightModel(std::vector<Highlight>&& data, std::vector<QString>&& documents = {}, std::vector<QString>&& checksums = {}, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;


};

class BookmarkModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum BookmarkModelColumn {
        description = 0,
        bookmark = 1,
        file_name = 2,
        checksum = 3,
        max_columns = 4
    };

    std::vector<BookMark> bookmarks;
    std::vector<QString> documents;
    std::vector<QString> checksums;


    BookmarkModel(std::vector<BookMark>&& data, std::vector<QString>&& documents = {}, std::vector<QString>&& checksums = {}, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
};

class ItemWithDescriptionModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum ItemWithDescriptionColumn {
        item_text = 0,
        description = 1,
        metadata = 2,
        max_columns = 4
    };

    std::vector<QString> items;
    std::vector<QString> descriptions;
    std::vector<QString> metadatas;


    ItemWithDescriptionModel(std::vector<QString>&& items, std::vector<QString>&& descriptions, std::vector<QString>&& metadata, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

};

class DocumentNameModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum DocumentNameColumn {
        file_path = 0,
        document_title = 1,
        last_access_time = 2,
        is_server_only = 3,
        max_columns = 4,
    };

    std::vector<OpenedBookInfo> opened_documents;
    //std::vector<QString> document_titles;
    //std::vector<QDateTime> last_access_times;


    DocumentNameModel(std::vector<OpenedBookInfo>&& books, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

};

class FulltextResultModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum SearchResultColumn {
        snippet = 0,
        document_title = 1,
        page = 2,
        max_columns = 3,
    };

    std::vector<FulltextSearchResult> search_results;


    FulltextResultModel(std::vector<FulltextSearchResult>&& results, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

};
