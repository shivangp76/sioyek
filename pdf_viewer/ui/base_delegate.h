#pragma once
#include <QStyledItemDelegate>

struct SimilarityScoreResult {
    int score;
    int highlight_begin;
    int highlight_end;
};

class BaseCustomDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    QString pattern;

    mutable std::map<std::tuple<QString, QString, float>, SimilarityScoreResult> cached_similarity_scores;


    virtual void set_pattern(QString p);
    virtual void clear_cache() = 0;
    QString highlight_pattern(QString txt) const;

    int similarity_score_cached(QString haystack, QString needle, int* match_begin, int* match_end, float ratio=0.5f) const;
};

