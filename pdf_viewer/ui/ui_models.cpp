#include "ui/ui_models.h"

CommandModel::CommandModel(
    std::vector<QString> commands,
    std::vector<QStringList> keybinds,
    std::vector<bool> requires_pro)
    : commands(commands), keybinds(keybinds), requires_pro(requires_pro){

}

int CommandModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return commands.size();
    }
    return 0;
}

int CommandModel::columnCount(const QModelIndex& parent) const {
    return CommandModel::max_columns;
}

QVariant CommandModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == CommandModel::command_name) {
            return commands[index.row()];
        }
        else if (index.column() == CommandModel::keybind) {
            if (index.row() < keybinds.size()) {
                return keybinds[index.row()];
            }
        }
        else if (index.column() == CommandModel::is_pro) {
            if (index.row() < requires_pro.size()) {
                // return QVariant::fromValue(requires_pro[index.row()]);
                return (bool)requires_pro[index.row()];
            }
            return false;
        }
    }
    return QVariant();
}

QVariant CommandModel::headerData(int section, Qt::Orientation orientation, int role) const {

    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Command";
    }
    return QVariant();
}

QHash<int, QByteArray> HighlightModel::roleNames() const {

    QHash<int, QByteArray> roles;
    roles[HighlightModelColumn::checksum] = "checksum";
    roles[HighlightModelColumn::description] = "description";
    roles[HighlightModelColumn::file_name] = "fileName";
    roles[HighlightModelColumn::type] = "highlightType";
    roles[HighlightModelColumn::text] = "highlightText";
    return roles;

}

HighlightModel::HighlightModel(std::vector<Highlight>&& data, std::vector<QString>&& docs, std::vector<QString>&& doc_checksums, QObject* parent) : QAbstractTableModel(parent) {
    highlights = data;
    documents = docs;
    checksums = doc_checksums;
}

int HighlightModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return highlights.size();
    }
    return 0;
}

int HighlightModel::columnCount(const QModelIndex& parent) const {
    if (documents.size() == 0) {
        return 3;
    }
    else if (checksums.size() == 0) {
        return 4;
    }
    else {
        return 5;
    }
}

QVariant HighlightModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == HighlightModelColumn::text) {
            return QString::fromStdWString(highlights[index.row()].description);
        }
        if (index.column() == HighlightModelColumn::type) {
            return highlights[index.row()].type;
        }
        if (index.column() == HighlightModelColumn::description) {
            return QString::fromStdWString(highlights[index.row()].text_annot);
        }
        if (index.column() == HighlightModelColumn::file_name) {
            return documents[index.row()];
        }
        if (index.column() == HighlightModelColumn::checksum) {
            return checksums[index.row()];
        }
    }

    return QVariant();
}

QVariant HighlightModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Highlight";
    }
    return QVariant();
}

BookmarkModel::BookmarkModel(std::vector<BookMark>&& data, std::vector<QString>&& docs, std::vector<QString>&& doc_checksums, QObject* parent) : QAbstractTableModel(parent) {
    bookmarks = data;
    documents = docs;
    checksums = doc_checksums;
}

int BookmarkModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return bookmarks.size();
    }
    return 0;
}

int BookmarkModel::columnCount(const QModelIndex& parent) const {
    if (documents.size() == 0) {
        return 2;
    }
    else if (checksums.size() == 0) {
        return 3;
    }
    else {
        return 4;
    }
}

QVariant BookmarkModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == BookmarkModelColumn::description) {
            return QString::fromStdWString(bookmarks[index.row()].description);
        }
        if (index.column() == BookmarkModelColumn::bookmark) {
            return QVariant::fromValue(bookmarks[index.row()]);
        }
        if (index.column() == BookmarkModelColumn::file_name) {
            return documents[index.row()];
        }
        if (index.column() == BookmarkModelColumn::checksum) {
            return checksums[index.row()];
        }
    }
    return QVariant();
}

QVariant BookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Bookmark";
    }
    return QVariant();
}

