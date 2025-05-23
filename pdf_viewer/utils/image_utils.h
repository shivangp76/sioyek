#pragma once

#include <vector>
#include "coordinates.h"

class QImage;

std::vector<WindowRect> get_largest_empty_rects_from_rendered_image(QImage& img, int status_bar_height);
std::vector<unsigned int> get_max_width_histogram_from_pixmap(fz_pixmap* pixmap);
int find_best_vertical_line_location(fz_pixmap* pixmap, int relative_click_x, int relative_click_y);
void get_line_begins_and_ends_from_histogram(std::vector<unsigned int> histogram, std::vector<unsigned int>& begins, std::vector<unsigned int>& ends);
