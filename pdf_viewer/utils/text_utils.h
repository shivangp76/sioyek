#pragma once
#include <utility>
#include <cassert>
#include <unordered_map>
#include <vector>

template<typename T>
int lcs(T X, T Y, int m, int n)
{
    std::vector<std::vector<int>> L;
    L.reserve(m + 1);
    for (int i = 0; i < m + 1; i++) {
        L.push_back(std::vector<int>(n + 1));
    }

    int i, j;

    /* Following steps build L[m+1][n+1] in bottom up fashion. Note
      that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
    for (i = 0; i <= m; i++) {
        for (j = 0; j <= n; j++) {
            if (i == 0 || j == 0)
                L[i][j] = 0;

            else if (X[i - 1] == Y[j - 1])
                L[i][j] = L[i - 1][j - 1] + 1;

            else
                L[i][j] = std::max(L[i - 1][j], L[i][j - 1]);
        }
    }

    /* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
    return L[m][n];
}


template<typename T>
int lcs_small_optimized(T X, T Y, int m, int n)
{
    const int SIZE = 128;
    thread_local int L[SIZE * SIZE];

    if ((m+1) > SIZE || (n+1) > SIZE) {
        return lcs<T>(X, Y, m, n);
    }
    //std::vector<std::vector<int>> L;
    //L.reserve(m + 1);
    //for (int i = 0; i < m + 1; i++) {
    //    L.push_back(std::vector<int>(n + 1));
    //}

    int i, j;

    /* Following steps build L[m+1][n+1] in bottom up fashion. Note
      that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
    for (i = 0; i <= m; i++) {
        for (j = 0; j <= n; j++) {
            if (i == 0 || j == 0) {
                L[i * SIZE + j] = 0;
            }
            else if (X[i - 1] == Y[j - 1]) {
                L[i * SIZE + j] = L[(i - 1) * SIZE + j - 1] + 1;
            }
            else {
                L[i * SIZE + j] = std::max(L[(i - 1) * SIZE + j], L[i* SIZE + j - 1]);
            }
        }
    }

    /* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
    return L[m * SIZE + n];
}

template<typename T>
std::pair<int, int> find_smallest_containing_substring_ascii(const T& haystack, const T& needle, wchar_t delimeter=' ') {
    int chars_left[256] = { 0 };
    int positive_count = 0;
    for (auto ch : needle) {
        if (ch == delimeter) continue;

        chars_left[ch]++;
        if (chars_left[ch] == 1) {
            positive_count++;
        }
    }
    if (positive_count == 0) {
        return std::make_pair(-1, -1);
    }


    int start = 0;
    int end = -1;

    auto move_start_keeping_match = [&]() {
        while (start < end) {
            //auto it = chars_left.find(haystack[start]);
            //if (it != chars_left.end()) {
            if (chars_left[haystack[start]] < 0) {
                chars_left[haystack[start]]++;
            }
            else {
                break;
            }
            //}
            start++;
        }
        };

    auto move_end_until_match = [&]() {
        while (end < static_cast<int>(haystack.size()) && positive_count > 0) {
            end++;
            //auto it = chars_left.find(haystack[end]);
            //if (it != chars_left.end()) {
            chars_left[haystack[end]]--;
                //it->second--;
            if (chars_left[haystack[end]] == 0) {
                positive_count--;
            }
            //}
        }
        move_start_keeping_match();
        };

    auto increment_start = [&]() {
        chars_left[haystack[start]]++;
        if (chars_left[haystack[start]] == 1) {
            positive_count++;
            start++;
        }
        else {
            assert(false);
        }
        };

    move_end_until_match();
    if (positive_count > 0) {
        return std::make_pair(-1, -1);
    }

    int min_length = end - start + 1;
    int min_start = start;
    int min_end = end;

    while (true) {
        increment_start();
        move_end_until_match();
        if (positive_count > 0) break;
        int length = end - start + 1;
        if (length < min_length) {
            min_length = length;
            min_start = start;
            min_end = end;
        }

    }
    return std::make_pair(min_start, min_end+1);

}

template<typename T>
std::pair<int, int> find_smallest_containing_substring_unicode(const T& haystack, const T& needle, wchar_t delimeter=' ') {
    std::unordered_map<int, int> chars_left;
    for (auto ch : needle) {
        if (ch == delimeter) continue;
        if (ch == '\n') continue;

        auto it = chars_left.find(ch);
        if (it == chars_left.end()) {
            chars_left[ch] = 1;
        }
        else {
            it->second++;
        }
    }
    int positive_count = chars_left.size();
    if (positive_count == 0) {
        return std::make_pair(-1, -1);
    }

    int start = 0;
    int end = -1;

    auto move_start_keeping_match = [&]() {
        while (start < end) {
            auto it = chars_left.find(haystack[start]);
            if (it != chars_left.end()) {
                if (it->second < 0) {
                    it->second++;
                }
                else {
                    break;
                }
            }
            start++;
        }
        };

    auto move_end_until_match = [&]() {
        while (end < static_cast<int>(haystack.size()) && positive_count > 0) {
            end++;
            auto it = chars_left.find(haystack[end]);
            if (it != chars_left.end()) {
                it->second--;
                if (it->second == 0) {
                    positive_count--;
                }
            }
        }
        move_start_keeping_match();
        };

    auto increment_start = [&]() {
        chars_left[haystack[start]]++;
        if (chars_left[haystack[start]] == 1) {
            positive_count++;
            start++;
        }
        else {
            assert(false);
        }
        };

    move_end_until_match();
    if (positive_count > 0) {
        return std::make_pair(-1, -1);
    }

    int min_length = end - start + 1;
    int min_start = start;
    int min_end = end;

    while (true) {
        increment_start();
        move_end_until_match();
        if (positive_count > 0) break;
        int length = end - start + 1;
        if (length < min_length) {
            min_length = length;
            min_start = start;
            min_end = end;
        }

    }
    return std::make_pair(min_start, min_end+1);

}

template<typename T>
bool has_unicode(const T& str) {
    for (int ch : str) {
        if (ch > 255) {
            return true;
        }
    }
    return false;
}
template<typename T>
float similarity_score(const T& haystack, const T& needle, int* out_begin = nullptr, int* out_end = nullptr, float size_threshold=0.5f) {
    bool unicode = has_unicode(haystack) || has_unicode(needle);

    float size_discount_factor = 1.0f / (haystack.size() > 0 ? static_cast<float>(haystack.size()) : 1.0f);
    if (needle.size() == 0) {
        return 100;
    }
    if (haystack.size() == 0) {
        return 0;
    }
    if (haystack == needle) {
        if (out_begin) {
            *out_begin = 0;
        }
        if (out_end) {
            *out_end = haystack.size();
        }
        return 110;
    }
    auto [begin, end] = unicode ? find_smallest_containing_substring_unicode<T>(haystack, needle) : find_smallest_containing_substring_ascii<T>(haystack, needle);
    if (out_begin && out_end) {
        *out_begin = begin;
        *out_end = end;
    }

    if (begin == -1) {
        int lcs_length = lcs_small_optimized(&haystack[0], &needle[0], haystack.length(), needle.size());
        if (lcs_length < needle.size() / 2) {
            return 0;
        }
        return lcs_length * 20 / needle.size();
    }

    int length = end - begin;

    if (static_cast<int>(length * size_threshold) > needle.size()) {
        return 0;
    }

    int lcs_length = lcs_small_optimized(&haystack[begin], &needle[0], length, needle.size());
    return lcs_length * 100 / length + size_discount_factor;
}
