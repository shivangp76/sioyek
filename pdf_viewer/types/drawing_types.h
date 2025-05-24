#pragma once

#include <QObject>
#include <qopengl.h>
#include <QDateTime>
#include <QUuid>
#include <QPixmap>

#include "coordinates.h"
#include "types/common_types.h"

struct CompiledDrawingData {
    GLuint vao = 0;
    GLuint vertex_buffer = 0;
    GLuint index_buffer = 0;
    GLuint dots_vertex_buffer = 0;
    GLuint dots_uv_buffer = 0;
    GLuint dots_index_buffer = 0;
    GLuint lines_type_index_buffer = 0;
    GLuint dots_type_index_buffer = 0;
    int n_elements = 0;
    int n_dot_elements = 0;
};

struct FreehandDrawingPoint {
    Q_GADGET
    Q_PROPERTY(AbsoluteDocumentPos pos READ get_pos)
    Q_PROPERTY(float thickness MEMBER thickness)
public:
    AbsoluteDocumentPos pos;
    float thickness;

    AbsoluteDocumentPos get_pos();
};
Q_DECLARE_METATYPE(FreehandDrawingPoint)


struct FreehandDrawing {
    Q_GADGET
    Q_PROPERTY(QList<FreehandDrawingPoint> points READ get_points)
    Q_PROPERTY(char type MEMBER type)
    Q_PROPERTY(float alpha MEMBER alpha)
    Q_PROPERTY(QDateTime creation_time MEMBER creattion_time)
public:
    QList<FreehandDrawingPoint> points;
    char type;
    float alpha = 1;
    QDateTime creattion_time;
    QUuid::Id128Bytes uuid = {};
    bool is_synced = false;
    int network_pending_request_id = -1;

    Q_INVOKABLE AbsoluteRect bbox() const;
    QList<FreehandDrawingPoint> get_points();

};

struct PixmapDrawing {
    QPixmap pixmap;
    AbsoluteRect rect;
};

struct SelectedDrawings {
    int page;
    AbsoluteRect selection_absrect_;
    std::vector<SelectedObjectIndex> selected_indices;
};

struct PageFreehandDrawing{
    Q_GADGET
    Q_PROPERTY(QList<FreehandDrawing> drawings READ get_drawings)
public:
    QList<FreehandDrawing> drawings;
    QDateTime last_addition_time;
    QDateTime last_deletion_time;

    QList<FreehandDrawing> get_drawings();
};
