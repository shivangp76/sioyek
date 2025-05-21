#include <sstream>
#include <QVariant>
#include "utils/color_utils.h"

extern float HIGHLIGHT_COLORS[26 * 3];
extern float DARK_MODE_CONTRAST;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern float CUSTOM_COLOR_CONTRAST;
extern float BLACK_COLOR[3];
extern bool ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE;

template<int d1, int d2, int d3>
void matmul(const float m1[], const float m2[], float result[]) {
    for (int i = 0; i < d1; i++) {
        for (int j = 0; j < d3; j++) {
            result[i * d3 + j] = 0;
            for (int k = 0; k < d2; k++) {
                result[i * d3 + j] += m1[i * d2 + k] * m2[k * d3 + j];
            }
        }
    }
}

void convert_color4(const float* in_color, int* out_color) {
    out_color[0] = (int)(in_color[0] * 255);
    out_color[1] = (int)(in_color[1] * 255);
    out_color[2] = (int)(in_color[2] * 255);
    out_color[3] = (int)(in_color[3] * 255);
}

void convert_color3(const float* in_color, int* out_color) {
    out_color[0] = (int)(in_color[0] * 255);
    out_color[1] = (int)(in_color[1] * 255);
    out_color[2] = (int)(in_color[2] * 255);
}

void convert_qcolor_to_float3(const QColor& color, float* out_floats) {
    *(out_floats + 0) = static_cast<float>(color.red()) / 255.0f;
    *(out_floats + 1) = static_cast<float>(color.green()) / 255.0f;
    *(out_floats + 2) = static_cast<float>(color.blue()) / 255.0f;
}

//QColor convert_float3_to_qcolor(const float* floats) {
//    return QColor(get_color_qml_string(floats[0], floats[1], floats[2]));
//}

QColor convert_float3_to_qcolor(const float* floats) {
    return QColor::fromRgbF(floats[0], floats[1], floats[2]);
}

QColor convert_float4_to_qcolor(const float* floats) {
    int colors[4];
    convert_color4(floats, colors);
    return QColor(colors[0], colors[1], colors[2], colors[3]);
}

void convert_qcolor_to_float4(const QColor& color, float* out_floats) {
    *(out_floats + 0) = static_cast<float>(color.red()) / 255.0f;
    *(out_floats + 1) = static_cast<float>(color.green()) / 255.0f;
    *(out_floats + 2) = static_cast<float>(color.blue()) / 255.0f;
    *(out_floats + 3) = static_cast<float>(color.alpha()) / 255.0f;
}

float highlight_color_distance(float color1[3], float color2[3]) {
    QColor c1 = QColor::fromRgbF(color1[0], color1[1], color1[2]);
    QColor c2 = QColor::fromRgbF(color2[0], color2[1], color2[2]);
    return std::abs(c1.hslHueF() - c2.hslHueF());
}

char get_highlight_color_type(float color[3]) {
    float min_distance = 1000;
    int min_index = -1;

    for (int i = 0; i < 26; i++) {
        //float dist = vec3_distance_squared(color, &HIGHLIGHT_COLORS[i * 3]);
        float dist = highlight_color_distance(color, &HIGHLIGHT_COLORS[i * 3]);
        if (dist < min_distance) {
            min_distance = dist;
            min_index = i;
        }
    }

    return 'a' + min_index;
}

float* get_highlight_type_color(char type) {
    if (type == '_') {
        return &BLACK_COLOR[0];
    }
    if (type >= 'a' && type <= 'z') {
        return &HIGHLIGHT_COLORS[(type - 'a') * 3];
    }
    if (type >= 'A' && type <= 'Z') {
        return &HIGHLIGHT_COLORS[(type - 'A') * 3];
    }
    return &HIGHLIGHT_COLORS[0];
}

void lighten_color(float input[3], float output[3]) {
    QColor color = qRgb(
        static_cast<int>(input[0] * 255),
        static_cast<int>(input[1] * 255),
        static_cast<int>(input[2] * 255)
    );
    float prev_lightness = static_cast<float>(color.lightness()) / 255.0f;
    int lightness_increase = static_cast<int>((0.9f / prev_lightness) * 100);

    QColor lighter = color;
    if (lightness_increase > 100) {
        lighter = color.lighter(lightness_increase);
    }

    output[0] = lighter.redF();
    output[1] = lighter.greenF();
    output[2] = lighter.blueF();
}

QVariantMap get_color_mapping() {
    QVariantMap color_map;
    for (int i = 'a'; i <= 'z'; i++) {
        QColor color = QColor::fromRgbF(
            HIGHLIGHT_COLORS[3 * (i - 'a') + 0],
            HIGHLIGHT_COLORS[3 * (i - 'a') + 1],
            HIGHLIGHT_COLORS[3 * (i - 'a') + 2]
        );
        color_map[QString::number(i)] = color;
        color_map[QString::number(i + 'A' - 'a')] = color;
    }
    color_map["_"] = QVariant::fromValue(Qt::black);
    return color_map;
}

