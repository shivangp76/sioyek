#include "utils/drawing_utils.h"

struct AlmostAxisAlignedLine{
    float c;
    float strength;
};

struct TwoCenterResult{
    float smaller_center;
    float smaller_center_strength;
    float larger_center;
    float larger_center_strength;
};

TwoCenterResult weighted_one_dimensional_two_center(const std::vector<AlmostAxisAlignedLine>& points){
    float larger_center = points[0].c;
    float smaller_center = points[0].c;
    float smaller_center_strength = points[0].strength;
    float larger_center_strength = points[0].strength;

    for (int i = 1; i < points.size(); i++){
        if (points[i].c > larger_center) larger_center = points[i].c;
        if (points[i].c < smaller_center) smaller_center = points[i].c;
    }

    int NUM_ITERATIONS = 10;
    for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++){
        float sum_points_closer_to_smaller_center = 0;
        float sum_weights_closer_to_smaller_center = 0;
        float sum_points_closer_to_larger_center = 0;
        float sum_weights_closer_to_larger_center = 0;

        for (const AlmostAxisAlignedLine& p : points){
            if (std::abs(p.c - smaller_center)  < std::abs(p.c - larger_center)){
                sum_points_closer_to_smaller_center += p.c * p.strength;
                sum_weights_closer_to_smaller_center += p.strength;
            }
            else{
                sum_points_closer_to_larger_center += p.c * p.strength;
                sum_weights_closer_to_larger_center += p.strength;
            }
        }

        smaller_center = sum_points_closer_to_smaller_center / sum_weights_closer_to_smaller_center;
        larger_center = sum_points_closer_to_larger_center / sum_weights_closer_to_larger_center;

        smaller_center_strength = sum_weights_closer_to_smaller_center;
        larger_center_strength = sum_weights_closer_to_larger_center;
    }

    TwoCenterResult result;
    result.smaller_center = smaller_center;
    result.larger_center = larger_center;
    result.smaller_center_strength = smaller_center_strength;
    result.larger_center_strength = larger_center_strength;
    return result;
}


struct Line2D {
    float nx;
    float ny;
    float c;
};

float point_distance_from_line(AbsoluteDocumentPos point, Line2D line) {
    return std::abs(line.nx * point.x + line.ny * point.y - line.c);
}

Line2D line_from_points(AbsoluteDocumentPos p1, AbsoluteDocumentPos p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float nx = -dy;
    float ny = dx;
    float size = std::sqrt(nx * nx + ny * ny);

    nx = nx / size;
    ny = ny / size;

    Line2D res;
    res.nx = nx;
    res.ny = ny;
    res.c = nx * p1.x + ny * p1.y;
    return res;
}

QList<FreehandDrawingPoint> smooth_filter_drawing_points_helper(const QList<FreehandDrawingPoint>& points) {
    if (points.size() < 3) return points;

    QList<FreehandDrawingPoint> new_points;
    new_points.push_back(points[0]);
    for (int i = 1; i < points.size()-1; i++){
        const FreehandDrawingPoint& next_point = points[i+1];
        const FreehandDrawingPoint& prev_point = points[i-1];

        FreehandDrawingPoint new_point;
        new_point.thickness = (prev_point.thickness + next_point.thickness) / 2;
        new_point.pos.x = (prev_point.pos.x + next_point.pos.x) / 2;
        new_point.pos.y = (prev_point.pos.y + next_point.pos.y) / 2;
        new_points.push_back(new_point);
    }
    new_points.push_back(points[points.size()-1]);
    return new_points;
}

QList<FreehandDrawingPoint> smooth_filter_drawing_points(const QList<FreehandDrawingPoint>& points, int amount) {
    QList<FreehandDrawingPoint> smoothed_points = points;
    for (int i = 0; i < amount; i++){
        smoothed_points = smooth_filter_drawing_points_helper(smoothed_points);
    }
    return smoothed_points;
}

QList<FreehandDrawingPoint> prune_freehand_drawing_points(const QList<FreehandDrawingPoint>& points) {

    if (points.size() < 3) {
        return points;
    }

    QList<FreehandDrawingPoint> pruned_points;
    pruned_points.push_back(points[0]);
    int candid_index = 1;

    while (candid_index < points.size() - 1) {
        int next_index = candid_index + 1;
        if ((points[candid_index].pos.x == pruned_points.back().pos.x) && (points[candid_index].pos.y == pruned_points.back().pos.y)) {
            candid_index++;
            continue;
        }

        float dx0 = points[candid_index].pos.x - pruned_points.back().pos.x;
        float dy0 = points[candid_index].pos.y - pruned_points.back().pos.y;

        float dx1 = points[next_index].pos.x - points[candid_index].pos.x;
        float dy1 = points[next_index].pos.y - points[candid_index].pos.y;
        float dot_product = dx0 * dx1 + dy0 * dy1;

        Line2D line = line_from_points(pruned_points.back().pos, points[next_index].pos);
        float thickness_factor = std::min(points[candid_index].thickness, 1.0f);
        if ((dot_product < 0) || (point_distance_from_line(points[candid_index].pos, line) > (0.2f * thickness_factor))) {
            pruned_points.push_back(points[candid_index]);
        }


        candid_index++;
    }

    pruned_points.push_back(points[points.size() - 1]);


    return pruned_points;
}


