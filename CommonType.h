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
    POLYLINE
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
    TANGENT
};