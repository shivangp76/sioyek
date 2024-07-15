#pragma once
#include <QString>
#include <qsortfilterproxymodel.h>
#include <map>
#include <qmap.h>
#include <qlist.h>

extern "C" {
    #include "fzf/fzf.h"
}

class MySortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    QString ignore_prefix = "";
    QString filterString;
    std::optional<int> filter_type = {};
    mutable std::vector<float> scores;
    mutable QString last_update_string;
    bool is_fuzzy = false;
    bool is_highlight = false;
    fzf_slab_t* slab;
    mutable QMap<QModelIndex, int> index_map;
    bool is_tree = false;

    MySortFilterProxyModel(bool fuzzy, bool is_tree);
    ~MySortFilterProxyModel();
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const;
    bool filter_accepts_row_column(int row, int col, const QModelIndex& source_parent) const;
    Q_INVOKABLE void setFilterCustom(const QString& filterString);
    Q_INVOKABLE QList<int> get_highlight_positions(QString haystack, QString needle);

    //void setFilterFixedString(const QString &pattern) override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

    float compute_score(QString filter_string, QString item_string) const;
    float compute_score(fzf_pattern_t* pattern, QString item_string) const;
    float update_scores_for_index(fzf_pattern_t* pattern, const QModelIndex& index, int col) const;
    void ensure_scores() const;
    void update_scores() const;
    void set_ignore_prefix(QString prefix);
    void set_is_highlight(bool is_hl);

};