bool BookmarkModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);
    bookmarks.erase(bookmarks.begin() + row, bookmarks.begin() + row + count);

    if (documents.size() > 0) {
        documents.erase(documents.begin() + row, documents.begin() + row + count);
    }

    if (checksums.size() > 0) {
        checksums.erase(checksums.begin() + row, checksums.begin() + row + count);
    }

    endRemoveRows();
    return true;
}

ItemWithDescriptionModel::ItemWithDescriptionModel(std::vector<QString> && items_, std::vector<QString> && descriptions_, std::vector<QString> && metadata_, QObject * parent): QAbstractTableModel(parent) {
    items = items_;
    descriptions = descriptions_;
    metadatas = metadata_;
}

int ItemWithDescriptionModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return items.size();
    }
    return 0;
}

int ItemWithDescriptionModel::columnCount(const QModelIndex& parent) const {
    if (metadatas.size() == 0) {
        return 2;
    }
    return 3;
}

QVariant ItemWithDescriptionModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == ItemWithDescriptionColumn::item_text) {
            return items[index.row()];
        }
        if (index.column() == ItemWithDescriptionColumn::description) {
            return descriptions[index.row()];
        }
        if (index.column() == ItemWithDescriptionColumn::metadata) {
            return metadatas[index.row()];
        }
    }
    return QVariant();
}

QVariant ItemWithDescriptionModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Item";
    }
    return QVariant();
}

bool ItemWithDescriptionModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);
    items.erase(items.begin() + row, items.begin() + row + count);

    if (descriptions.size() > 0) {
        descriptions.erase(descriptions.begin() + row, descriptions.begin() + row + count);
    }

    if (metadatas.size() > 0) {
        metadatas.erase(metadatas.begin() + row, metadatas.begin() + row + count);
    }

    endRemoveRows();
    return true;
}

DocumentNameModel::DocumentNameModel(
    std::vector<OpenedBookInfo>&& books, QObject* parent)
    : QAbstractTableModel(parent) {
    opened_documents = books;
}


int DocumentNameModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return opened_documents.size();
    }
    return 0;
}

int DocumentNameModel::columnCount(const QModelIndex& parent) const {
    return DocumentNameColumn::max_columns;
}

QVariant DocumentNameModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == DocumentNameColumn::file_path) {
            //return file_paths[index.row()];
            return opened_documents[index.row()].file_name;
        }
        if (index.column() == DocumentNameColumn::document_title) {
            //return document_titles[index.row()];
            return opened_documents[index.row()].document_title;
        }
        if (index.column() == DocumentNameColumn::last_access_time) {
            return opened_documents[index.row()].last_access_time;
        }
        if (index.column() == DocumentNameColumn::is_server_only) {
            return opened_documents[index.row()].is_server_only;
        }
    }

    return QVariant();
}

QVariant DocumentNameModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Document";
    }
    return QVariant();
}


bool DocumentNameModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);

    opened_documents.erase(opened_documents.begin() + row, opened_documents.begin() + row + count);

    endRemoveRows();
    return true;
}

FulltextResultModel::FulltextResultModel(std::vector<FulltextSearchResult>&& results, QObject* parent)
    : QAbstractTableModel(parent) {
    search_results = results;
}

int FulltextResultModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return search_results.size();
    }
    return 0;
}

int FulltextResultModel::columnCount(const QModelIndex& parent) const {
    return SearchResultColumn::max_columns;
}

QVariant FulltextResultModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == SearchResultColumn::snippet) {
            return QString::fromStdWString(search_results[index.row()].snippet);
        }
        if (index.column() == SearchResultColumn::document_title) {
            return QString::fromStdWString(search_results[index.row()].document_title);
        }
        if (index.column() == SearchResultColumn::page) {
            return search_results[index.row()].page;
        }
    }

    return QVariant();
}

QVariant FulltextResultModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Search Result";
    }
    return QVariant();
}

bool HighlightModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);
    highlights.erase(highlights.begin() + row, highlights.begin() + row + count);

    if (documents.size() > 0) {
        documents.erase(documents.begin() + row, documents.begin() + row + count);
    }

    if (checksums.size() > 0) {
        checksums.erase(checksums.begin() + row, checksums.begin() + row + count);
    }

    endRemoveRows();
    return true;
}
