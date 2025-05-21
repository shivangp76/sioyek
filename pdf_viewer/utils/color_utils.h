#pragma once

#include <QColor>
#include "book.h"

void convert_color3(const float* in_color, int* out_color);
void convert_color4(const float* in_color, int* out_color);
QColor convert_float3_to_qcolor(const float* floats);
QColor convert_float4_to_qcolor(const float* floats);
void convert_qcolor_to_float3(const QColor& color, float* out_floats);
void convert_qcolor_to_float4(const QColor& color, float* out_floats);

char get_highlight_color_type(float color[3]);
float* get_highlight_type_color(char type);
void lighten_color(float input[3], float output[3]);

QVariantMap get_color_mapping();
QString get_color_qml_string(float r, float g, float b);

void hexademical_to_normalized_color(std::wstring color_string, float* color, int n_components);
void parse_color(std::wstring color_string, float* out_color, int n_components);

void get_color_for_mode(ColorPalette color_mode, const float* input_color, float* output_color);
void get_custom_color_transform_matrix(float matrix_data[16]);
void convert_pixels_with_converter(unsigned char* pixels, int width, int height, int stride, int n_channels, std::function<void(unsigned char*)> converter);
void convert_pixel_to_dark_mode(unsigned char* pixel);
void convert_pixel_to_custom_color(unsigned char* pixel, float transform_matrix[16]);

void rgb2hsv(float* rgb_color, float* hsv_color);
void hsv2rgb(float* hsv_color, float* rgb_color);

QColor qconvert_color3(const float* input_color, ColorPalette palette);

QList<QColor> get_symbol_colors_for_qml();
