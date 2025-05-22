#include <QImage>
#include "utils/image_utils.h"

extern float AUTO_BOOKMARK_VERTICAL_MARGIN;
extern float AUTO_BOOKMARK_HORIZONTAL_MARGIN;

struct MaximumRectangleResult {
    int area;
    int begin_row;
    int end_row;
    int begin_col;
    int end_col;
};

struct LargestRectangleOutput {
    std::vector<int> areas;
    std::vector<int> begin_indices;
    std::vector<int> end_indices;
};

struct LargestRectangleOutputMatrix {
    std::vector<std::vector<int>> areas;
    std::vector<std::vector<int>> begin_indices;
    std::vector<std::vector<int>> end_indices;
};

std::vector<std::vector<int>> histogram_matrix_from_bool_matrix(const std::vector<std::vector<bool>>& bmatrix) {
    std::vector<std::vector<int>> res;
    for (int i = 0; i < bmatrix.size(); i++) {
        std::vector<int> row;
        for (int j = 0; j < bmatrix[0].size(); j++) {
            row.push_back(0);
        }
        res.push_back(row);
    }

    for (int i = 0; i < bmatrix.size(); i++) {
        for (int j = 0; j < bmatrix[0].size(); j++) {
            if (bmatrix[i][j]) {
                if (i > 0) {
                    res[i][j] = res[i - 1][j] + 1;
                }
                else {
                    res[i][j] = 1;
                }

            }
        }
    }
    return res;
}

std::vector<int> largest_rectangle_helper(const std::vector<int>& heights) {
    std::vector<int> smaller_indices;
    std::vector<int> offsets;

    smaller_indices.push_back(-1);

    for (int i = 0; i < heights.size(); i++) {
        int current_height = heights[i];
        while (smaller_indices.back() != -1 && heights[smaller_indices.back()] >= current_height) {
            smaller_indices.pop_back();
        }
        offsets.push_back((i - 1 - smaller_indices.back()));
        smaller_indices.push_back(i);
    }

    return offsets;
}


LargestRectangleOutput largest_rectangle(const std::vector<int>& heights){
    LargestRectangleOutput res;

    std::vector<int> reversed;
    reversed.reserve(heights.size());
    for (int i = 0; i < heights.size(); i++) {
        reversed.push_back(heights[heights.size() - 1 - i]);
    }

    std::vector<int> prev_offsets = largest_rectangle_helper(heights);
    std::vector<int> next_offsets = largest_rectangle_helper(reversed);


    for (int i = 0; i < heights.size(); i++) {
        int begin_index = i - prev_offsets[i];
        int end_index = i + next_offsets[heights.size() - 1 - i];
        int area = heights[i] * (end_index - begin_index + 1);
        res.areas.push_back(area);
        res.begin_indices.push_back(begin_index);
        res.end_indices.push_back(end_index);
    }
    return res;
}

LargestRectangleOutputMatrix areas_from_hist(const std::vector<std::vector<int>>& hist_matrix) {
    LargestRectangleOutputMatrix res;

    for (int i = 0; i < hist_matrix.size(); i++) {
        LargestRectangleOutput row_areas = largest_rectangle(hist_matrix[i]);
        res.areas.push_back(row_areas.areas);
        res.begin_indices.push_back(row_areas.begin_indices);
        res.end_indices.push_back(row_areas.end_indices);
    }
    return res;

}