QString get_color_hexadecimal(float color) {
    QString hex_map[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f" };
    int val = static_cast<int>(color * 255);
    int high = val / 16;
    int low = val % 16;
    return QString("%1%2").arg(hex_map[high], hex_map[low]);

}

QString get_color_qml_string(float r, float g, float b) {
    QString res = QString("#%1%2%3").arg(get_color_hexadecimal(r), get_color_hexadecimal(g), get_color_hexadecimal(b));
    return res;
}

int hex2int(int hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    }
    else {
        return (hex - 'a') + 10;
    }
}

std::wstring lowercase(const std::wstring& input) {

    std::wstring res;
    for (int i = 0; i < input.size(); i++) {
        if ((input[i] >= 'A') && (input[i] <= 'Z')) {
            res.push_back(input[i] + 'a' - 'A');
        }
        else {
            res.push_back(input[i]);
        }
    }
    return res;
}

float get_color_component_from_hex(std::wstring hexcolor) {
    hexcolor = lowercase(hexcolor);

    if (hexcolor.size() < 2) {
        return 0;
    }
    return static_cast<float>(hex2int(hexcolor[0]) * 16 + hex2int(hexcolor[1])) / 255.0f;
}

void hexademical_to_normalized_color(std::wstring color_string, float* color, int n_components) {
    if (color_string[0] == '#') {
        color_string = color_string.substr(1, color_string.size() - 1);
    }

    for (int i = 0; i < n_components; i++) {
        *(color + i) = get_color_component_from_hex(color_string.substr(i * 2, 2));
    }
}

void parse_color(std::wstring color_string, float* out_color, int n_components) {
    if (color_string.size() > 0) {
        if (color_string[0] == '#') {
            hexademical_to_normalized_color(color_string, out_color, n_components);
        }
        else {
            std::wstringstream ss(color_string);

            for (int i = 0; i < n_components; i++) {
                ss >> *(out_color + i);
            }
        }
    }
}

void get_color_for_mode(ColorPalette color_mode, const float* input_color, float* output_color) {
    if (!ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE) {
        output_color[0] = input_color[0];
        output_color[1] = input_color[1];
        output_color[2] = input_color[2];
        return;
    }

    if (color_mode == ColorPalette::Dark) {
        float inverted_color[3];
        inverted_color[0] = (0.5f - input_color[0]) * DARK_MODE_CONTRAST + 0.5f;
        inverted_color[1] = (0.5f - input_color[1]) * DARK_MODE_CONTRAST + 0.5f;
        inverted_color[2] = (0.5f - input_color[2]) * DARK_MODE_CONTRAST + 0.5f;
        float hsv_color[3];
        rgb2hsv(inverted_color, hsv_color);
        float new_hue = fmod(hsv_color[0] + 0.5f, 1.0f);
        hsv_color[0] = new_hue;
        hsv2rgb(hsv_color, output_color);
    }
    else if (color_mode == ColorPalette::Custom) {
        float transform_matrix[16];
        float input_vector[4];
        float output_vector[4];
        input_vector[0] = input_color[0];
        input_vector[1] = input_color[1];
        input_vector[2] = input_color[2];
        input_vector[3] = 1.0f;

        get_custom_color_transform_matrix(transform_matrix);
        matmul<4, 4, 1>(transform_matrix, input_vector, output_vector);
        output_color[0] = fz_clamp(output_vector[0], 0, 1);
        output_color[1] = fz_clamp(output_vector[1], 0, 1);
        output_color[2] = fz_clamp(output_vector[2], 0, 1);
        return;
    }
    else {
        output_color[0] = input_color[0];
        output_color[1] = input_color[1];
        output_color[2] = input_color[2];
    }
}

void get_custom_color_transform_matrix(float matrix_data[16]) {
    float inputs_inverse[16] = { 0, 1, 0, 0, -1, 1, -1, 1, 1, -1, 0, 0, 0, -1, 1, 0 };
    float outputs[16] = {
        CUSTOM_BACKGROUND_COLOR[0], CUSTOM_TEXT_COLOR[0], 1, 0,
        CUSTOM_BACKGROUND_COLOR[1], CUSTOM_TEXT_COLOR[1], CUSTOM_COLOR_CONTRAST * (1 - CUSTOM_BACKGROUND_COLOR[1]), CUSTOM_COLOR_CONTRAST * (1 - CUSTOM_BACKGROUND_COLOR[1]),
        CUSTOM_BACKGROUND_COLOR[2], CUSTOM_TEXT_COLOR[2], 0, 1,
        1, 1, 1, 1,
    };

    matmul<4, 4, 4>(outputs, inputs_inverse, matrix_data);
}

