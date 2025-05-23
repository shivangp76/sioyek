#pragma once
#include <QList>
#include "types/drawing_types.h"

struct DetectedRectResult{
    AbsoluteRect rect;
    float x0_strength;
    float x1_strength;
    float y0_strength;
    float y1_strength;
};


QList<FreehandDrawingPoint> smooth_filter_drawing_points(const QList<FreehandDrawingPoint>& points, int amount);
QList<FreehandDrawingPoint> prune_freehand_drawing_points(const QList<FreehandDrawingPoint>& points);
bool are_same(const FreehandDrawing& lhs, const FreehandDrawing& rhs);

std::optional<DetectedRectResult> detect_rect_drawing(const std::vector<FreehandDrawing>& drawings);