std::vector<MaximumRectangleResult> maximum_rectangle(std::vector<std::vector<bool>>& rect) {
    auto hist = histogram_matrix_from_bool_matrix(rect);
    LargestRectangleOutputMatrix largest_rectangles = areas_from_hist(hist);
    MaximumRectangleResult res;
    res.area = 0;

    for (int i = 0; i < largest_rectangles.areas.size(); i++) {

        for (int j = 0; j < largest_rectangles.areas[0].size(); j++) {
            if (largest_rectangles.areas[i][j] > res.area) {
                res.area = largest_rectangles.areas[i][j];
                res.begin_col = largest_rectangles.begin_indices[i][j];
                res.end_col = largest_rectangles.end_indices[i][j];
                res.end_row = i;
                res.begin_row = i - res.area / (res.end_col - res.begin_col + 1);
            }
        }
    }
    int threshold = res.area / 4;

    std::vector<std::pair<int, int>> large_non_dominated_indices;

    for (int i = 0; i < largest_rectangles.areas.size(); i++) {
        for (int j = 0; j < largest_rectangles.areas[0].size(); j++) {
            int index_area = largest_rectangles.areas[i][j];
            if (index_area < threshold) continue;
            if (i < largest_rectangles.areas.size() - 1) {
                int next_row_area = largest_rectangles.areas[i + 1][j];
                if (next_row_area >= index_area) continue;
            }
            if (j < largest_rectangles.areas[0].size() - 1) {
                int next_col_area = largest_rectangles.areas[i][j+1];
                if (next_col_area >= index_area) continue;
            }
            large_non_dominated_indices.push_back(std::make_pair(i, j));
        }
    }

    std::vector<MaximumRectangleResult> large_results;
    for (auto [i, j] : large_non_dominated_indices) {
        MaximumRectangleResult index_result;
        index_result.area = largest_rectangles.areas[i][j];
        index_result.begin_col = largest_rectangles.begin_indices[i][j];
        index_result.end_col = largest_rectangles.end_indices[i][j];
        index_result.end_row = i;
        index_result.begin_row = i - index_result.area / (index_result.end_col - index_result.begin_col + 1);
        large_results.push_back(index_result);

    }

    std::sort(large_results.begin(), large_results.end(), [&](const MaximumRectangleResult& lhs, const MaximumRectangleResult& rhs) {
        return lhs.area > rhs.area;
        });

    while (large_results.size() > 20) {
        large_results.pop_back();
    }

    std::vector<MaximumRectangleResult> final_results;
    final_results.push_back(large_results[0]);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < large_results.size(); j++) {
            bool is_acceptable = true;
            for (int k = 0; k < final_results.size(); k++) {
                WindowRect current_rect;
                current_rect.x0 = large_results[j].begin_col;
                current_rect.x1 = large_results[j].end_col;
                current_rect.y0 = large_results[j].begin_row;
                current_rect.y1 = large_results[j].end_row;
                int current_area = current_rect.area();

                WindowRect accepted_rect;
                accepted_rect.x0 = final_results[k].begin_col;
                accepted_rect.x1 = final_results[k].end_col;
                accepted_rect.y0 = final_results[k].begin_row;
                accepted_rect.y1 = final_results[k].end_row;
                int accepted_area = accepted_rect.area();

                auto intersection_rect = current_rect.intersect_rect(accepted_rect);
                int intersection_area = intersection_rect.area();
                int threshold = std::min(current_area, accepted_area) / 2;
                if (intersection_area > threshold) {
                    is_acceptable = false;
                }
            }
            if (is_acceptable) {
                final_results.push_back(large_results[j]);
                break;
            }
        }
    }

    return final_results;

}

std::vector<WindowRect> get_largest_empty_rects_from_rendered_image(QImage& image, int status_bar_height) {
    const int REDUCE_FACTOR = 8;
    image = image.scaled(image.width() / REDUCE_FACTOR, image.height() / REDUCE_FACTOR);

    std::unordered_map<int, int> counts;

    QRgb mode_color = image.pixel(QPoint(0, 0));
    int mode_count = 1;

    for (int j = 0; j < image.height(); j++) {
        for (int i = 0; i < image.width(); i++) {
            QRgb pixel = image.pixel(QPoint(i, j));

            if (counts.find(pixel) == counts.end()) {
                counts[pixel] = 1;
            }
            else {
                counts[pixel] += 1;
            }
            if (counts[pixel] > mode_count) {
                mode_count = counts[pixel];
                mode_color = pixel;
            }
        }
    }

    std::vector<std::vector<bool>> binary_matrix;
    for (int j = 0; j < image.height(); j++) {
        std::vector<bool> row;
        for (int i = 0; i < image.width(); i++) {
            QRgb pixel = image.pixel(QPoint(i, j));
            if (pixel == mode_color) {
                row.push_back(true);
            }
            else {
                row.push_back(false);
            }
        }
        binary_matrix.push_back(row);
    }

    std::vector<MaximumRectangleResult> largest_rects = maximum_rectangle(binary_matrix);


    std::vector<WindowRect> window_rects;
    for (auto largest_rect : largest_rects) {
        WindowRect window_rect;
        window_rect.x0 = largest_rect.begin_col * REDUCE_FACTOR + AUTO_BOOKMARK_HORIZONTAL_MARGIN;
        window_rect.x1 = largest_rect.end_col * REDUCE_FACTOR - AUTO_BOOKMARK_HORIZONTAL_MARGIN;
        window_rect.y0 = largest_rect.begin_row * REDUCE_FACTOR + AUTO_BOOKMARK_VERTICAL_MARGIN;
        window_rect.y1 = largest_rect.end_row * REDUCE_FACTOR - AUTO_BOOKMARK_VERTICAL_MARGIN;
        if (window_rect.y0 < status_bar_height) {
            window_rect.y0 = status_bar_height;
        }
        window_rects.push_back(window_rect);
    }
    return window_rects;
}
