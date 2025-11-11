#include "IntersectionManager.h"
#include <cmath>
#include <algorithm>
#include <vector>

// 使用数学工具命名空间
using namespace IntersectionMath;

// 线段-线段精确求交
std::vector<D2D1_POINT_2F> IntersectionManager::calculateLineLineIntersection(Line *line1, Line *line2) {
    std::vector<D2D1_POINT_2F> intersections;

    if (!line1 || !line2) return intersections;

    D2D1_POINT_2F p1 = line1->GetStart();
    D2D1_POINT_2F p2 = line1->GetEnd();
    D2D1_POINT_2F p3 = line2->GetStart();
    D2D1_POINT_2F p4 = line2->GetEnd();

    float denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);

    if (fabs(denom) < EPSILON) {
        // 平行或重合
        if (isPointOnSegment(p1, p3, p4)) intersections.push_back(p1);
        if (isPointOnSegment(p2, p3, p4)) intersections.push_back(p2);
        if (isPointOnSegment(p3, p1, p2)) intersections.push_back(p3);
        if (isPointOnSegment(p4, p1, p2)) intersections.push_back(p4);
        removeDuplicatePoints(intersections);
        return intersections;
    }

    float t = ((p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x)) / denom;
    float u = -((p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x)) / denom;

    if (t >= -EPSILON && t <= 1.0f + EPSILON && u >= -EPSILON && u <= 1.0f + EPSILON) {
        D2D1_POINT_2F intersection;
        intersection.x = p1.x + t * (p2.x - p1.x);
        intersection.y = p1.y + t * (p2.y - p1.y);
        intersections.push_back(intersection);
    }

    return intersections;
}

// 直线-圆精确求交
std::vector<D2D1_POINT_2F> IntersectionManager::calculateLineCircleIntersection(Line *line, Circle *circle) {
    std::vector<D2D1_POINT_2F> intersections;

    if (!line || !circle) return intersections;

    D2D1_POINT_2F lineStart = line->GetStart();
    D2D1_POINT_2F lineEnd = line->GetEnd();
    D2D1_POINT_2F center = circle->GetCenter();
    float radius = circle->GetRadius();

    // 将直线表示为参数方程：P = lineStart + t * (lineEnd - lineStart)
    D2D1_POINT_2F dir = {lineEnd.x - lineStart.x, lineEnd.y - lineStart.y};

    // 计算直线方程系数
    float a = dir.x * dir.x + dir.y * dir.y;
    float b = 2 * (dir.x * (lineStart.x - center.x) + dir.y * (lineStart.y - center.y));
    float c = (lineStart.x - center.x) * (lineStart.x - center.x) + (lineStart.y - center.y) * (lineStart.y - center.y) - radius * radius;

    // 计算判别式
    float discriminant = b * b - 4 * a * c;

    if (discriminant < -EPSILON) {
        // 无实数解，没有交点
        return intersections;
    } else if (fabs(discriminant) < EPSILON) {
        // 一个解，相切
        float t = -b / (2 * a);
        if (t >= -EPSILON && t <= 1.0f + EPSILON) {
            D2D1_POINT_2F intersection;
            intersection.x = lineStart.x + t * dir.x;
            intersection.y = lineStart.y + t * dir.y;
            intersections.push_back(intersection);
        }
    } else {
        // 两个解，相交
        discriminant = sqrtf(discriminant);
        float t1 = (-b + discriminant) / (2 * a);
        float t2 = (-b - discriminant) / (2 * a);

        if (t1 >= -EPSILON && t1 <= 1.0f + EPSILON) {
            D2D1_POINT_2F intersection;
            intersection.x = lineStart.x + t1 * dir.x;
            intersection.y = lineStart.y + t1 * dir.y;
            intersections.push_back(intersection);
        }

        if (t2 >= -EPSILON && t2 <= 1.0f + EPSILON) {
            D2D1_POINT_2F intersection;
            intersection.x = lineStart.x + t2 * dir.x;
            intersection.y = lineStart.y + t2 * dir.y;
            intersections.push_back(intersection);
        }

        removeDuplicatePoints(intersections);
    }

    return intersections;
}

