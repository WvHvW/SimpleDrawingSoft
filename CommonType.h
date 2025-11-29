#pragma once

// 定义公共的枚举类型，避免重定义
enum class ShapeType {
    LINE,
    CIRCLE,
    RECTANGLE,
    TRIANGLE,
    DIAMOND,
    PARALLELOGRAM,
    CURVE,
    POLYLINE,
    MULTI_BEZIER,     // 多点Bezier曲线
    POLYGON           // 多边形
};

enum class DrawingMode {
    SELECT,
    LINE,
    CIRCLE,
    RECTANGLE,
    TRIANGLE,
    DIAMOND,
    PARALLELOGRAM,
    CURVE,
    POLYLINE,
    INTERSECT,
    PERPENDICULAR,
    CENTER,
    TANGENT,
    MIDPOINT_LINE,
    BRESENHAM_LINE,
    MIDPOINT_CIRCLE,
    BRESENHAM_CIRCLE,
    MULTI_BEZIER,  // 多点Bezier曲线
    SCANLINE_FILL, // 栅栏填充
    SEED_FILL,     // 种子填充
    POLYGON        // 多边形
};

enum class TransformMode {
    NONE,
    MOVE,
    ROTATE,
    SCALE
};

// 线宽枚举
enum class LineWidth {
    WIDTH_1PX = 1,
    WIDTH_2PX = 2,
    WIDTH_4PX = 4,
    WIDTH_8PX = 8,
    WIDTH_16PX = 16
};

// 线型枚举 (为后续功能预留)
enum class LineStyle {
    SOLID = 0,          // 实线
    DASH = 1,           // 虚线
    DOT = 2,            // 点线
    DASH_DOT = 3,       // 点划线
    DASH_DOT_DOT = 4    // 双点划线
};