// With qpainter backend we need to manually convert pixels to
// the current colorscheme. The following code basically does this
// conversion for every pixel. For performance reasons we cache the previous result
// so we don't have to recompute it when successive pixels are the same
// (which happens a lot)
void convert_pixels_with_converter(unsigned char* pixels, int width, int height, int stride, int n_channels, std::function<void(unsigned char*)> converter) {
    unsigned char* first_pixel = pixels;
    unsigned char last_r = first_pixel[0], last_g = first_pixel[1], last_b = first_pixel[2];
    converter(first_pixel);
    unsigned char res_r = first_pixel[0], res_g = first_pixel[1], res_b = first_pixel[2];

    for (int row = 0; row < height; row++) {
        unsigned char* row_samples = pixels + row * stride;
        for (int col = 0; col < width; col++) {
            unsigned char* pixel = row_samples + col * n_channels;
            if (pixel[0] == last_r && pixel[1] == last_g && pixel[2] == last_b) {
                pixel[0] = res_r;
                pixel[1] = res_g;
                pixel[2] = res_b;
                continue;
            }
            last_r = pixel[0];
            last_g = pixel[1];
            last_b = pixel[2];

            converter(pixel);

            res_r = pixel[0];
            res_g = pixel[1];
            res_b = pixel[2];
        }
    }
};

void convert_pixel_to_dark_mode(unsigned char* pixel){
    QColor inv(
        static_cast<unsigned char>((127 - pixel[0]) * DARK_MODE_CONTRAST + 127),
        static_cast<unsigned char>((127 - pixel[1]) * DARK_MODE_CONTRAST + 127),
        static_cast<unsigned char>((127 - pixel[2]) * DARK_MODE_CONTRAST + 127)
        );

    QColor hsv_color = inv.toHsv();
    int new_hue = (hsv_color.hue() + 180) % 360;
    QColor new_color = QColor::fromHsv(new_hue, hsv_color.saturation(), hsv_color.value());

    pixel[0] = (unsigned char)new_color.red();
    pixel[1] = (unsigned char)new_color.green();
    pixel[2] = (unsigned char)new_color.blue();
}

void convert_pixel_to_custom_color(unsigned char* pixel, float transform_matrix[16]){
    float colorf[4];
    colorf[0] = static_cast<float>(pixel[0]) / 255.0f;
    colorf[1] = static_cast<float>(pixel[1]) / 255.0f;
    colorf[2] = static_cast<float>(pixel[2]) / 255.0f;
    colorf[3] = 1.0f;
    float transformed_color[4];

    matmul<4, 4, 1>(transform_matrix, colorf, transformed_color);
    transformed_color[0] = std::clamp(transformed_color[0], 0.0f, 1.0f);
    transformed_color[1] = std::clamp(transformed_color[1], 0.0f, 1.0f);
    transformed_color[2] = std::clamp(transformed_color[2], 0.0f, 1.0f);

    pixel[0] = static_cast<unsigned char>(transformed_color[0] * 255);
    pixel[1] = static_cast<unsigned char>(transformed_color[1] * 255);
    pixel[2] = static_cast<unsigned char>(transformed_color[2] * 255);
}

void rgb2hsv(float* rgb_color, float* hsv_color) {
    int rgb_255_color[3];
    convert_color3(rgb_color, rgb_255_color);
    QColor qcolor(rgb_255_color[0], rgb_255_color[1], rgb_255_color[2]);
    QColor hsv_qcolor = qcolor.toHsv();
    hsv_color[0] = hsv_qcolor.hsvHueF();
    hsv_color[1] = hsv_qcolor.hsvSaturationF();
    hsv_color[2] = hsv_qcolor.lightnessF();
    if (hsv_color[0] < 0) hsv_color[0] += 1.0f;
}

void hsv2rgb(float* hsv_color, float* rgb_color) {
    QColor qcolor;
    qcolor.setHsvF(hsv_color[0], hsv_color[1], hsv_color[2]);
    rgb_color[0] = qcolor.redF();
    rgb_color[1] = qcolor.greenF();
    rgb_color[2] = qcolor.blueF();
}

QColor qconvert_color3(const float* input_color, ColorPalette palette) {
    std::array<float, 3> result;
    get_color_for_mode(palette, input_color, &result[0]);
    return convert_float3_to_qcolor(&result[0]);
}

QList<QColor> get_symbol_colors_for_qml(){
    QList<QColor> colors;
    const int N_COLORS = 26;
    for (int i = 0; i < N_COLORS; i++) {
        colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * i]));
    }
    return colors;
}
