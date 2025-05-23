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

void get_pixmap_pixel(fz_pixmap* pixmap, int x, int y, unsigned char* r, unsigned char* g, unsigned char* b) {
    if (
        (x < 0) ||
        (y < 0) ||
        (x >= pixmap->w) ||
        (y >= pixmap->h)
        ) {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    (*r) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 0];
    (*g) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 1];
    (*b) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 2];
}

int find_max_horizontal_line_length_at_pos(fz_pixmap* pixmap, int pos_x, int pos_y) {
    int min_x = pos_x;
    int max_x = pos_x;

    while (true) {
        unsigned char r, g, b;
        get_pixmap_pixel(pixmap, min_x, pos_y, &r, &g, &b);
        if ((r != 255) || (g != 255) || (b != 255)) {
            break;
        }
        else {
            min_x--;
        }
    }
    while (true) {
        unsigned char r, g, b;
        get_pixmap_pixel(pixmap, max_x, pos_y, &r, &g, &b);
        if ((r != 255) || (g != 255) || (b != 255)) {
            break;
        }
        else {
            max_x++;
        }
    }
    return max_x - min_x;
}

bool largest_contigous_ones(std::vector<int>& arr, int* start_index, int* end_index) {
    arr.push_back(0);

    bool has_at_least_one_one = false;

    for (auto x : arr) {
        if (x == 1) {
            has_at_least_one_one = true;
        }
    }
    if (!has_at_least_one_one) {
        return false;
    }


    int max_count = 0;
    int max_start_index = -1;
    int max_end_index = -1;

    int count = 0;

    for (size_t i = 0; i < arr.size(); i++) {
        if (arr[i] == 1) {
            count++;
        }
        else {
            if (count > max_count) {
                max_count = count;
                max_end_index = i - 1;
                max_start_index = i - count;
            }
            count = 0;
        }
    }

    *start_index = max_start_index;
    *end_index = max_end_index;
    return true;
}


template<typename T>
float average_value(std::vector<T> values) {
    T sum = 0;
    for (auto x : values) {
        sum += x;
    }
    return static_cast<float>(sum) / values.size();
}

template<typename T>
float standard_deviation(std::vector<T> values, float average_value) {
    T sum = 0;
    for (auto x : values) {
        sum += (x - average_value) * (x - average_value);
    }
    return std::sqrt(static_cast<float>(sum) / values.size());
}

//std::vector<unsigned int> get_line_ends_from_histogram(std::vector<unsigned int> histogram) {
void get_line_begins_and_ends_from_histogram(std::vector<unsigned int> histogram, std::vector<unsigned int>& res_begins, std::vector<unsigned int>& res) {

    float mean_width = average_value(histogram);
    float std = standard_deviation(histogram, mean_width);

    std::vector<float> normalized_histogram;

    if (std < 0.00001f) {
        return;
    }

    for (auto x : histogram) {
        normalized_histogram.push_back((x - mean_width) / std);
    }

    size_t i = 0;

    while (i < histogram.size()) {

        while ((i < histogram.size()) && (normalized_histogram[i] > 0.2f)) i++;
        res_begins.push_back(i);
        while ((i < histogram.size()) && (normalized_histogram[i] <= 0.21f)) i++;
        if (i == histogram.size()) break;
        res.push_back(i);
    }

    while (res_begins.size() > res.size()) {
        res_begins.pop_back();
    }

    float additional_distance = 0.0f;

    if (res.size() > 5) {
        std::vector<float> line_distances;

        for (size_t i = 0; i < res.size() - 1; i++) {
            line_distances.push_back(res[i + 1] - res[i]);
        }
        std::nth_element(line_distances.begin(), line_distances.begin() + line_distances.size() / 2, line_distances.end());
        additional_distance = line_distances[line_distances.size() / 2];

        for (size_t i = 0; i < res.size(); i++) {
            res[i] += static_cast<unsigned int>(additional_distance / 5.0f);
            res_begins[i] -= static_cast<unsigned int>(additional_distance / 5.0f);
        }

    }

}

int find_best_vertical_line_location(fz_pixmap* pixmap, int doc_x, int doc_y) {


    int search_height = 5;

    float min_candid_y = doc_y;
    float max_candid_y = doc_y + search_height;

    std::vector<int> max_possible_widths;

    for (int candid_y = min_candid_y; candid_y <= max_candid_y; candid_y++) {
        int current_width = find_max_horizontal_line_length_at_pos(pixmap, doc_x, candid_y);
        max_possible_widths.push_back(current_width);
    }

    int max_width_value = -1;

    for (auto w : max_possible_widths) {
        if (w > max_width_value) {
            max_width_value = w;
        }
    }
    std::vector<int> is_max_list;

    for (auto x : max_possible_widths) {
        if (x == max_width_value) {
            is_max_list.push_back(1);
        }
        else {
            is_max_list.push_back(0);
        }
    }

    int start_index, end_index;
    largest_contigous_ones(is_max_list, &start_index, &end_index);

    //return doc_y + (start_index + end_index) / 2;
    return doc_y + start_index;
}

std::vector<unsigned int> get_max_width_histogram_from_pixmap(fz_pixmap* pixmap) {
    std::vector<unsigned int> res;

    for (int j = 0; j < pixmap->h; j++) {
        unsigned int x_value = 0;
        for (int i = 0; i < pixmap->w; i++) {
            unsigned char r, g, b;
            get_pixmap_pixel(pixmap, i, j, &r, &g, &b);
            float lightness = (static_cast<float>(r) + static_cast<float>(g) + static_cast<float>(b)) / 3.0f;

            if (lightness > 150) {
                x_value += 1;
            }
        }
        res.push_back(x_value);
    }

    return res;
}
