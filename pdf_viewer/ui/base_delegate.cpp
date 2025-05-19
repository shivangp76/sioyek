#include "ui/base_delegate.h"
#include "utils/text_utils.h"


extern std::wstring MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE;

int BaseCustomDelegate::similarity_score_cached(QString haystack, QString needle, int* match_begin, int* match_end, float ratio) const{
    std::tuple<QString, QString, float> key = std::make_tuple(haystack, needle, ratio);
    auto cached_result = cached_similarity_scores.find(key);
    if (cached_result != cached_similarity_scores.end()) {
        *match_begin = cached_result->second.highlight_begin;
        *match_end = cached_result->second.highlight_end;
        return cached_result->second.score;
    }

    int score = similarity_score(haystack.toStdWString(), needle.toStdWString(), match_begin, match_end, ratio);
    SimilarityScoreResult result;
    result.score = score;
    result.highlight_begin = *match_begin;
    result.highlight_end = *match_end;
    cached_similarity_scores[key] = result;

    return score;

}

void BaseCustomDelegate::set_pattern(QString p) {
    pattern = p.toLower();
}

QString BaseCustomDelegate::highlight_pattern(QString txt) const{
    int text_highlight_begin = 0;
    int text_highlight_end = 0;
    int text_similarity = similarity_score_cached(txt.toLower(), pattern, &text_highlight_begin, &text_highlight_end, 0.8f);
    const int similarity_threshold = 70;
    if (text_similarity > similarity_threshold) {
        txt = txt.left(text_highlight_begin) + "<span style=\"" + QString::fromStdWString(MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE) + "\">" + txt.mid(text_highlight_begin, text_highlight_end - text_highlight_begin) + "</span>" + txt.mid(text_highlight_end);
    }
    return txt;
}
