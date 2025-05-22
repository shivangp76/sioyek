#pragma once

#include <vector>
#include "coordinates.h"

class QImage;

std::vector<WindowRect> get_largest_empty_rects_from_rendered_image(QImage& img, int status_bar_height);
