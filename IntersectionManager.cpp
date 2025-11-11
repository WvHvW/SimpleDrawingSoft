#include "IntersectionManager.h"
#include <cmath>
#include <algorithm>

namespace {
const float EPS = 1e-5f;

// 点是否在线段上（含端点）
bool pointOnSegment(D2D1_POINT_2F p, D2D1_POINT_2F a, D2D1_POINT_2F b) {
    float cross = (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
    if (std::fabs(cross) > EPS) return false;
    float dot = (p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y);
    if (dot < -EPS) return false;
    float len2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
    if (dot > len2 + EPS) return false;
    return true;
}

// 直线-直线（线段）交点
std::vector<D2D1_POINT_2F> lineLine(D2D1_POINT_2F p1, D2D1_POINT_2F p2,
                                    D2D1_POINT_2F q1, D2D1_POINT_2F q2) {
    std::vector<D2D1_POINT_2F> out;
    float dx1 = p2.x - p1.x, dy1 = p2.y - p1.y;
    float dx2 = q2.x - q1.x, dy2 = q2.y - q1.y;
    float den = dx1 * dy2 - dy1 * dx2;
    if (std::fabs(den) < EPS) return out; // 平行
    float ua = ((q1.x - p1.x) * dy2 - (q1.y - p1.y) * dx2) / den;
    if (ua < 0.f || ua > 1.f) return out; // 在线段外
    float ub = ((q1.x - p1.x) * dy1 - (q1.y - p1.y) * dx1) / den;
    if (ub < 0.f || ub > 1.f) return out;
    out.push_back(D2D1::Point2F(p1.x + ua * dx1, p1.y + ua * dy1));
    return out;
}

// 直线-圆交点
std::vector<D2D1_POINT_2F> lineCircle(D2D1_POINT_2F a, D2D1_POINT_2F b,
                                      D2D1_POINT_2F ctr, float r) {
    std::vector<D2D1_POINT_2F> out;
    float dx = b.x - a.x, dy = b.y - a.y;
    float fx = a.x - ctr.x, fy = a.y - ctr.y;
    float A = dx * dx + dy * dy;
    float B = 2 * (fx * dx + fy * dy);
    float C = fx * fx + fy * fy - r * r;
    float D = B * B - 4 * A * C;
    if (D < 0.f) return out;
    D = std::sqrt(D);
    float t1 = (-B - D) / (2 * A);
    float t2 = (-B + D) / (2 * A);
    if (t1 >= 0.f && t1 <= 1.f)
        out.push_back(D2D1::Point2F(a.x + t1 * dx, a.y + t1 * dy));
    if (t2 >= 0.f && t2 <= 1.f)
        out.push_back(D2D1::Point2F(a.x + t2 * dx, a.y + t2 * dy));
    return out;
}

// 圆-圆交点
std::vector<D2D1_POINT_2F> circleCircle(D2D1_POINT_2F c1, float r1,
                                        D2D1_POINT_2F c2, float r2) {
    std::vector<D2D1_POINT_2F> out;
    float dx = c2.x - c1.x, dy = c2.y - c1.y;
    float d2 = dx * dx + dy * dy, d = std::sqrt(d2);
    if (d > r1 + r2 || d < std::fabs(r1 - r2)) return out;
    float a = (r1 * r1 - r2 * r2 + d2) / (2 * d);
    float h = std::sqrt(r1 * r1 - a * a);
    float cx = c1.x + a * dx / d;
    float cy = c1.y + a * dy / d;
    float rx = -dy * h / d, ry = dx * h / d;
    out.push_back(D2D1::Point2F(cx + rx, cy + ry));
    if (h > EPS)
        out.push_back(D2D1::Point2F(cx - rx, cy - ry));
    return out;
}

// 获取多边形边集（矩形/三角形/菱形/平行四边形）
template <class T>
std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> edges(const T &poly) {
    return poly.GetIntersectionSegments();
}
} // namespace

// 单例
IntersectionManager &IntersectionManager::getInstance() {
    static IntersectionManager inst;
    return inst;
}

bool IntersectionManager::selectShape(std::shared_ptr<Shape> shape) {
    if (!shape1) {
        shape1 = shape;
        return true;
    }
    if (!shape2) {
        shape2 = shape;
        return true;
    }
    shape2 = shape; // 覆盖最后一个
    return true;
}

void IntersectionManager::clear() {
    shape1.reset();
    shape2.reset();
    intersectionPoints.clear();
}

bool IntersectionManager::hasTwoShapes() const {
    return shape1 && shape2;
}

std::vector<D2D1_POINT_2F> IntersectionManager::calculateIntersection() {
    intersectionPoints = calculateIntersectionImpl();
    return intersectionPoints;
}

const std::vector<D2D1_POINT_2F> &IntersectionManager::getIntersectionPoints() const {
    return intersectionPoints;
}

// 真正几何级实现（C++11 语法）
std::vector<D2D1_POINT_2F> IntersectionManager::calculateIntersectionImpl() {
    intersectionPoints.clear();
    if (!shape1 || !shape2) return intersectionPoints;

    Shape &a = *shape1;
    Shape &b = *shape2;

    // 直线 vs 直线
    if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::LINE) {
        Line &l1 = static_cast<Line &>(a);
        Line &l2 = static_cast<Line &>(b);
        intersectionPoints = lineLine(l1.GetStart(), l1.GetEnd(),
                                      l2.GetStart(), l2.GetEnd());
    }
    // 直线 vs 圆
    else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::CIRCLE) {
        Line &l = static_cast<Line &>(a);
        Circle &c = static_cast<Circle &>(b);
        intersectionPoints = lineCircle(l.GetStart(), l.GetEnd(),
                                        c.GetCenter(), c.GetRadius());
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::LINE) {
        Circle &c = static_cast<Circle &>(a);
        Line &l = static_cast<Line &>(b);
        intersectionPoints = lineCircle(l.GetStart(), l.GetEnd(),
                                        c.GetCenter(), c.GetRadius());
    }
    // 圆 vs 圆
    else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::CIRCLE) {
        Circle &c1 = static_cast<Circle &>(a);
        Circle &c2 = static_cast<Circle &>(b);
        intersectionPoints = circleCircle(c1.GetCenter(), c1.GetRadius(),
                                          c2.GetCenter(), c2.GetRadius());
    }
    // 直线 vs 矩形
    else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::RECTANGLE) {
        Line &l = static_cast<Line &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineLine(l.GetStart(), l.GetEnd(), segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::LINE) {
        return calculateIntersectionImpl(); // 交换再算
    }
    // 圆 vs 矩形
    else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::RECTANGLE) {
        Circle &c = static_cast<Circle &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineCircle(segs[i].first, segs[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::CIRCLE) {
        return calculateIntersectionImpl(); // 交换
    }
    // 矩形 vs 矩形
    else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::RECTANGLE) {
        Rect &r1 = static_cast<Rect &>(a);
        Rect &r2 = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(r1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(r2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 三角形 vs 直线
    else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::LINE) {
        Triangle &t = static_cast<Triangle &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(t);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineLine(s[i].first, s[i].second, l.GetStart(), l.GetEnd());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::TRIANGLE) {
        return calculateIntersectionImpl(); // 交换
    }
    // 圆 vs 三角形
    else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::TRIANGLE) {
        Circle &c = static_cast<Circle &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(t);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::CIRCLE) {
        return calculateIntersectionImpl(); // 交换
    }
    // 三角形 vs 三角形
    else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::TRIANGLE) {
        Triangle &t1 = static_cast<Triangle &>(a);
        Triangle &t2 = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(t1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(t2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 菱形 vs 直线
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::LINE) {
        Diamond &d = static_cast<Diamond &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(d);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineLine(s[i].first, s[i].second, l.GetStart(), l.GetEnd());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::DIAMOND) {
        return calculateIntersectionImpl(); // 交换
    }
    // 菱形 vs 圆
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::CIRCLE) {
        Diamond &d = static_cast<Diamond &>(a);
        Circle &c = static_cast<Circle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(d);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::DIAMOND) {
        return calculateIntersectionImpl(); // 交换
    }
    // 菱形 vs 矩形
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::RECTANGLE) {
        Diamond &d = static_cast<Diamond &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(r);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::DIAMOND) {
        return calculateIntersectionImpl(); // 交换
    }
    // 菱形 vs 三角形
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::TRIANGLE) {
        Diamond &d = static_cast<Diamond &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(t);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::DIAMOND) {
        return calculateIntersectionImpl(); // 交换
    }
    // 菱形 vs 菱形
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::DIAMOND) {
        Diamond &d1 = static_cast<Diamond &>(a);
        Diamond &d2 = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(d2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 平行四边形 vs 直线
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::LINE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineLine(s[i].first, s[i].second, l.GetStart(), l.GetEnd());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::PARALLELOGRAM) {
        return calculateIntersectionImpl(); // 交换
    }
    // 平行四边形 vs 圆
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::CIRCLE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Circle &c = static_cast<Circle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts =
                lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::PARALLELOGRAM) {
        return calculateIntersectionImpl(); // 交换
    }
    // 平行四边形 vs 矩形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::RECTANGLE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(r);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::PARALLELOGRAM) {
        return calculateIntersectionImpl(); // 交换
    }
    // 平行四边形 vs 三角形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::TRIANGLE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(t);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::PARALLELOGRAM) {
        return calculateIntersectionImpl(); // 交换
    }
    // 平行四边形 vs 菱形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::DIAMOND) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(d);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::PARALLELOGRAM) {
        return calculateIntersectionImpl(); // 交换
    }
    // 平行四边形 vs 平行四边形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::PARALLELOGRAM) {
        Parallelogram &p1 = static_cast<Parallelogram &>(a);
        Parallelogram &p2 = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts =
                    lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }

    // 去重
    std::vector<D2D1_POINT_2F>::iterator it =
        std::unique(intersectionPoints.begin(), intersectionPoints.end(),
                    [](const D2D1_POINT_2F &p, const D2D1_POINT_2F &q) {
                        return std::fabs(p.x - q.x) < EPS && std::fabs(p.y - q.y) < EPS;
                    });
    intersectionPoints.erase(it, intersectionPoints.end());
    return intersectionPoints;
}