// 圆-圆精确求交
std::vector<D2D1_POINT_2F> IntersectionManager::calculateCircleCircleIntersection(Circle *circle1, Circle *circle2) {
    std::vector<D2D1_POINT_2F> intersections;

    if (!circle1 || !circle2) return intersections;

    D2D1_POINT_2F center1 = circle1->GetCenter();
    D2D1_POINT_2F center2 = circle2->GetCenter();
    float radius1 = circle1->GetRadius();
    float radius2 = circle2->GetRadius();

    // 计算圆心距离
    float dx = center2.x - center1.x;
    float dy = center2.y - center1.y;
    float dist = sqrtf(dx * dx + dy * dy);

    // 检查特殊情况
    if (dist > radius1 + radius2 + EPSILON) {
        // 两圆相离
        return intersections;
    }
    if (dist < fabs(radius1 - radius2) - EPSILON) {
        // 一圆包含另一圆
        return intersections;
    }
    if (dist < EPSILON && fabs(radius1 - radius2) < EPSILON) {
        // 同心圆且半径相等，无限多个交点（这里返回空）
        return intersections;
    }

    // 计算交点
    float a = (radius1 * radius1 - radius2 * radius2 + dist * dist) / (2 * dist);
    float h = sqrtf(radius1 * radius1 - a * a);

    // 计算中点
    D2D1_POINT_2F midPoint;
    midPoint.x = center1.x + a * (center2.x - center1.x) / dist;
    midPoint.y = center1.y + a * (center2.y - center1.y) / dist;

    // 计算两个交点
    if (h > EPSILON) {
        D2D1_POINT_2F intersection1, intersection2;
        intersection1.x = midPoint.x + h * (center2.y - center1.y) / dist;
        intersection1.y = midPoint.y - h * (center2.x - center1.x) / dist;

        intersection2.x = midPoint.x - h * (center2.y - center1.y) / dist;
        intersection2.y = midPoint.y + h * (center2.x - center1.x) / dist;

        intersections.push_back(intersection1);
        intersections.push_back(intersection2);
    } else {
        // 相切，只有一个交点
        intersections.push_back(midPoint);
    }

    return intersections;
}

// 直线-矩形精确求交
std::vector<D2D1_POINT_2F> IntersectionManager::calculateLineRectIntersection(Line *line, Rect *rect) {
    std::vector<D2D1_POINT_2F> intersections;

    if (!line || !rect) return intersections;

    // 复用Shape类中的线段获取方法
    auto segments = rect->GetIntersectionSegments();

    // 检查直线与每条边的交点
    for (const auto &segment : segments) {
        Line edgeLine(segment.first, segment.second);
        auto edgeIntersections = calculateLineLineIntersection(line, &edgeLine);
        intersections.insert(intersections.end(), edgeIntersections.begin(), edgeIntersections.end());
    }

    removeDuplicatePoints(intersections);
    return intersections;
}

// 圆-矩形精确求交
std::vector<D2D1_POINT_2F> IntersectionManager::calculateCircleRectIntersection(Circle *circle, Rect *rect) {
    std::vector<D2D1_POINT_2F> intersections;

    if (!circle || !rect) return intersections;

    // 复用Shape类中的线段获取方法
    auto segments = rect->GetIntersectionSegments();

    for (const auto &segment : segments) {
        Line edgeLine(segment.first, segment.second);
        auto edgeIntersections = calculateLineCircleIntersection(&edgeLine, circle);
        intersections.insert(intersections.end(), edgeIntersections.begin(), edgeIntersections.end());
    }

    removeDuplicatePoints(intersections);
    return intersections;
}

// 矩形-矩形精确求交
std::vector<D2D1_POINT_2F> IntersectionManager::calculateRectRectIntersection(Rect *rect1, Rect *rect2) {
    std::vector<D2D1_POINT_2F> intersections;

    if (!rect1 || !rect2) return intersections;

    // 复用Shape类中的线段获取方法
    auto segments1 = rect1->GetIntersectionSegments();
    auto segments2 = rect2->GetIntersectionSegments();

    // 检查所有边之间的交点
    for (const auto &seg1 : segments1) {
        for (const auto &seg2 : segments2) {
            Line line1(seg1.first, seg1.second);
            Line line2(seg2.first, seg2.second);
            auto edgeIntersections = calculateLineLineIntersection(&line1, &line2);
            intersections.insert(intersections.end(), edgeIntersections.begin(), edgeIntersections.end());
        }
    }

    removeDuplicatePoints(intersections);
    return intersections;
}

// 几何计算辅助函数
std::vector<D2D1_POINT_2F> IntersectionManager::calculateLineSegmentIntersection(
    const D2D1_POINT_2F &p1, const D2D1_POINT_2F &p2,
    const D2D1_POINT_2F &q1, const D2D1_POINT_2F &q2) {
    // 复用现有的直线求交逻辑，创建临时Line对象
    Line tempLine1(p1, p2);
    Line tempLine2(q1, q2);

    return calculateLineLineIntersection(&tempLine1, &tempLine2);
}

// 核心求交实现
std::vector<D2D1_POINT_2F> IntersectionManager::calculateIntersectionImpl() {
    std::vector<D2D1_POINT_2F> intersections;

    if (!shape1 || !shape2) return intersections;

    // 首先尝试精确求交
    intersections = routeToExactIntersection(shape1.get(), shape2.get());

    // 如果精确求交有结果，直接返回
    if (!intersections.empty()) {
        return intersections;
    }

    // 对于其他组合，复用Shape类中的离散化方法
    auto segments1 = shape1->GetIntersectionSegments();
    auto segments2 = shape2->GetIntersectionSegments();

    // 复用现有的线段求交逻辑
    for (const auto &seg1 : segments1) {
        for (const auto &seg2 : segments2) {
            auto points = calculateLineSegmentIntersection(seg1.first, seg1.second, seg2.first, seg2.second);
            intersections.insert(intersections.end(), points.begin(), points.end());
        }
    }

    removeDuplicatePoints(intersections);
    return intersections;
}