bool are_same(const FreehandDrawing& lhs, const FreehandDrawing& rhs) {
    if (lhs.points.size() != rhs.points.size()) {
        return false;
    }
    if (lhs.type != rhs.type) {
        return false;
    }
    for (int i = 0; i < lhs.points.size(); i++) {
        if (!are_same(lhs.points[i].pos, rhs.points[i].pos)) {
            return false;
        }
    }
    return true;

}

std::optional<DetectedRectResult> detect_rect_drawing(const std::vector<FreehandDrawing>& drawings){

    std::vector<AlmostAxisAlignedLine> almost_vertical_line_xs;
    std::vector<AlmostAxisAlignedLine> almost_horizontal_line_ys;

    for (const FreehandDrawing& drawing : drawings){
        if (drawing.points.size() > 1){
            for (int i = 1; i < drawing.points.size(); i++){
                Line2D line = line_from_points(drawing.points[i-1].pos, drawing.points[i].pos);
                float strength = (drawing.points[i].pos -  drawing.points[i-1].pos).norm();
                if (std::abs(line.nx) < 0.2f){
                    if (line.ny < 0){
                        line.ny = -line.ny;
                        line.c = -line.c;
                    }
                    AlmostAxisAlignedLine axis_aligned_line;
                    axis_aligned_line.c = (drawing.points[i-1].pos.y + drawing.points[i].pos.y) / 2;
                    axis_aligned_line.strength = strength;
                    almost_horizontal_line_ys.push_back(axis_aligned_line);
                    // qDebug() << "!!" << line.c << " " << drawing.points[0].pos.x;
                }
                if (std::abs(line.ny) < 0.2f){
                    if (line.nx < 0){
                        line.nx = -line.nx;
                        line.c = -line.c;
                    }
                    AlmostAxisAlignedLine axis_aligned_line;
                    // axis_aligned_line.c = line.c;
                    axis_aligned_line.c = (drawing.points[i-1].pos.x + drawing.points[i].pos.x) / 2;
                    axis_aligned_line.strength = strength;
                    almost_vertical_line_xs.push_back(axis_aligned_line);
                }
            }

        }
    }

    float threshold = 10.0f;

    std::sort(almost_horizontal_line_ys.begin(), almost_horizontal_line_ys.end(), [](const AlmostAxisAlignedLine& p1, const AlmostAxisAlignedLine& p2){
        return p1.c < p2.c;
    });

    std::sort(almost_vertical_line_xs.begin(), almost_vertical_line_xs.end(), [](const AlmostAxisAlignedLine& p1, const AlmostAxisAlignedLine& p2){
        return p1.c < p2.c;
    });

    float max_horizontal_strength = 0;
    float max_vertical_strength = 0;

    for (int i = 0; i < almost_horizontal_line_ys.size(); i++){
        int j = i;
        while (j < almost_horizontal_line_ys.size() && (almost_horizontal_line_ys[j].c - almost_horizontal_line_ys[i].c) < threshold){
            almost_horizontal_line_ys[i].strength += almost_horizontal_line_ys[j].strength;
            float s = almost_horizontal_line_ys[i].strength;
            if (s > max_horizontal_strength){
                max_horizontal_strength = s;
            }
            j++;
        }
    }

    for (int i = 0; i < almost_vertical_line_xs.size(); i++){
        int j = i;
        while (j < almost_vertical_line_xs.size() && (almost_vertical_line_xs[j].c - almost_vertical_line_xs[i].c) < threshold){
            almost_vertical_line_xs[i].strength += almost_vertical_line_xs[j].strength;
            float s =almost_vertical_line_xs[i].strength;
            if (s > max_vertical_strength) {
                max_vertical_strength = s;
            }
            j++;
        }
    }
        // bookmarks.erase(std::remove_if(bookmarks.begin(), bookmarks.end(), predicate), bookmarks.end());
    almost_horizontal_line_ys.erase(std::remove_if(almost_horizontal_line_ys.begin(), almost_horizontal_line_ys.end(), [&](const AlmostAxisAlignedLine& p){
        return p.strength < max_horizontal_strength / 2;
    }), almost_horizontal_line_ys.end());

    almost_vertical_line_xs.erase(std::remove_if(almost_vertical_line_xs.begin(), almost_vertical_line_xs.end(), [&](const AlmostAxisAlignedLine& p){
        return p.strength < max_vertical_strength / 2;
    }), almost_vertical_line_xs.end());


    if (almost_horizontal_line_ys.size() >= 2 && almost_vertical_line_xs.size() >= 2){
        TwoCenterResult horizontal_points = weighted_one_dimensional_two_center(almost_horizontal_line_ys);
        TwoCenterResult vertical_points = weighted_one_dimensional_two_center(almost_vertical_line_xs);

        DetectedRectResult result;

        AbsoluteRect result_rect;
        result_rect.x0 = vertical_points.smaller_center;
        result_rect.x1 = vertical_points.larger_center;
        result_rect.y0 = horizontal_points.smaller_center;
        result_rect.y1 = horizontal_points.larger_center;

        result.x0_strength = vertical_points.smaller_center_strength;
        result.x1_strength = vertical_points.larger_center_strength;
        result.y0_strength = horizontal_points.smaller_center_strength;
        result.y1_strength = horizontal_points.larger_center_strength;

        result.rect = result_rect;
        return result;
    }

    return {};
}
