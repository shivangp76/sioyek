#include <QStandardItemModel>

#include "utils/model_utils.h"
#include "book.h"

QStandardItemModel* create_table_model(const std::vector<std::vector<std::wstring>> column_texts) {
    QStandardItemModel* model = new QStandardItemModel();
    if (column_texts.size() == 0) {
        return model;
    }
    int num_rows = column_texts[0].size();
    for (int i = 1; i < column_texts.size(); i++) {
        assert(column_texts[i].size() == num_rows);
    }

    for (int i = 0; i < num_rows; i++) {
        QList<QStandardItem*> items;
        for (int j = 0; j < column_texts.size(); j++) {
            QStandardItem* item = new QStandardItem(QString::fromStdWString(column_texts[j][i]));

            if (j == (column_texts.size() - 1)) {
                item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
            }
            else {
                item->setTextAlignment(Qt::AlignVCenter);
            }
            items.append(item);
        }
        model->appendRow(items);
    }
    return model;
}


QStandardItemModel* create_table_model(std::vector<std::wstring> lefts, std::vector<std::wstring> rights) {
    QStandardItemModel* model = new QStandardItemModel();

    assert(lefts.size() == rights.size());

    for (size_t i = 0; i < lefts.size(); i++) {
        QStandardItem* name_item = new QStandardItem(QString::fromStdWString(lefts[i]));
        QStandardItem* key_item = new QStandardItem(QString::fromStdWString(rights[i]));
        key_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        model->appendRow(QList<QStandardItem*>() << name_item << key_item);
    }
    return model;
}

QStandardItem* get_item_tree_from_toc_helper(const std::vector<TocNode*>& children, QStandardItem* parent) {

    for (const auto* child : children) {
        QStandardItem* child_item = new QStandardItem(QString::fromStdWString(child->title));
        QStandardItem* page_item = new QStandardItem("[ " + QString::number(child->page) + " ]");
        child_item->setData(child->page);
        page_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

        get_item_tree_from_toc_helper(child->children, child_item);
        parent->appendRow(QList<QStandardItem*>() << child_item << page_item);
    }
    return parent;
}

QStandardItemModel* get_model_from_toc(const std::vector<TocNode*>& roots) {

    QStandardItemModel* model = new QStandardItemModel();
    get_item_tree_from_toc_helper(roots, model->invisibleRootItem());
    return model;
}