// 其余管理函数保持不变...
IntersectionManager &IntersectionManager::getInstance() {
    static IntersectionManager instance;
    return instance;
}

bool IntersectionManager::selectShape(std::shared_ptr<Shape> shape) {
    if (!shape1) {
        shape1 = shape;
        return true;
    } else if (!shape2) {
        shape2 = shape;
        return true;
    }
    shape2 = shape;
    return true;
}

std::vector<D2D1_POINT_2F> IntersectionManager::calculateIntersection() {
    intersectionPoints = calculateIntersectionImpl();
    return intersectionPoints;
}

const std::vector<D2D1_POINT_2F> &IntersectionManager::getIntersectionPoints() const {
    return intersectionPoints;
}

void IntersectionManager::clear() {
    shape1.reset();
    shape2.reset();
    intersectionPoints.clear();
}

bool IntersectionManager::hasTwoShapes() const {
    return shape1 && shape2;
}

int IntersectionManager::getSelectedCount() const {
    return (shape1 ? 1 : 0) + (shape2 ? 1 : 0);
}

// 形状类型检查辅助函数
bool IntersectionManager::isLineShape(Shape *shape) const {
    return shape && shape->GetType() == ShapeType::LINE;
}

bool IntersectionManager::isCircleShape(Shape *shape) const {
    return shape && shape->GetType() == ShapeType::CIRCLE;
}

bool IntersectionManager::isRectangleShape(Shape *shape) const {
    return shape && shape->GetType() == ShapeType::RECTANGLE;
}

bool IntersectionManager::isTriangleShape(Shape *shape) const {
    return shape && shape->GetType() == ShapeType::TRIANGLE;
}

// 精确求交路由函数
std::vector<D2D1_POINT_2F> IntersectionManager::routeToExactIntersection(Shape *shape1, Shape *shape2) {
    std::vector<D2D1_POINT_2F> intersections;

    if (!shape1 || !shape2) return intersections;

    ShapeType type1 = shape1->GetType();
    ShapeType type2 = shape2->GetType();

    // 复用现有的精确求交逻辑
    if (type1 == ShapeType::CIRCLE && type2 == ShapeType::CIRCLE) {
        auto circle1 = dynamic_cast<Circle *>(shape1);
        auto circle2 = dynamic_cast<Circle *>(shape2);
        if (circle1 && circle2) {
            return calculateCircleCircleIntersection(circle1, circle2);
        }
    }

    if ((type1 == ShapeType::LINE && type2 == ShapeType::CIRCLE)) {
        auto line = dynamic_cast<Line *>(shape1);
        auto circle = dynamic_cast<Circle *>(shape2);
        if (line && circle) {
            return calculateLineCircleIntersection(line, circle);
        }
    }

    if ((type1 == ShapeType::CIRCLE && type2 == ShapeType::LINE)) {
        auto circle = dynamic_cast<Circle *>(shape1);
        auto line = dynamic_cast<Line *>(shape2);
        if (line && circle) {
            return calculateLineCircleIntersection(line, circle);
        }
    }

    if (type1 == ShapeType::LINE && type2 == ShapeType::LINE) {
        auto line1 = dynamic_cast<Line *>(shape1);
        auto line2 = dynamic_cast<Line *>(shape2);
        if (line1 && line2) {
            return calculateLineLineIntersection(line1, line2);
        }
    }

    // 添加矩形相关的精确求交
    if ((type1 == ShapeType::LINE && type2 == ShapeType::RECTANGLE)) {
        auto line = dynamic_cast<Line *>(shape1);
        auto rect = dynamic_cast<Rect *>(shape2);
        if (line && rect) {
            return calculateLineRectIntersection(line, rect);
        }
    }

    if ((type1 == ShapeType::RECTANGLE && type2 == ShapeType::LINE)) {
        auto rect = dynamic_cast<Rect *>(shape1);
        auto line = dynamic_cast<Line *>(shape2);
        if (line && rect) {
            return calculateLineRectIntersection(line, rect);
        }
    }

    if ((type1 == ShapeType::CIRCLE && type2 == ShapeType::RECTANGLE)) {
        auto circle = dynamic_cast<Circle *>(shape1);
        auto rect = dynamic_cast<Rect *>(shape2);
        if (circle && rect) {
            return calculateCircleRectIntersection(circle, rect);
        }
    }

    if ((type1 == ShapeType::RECTANGLE && type2 == ShapeType::CIRCLE)) {
        auto rect = dynamic_cast<Rect *>(shape1);
        auto circle = dynamic_cast<Circle *>(shape2);
        if (circle && rect) {
            return calculateCircleRectIntersection(circle, rect);
        }
    }

    if (type1 == ShapeType::RECTANGLE && type2 == ShapeType::RECTANGLE) {
        auto rect1 = dynamic_cast<Rect *>(shape1);
        auto rect2 = dynamic_cast<Rect *>(shape2);
        if (rect1 && rect2) {
            return calculateRectRectIntersection(rect1, rect2);
        }
    }

    return intersections; // 空结果，表示需要离散化求交
}