#pragma once
#include "Shape.h"
#include <vector>
#include <memory>

// 数学常量
constexpr float EPSILON = 1e-6f;
constexpr float PI = 3.14159265358979323846f;

// 数学工具函数 - 内联实现
namespace IntersectionMath {
inline float distance(const D2D1_POINT_2F &p1, const D2D1_POINT_2F &p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return sqrtf(dx * dx + dy * dy);
}

inline float crossProduct(const D2D1_POINT_2F &v1, const D2D1_POINT_2F &v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

inline float dotProduct(const D2D1_POINT_2F &v1, const D2D1_POINT_2F &v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

inline bool isPointOnSegment(const D2D1_POINT_2F &point, const D2D1_POINT_2F &segStart, const D2D1_POINT_2F &segEnd) {
    if (point.x < min(segStart.x, segEnd.x) - EPSILON || point.x > max(segStart.x, segEnd.x) + EPSILON || point.y < min(segStart.y, segEnd.y) - EPSILON || point.y > max(segStart.y, segEnd.y) + EPSILON) {
        return false;
    }

    D2D1_POINT_2F v1 = {point.x - segStart.x, point.y - segStart.y};
    D2D1_POINT_2F v2 = {segEnd.x - segStart.x, segEnd.y - segStart.y};
    return fabs(crossProduct(v1, v2)) < EPSILON;
}

inline bool isPointInRectangle(const D2D1_POINT_2F &point, const D2D1_RECT_F &rect) {
    return point.x >= rect.left - EPSILON && point.x <= rect.right + EPSILON && point.y >= rect.top - EPSILON && point.y <= rect.bottom + EPSILON;
}

inline bool doSegmentsIntersect(const D2D1_POINT_2F &p1, const D2D1_POINT_2F &p2,
                                const D2D1_POINT_2F &q1, const D2D1_POINT_2F &q2) {
    // 快速排斥实验
    if (max(p1.x, p2.x) < min(q1.x, q2.x) || max(q1.x, q2.x) < min(p1.x, p2.x) || max(p1.y, p2.y) < min(q1.y, q2.y) || max(q1.y, q2.y) < min(p1.y, p2.y)) {
        return false;
    }

    // 跨立实验
    auto cross = [](const D2D1_POINT_2F &a, const D2D1_POINT_2F &b, const D2D1_POINT_2F &c) {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    };

    if (cross(p1, p2, q1) * cross(p1, p2, q2) > EPSILON || cross(q1, q2, p1) * cross(q1, q2, p2) > EPSILON) {
        return false;
    }

    return true;
}

inline void removeDuplicatePoints(std::vector<D2D1_POINT_2F> &points) {
    auto it = std::unique(points.begin(), points.end(),
                          [](const D2D1_POINT_2F &p1, const D2D1_POINT_2F &p2) {
                              return distance(p1, p2) < EPSILON;
                          });
    points.erase(it, points.end());
}
} // namespace IntersectionMath

class IntersectionManager {
public:
    static IntersectionManager &getInstance();

    // 选择图元用于求交
    bool selectShape(std::shared_ptr<Shape> shape);

    // 计算当前选中图元的交点
    std::vector<D2D1_POINT_2F> calculateIntersection();

    // 获取交点用于显示
    const std::vector<D2D1_POINT_2F> &getIntersectionPoints() const;

    // 清除状态
    void clear();

    // 状态查询
    bool hasTwoShapes() const;
    int getSelectedCount() const;

    // 获取选中的图元
    std::shared_ptr<Shape> getFirstShape() const {
        return shape1;
    }
    std::shared_ptr<Shape> getSecondShape() const {
        return shape2;
    }

private:
    IntersectionManager() = default;

    std::shared_ptr<Shape> shape1;
    std::shared_ptr<Shape> shape2;
    std::vector<D2D1_POINT_2F> intersectionPoints;

    // 核心求交算法实现
    std::vector<D2D1_POINT_2F> calculateIntersectionImpl();

    // 精确求交算法 - 实例方法
    std::vector<D2D1_POINT_2F> calculateLineLineIntersection(Line *line1, Line *line2);
    std::vector<D2D1_POINT_2F> calculateLineCircleIntersection(Line *line, Circle *circle);
    std::vector<D2D1_POINT_2F> calculateCircleCircleIntersection(Circle *circle1, Circle *circle2);
    std::vector<D2D1_POINT_2F> calculateLineRectIntersection(Line *line, Rect *rect);
    std::vector<D2D1_POINT_2F> calculateCircleRectIntersection(Circle *circle, Rect *rect);
    std::vector<D2D1_POINT_2F> calculateRectRectIntersection(Rect *rect1, Rect *rect2);

    // 几何计算辅助函数 - 实例方法
    std::vector<D2D1_POINT_2F> calculateLineSegmentIntersection(
        const D2D1_POINT_2F &p1, const D2D1_POINT_2F &p2,
        const D2D1_POINT_2F &q1, const D2D1_POINT_2F &q2);

    // 形状类型检查辅助函数
    bool isLineShape(Shape *shape) const;
    bool isCircleShape(Shape *shape) const;
    bool isRectangleShape(Shape *shape) const;
    bool isTriangleShape(Shape *shape) const;

    // 精确求交路由函数
    std::vector<D2D1_POINT_2F> routeToExactIntersection(Shape *shape1, Shape *shape2);
};