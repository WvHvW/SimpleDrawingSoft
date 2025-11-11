#include "IntersectionManager.h"
#include <cmath>
#include <algorithm>

namespace {
const float EPS = 1e-5f;

bool pointOnSegment(D2D1_POINT_2F p, D2D1_POINT_2F a, D2D1_POINT_2F b) {
    float cross = (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
    if (std::fabs(cross) > EPS) return false;
    float dot = (p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y);
    if (dot < -EPS) return false;
    float len2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
    if (dot > len2 + EPS) return false;
    return true;
}

std::vector<D2D1_POINT_2F> lineLine(D2D1_POINT_2F p1, D2D1_POINT_2F p2,
                                    D2D1_POINT_2F q1, D2D1_POINT_2F q2) {
    std::vector<D2D1_POINT_2F> out;
    float dx1 = p2.x - p1.x, dy1 = p2.y - p1.y;
    float dx2 = q2.x - q1.x, dy2 = q2.y - q1.y;
    float den = dx1 * dy2 - dy1 * dx2;
    if (std::fabs(den) < EPS) return out;
    float ua = ((q1.x - p1.x) * dy2 - (q1.y - p1.y) * dx2) / den;
    if (ua < 0.f || ua > 1.f) return out;
    float ub = ((q1.x - p1.x) * dy1 - (q1.y - p1.y) * dx1) / den;
    if (ub < 0.f || ub > 1.f) return out;
    out.push_back(D2D1::Point2F(p1.x + ua * dx1, p1.y + ua * dy1));
    return out;
}

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

// --------------- 贝塞尔曲线工具 ---------------
// 计算三次贝塞尔在 t 处的点
D2D1_POINT_2F BezierPoint(const D2D1_POINT_2F p[4], float t) {
    float u = 1.0f - t;
    float uu = u * u, tt = t * t;
    float uuu = uu * u, ttt = tt * t;
    float uut = uu * t, utt = u * tt;

    return D2D1::Point2F(
        uuu * p[0].x + 3 * uut * p[1].x + 3 * utt * p[2].x + ttt * p[3].x,
        uuu * p[0].y + 3 * uut * p[1].y + 3 * utt * p[2].y + ttt * p[3].y);
}

// 三阶 Bézier 的 de Casteljau 一剖二
void SplitBezier(const D2D1_POINT_2F p[4],
                 D2D1_POINT_2F left[4], D2D1_POINT_2F right[4]) {
    left[0] = p[0];
    right[3] = p[3];

    D2D1_POINT_2F q1 = D2D1::Point2F((p[0].x + p[1].x) * 0.5f, (p[0].y + p[1].y) * 0.5f);
    D2D1_POINT_2F q2 = D2D1::Point2F((p[1].x + p[2].x) * 0.5f, (p[1].y + p[2].y) * 0.5f);
    D2D1_POINT_2F q3 = D2D1::Point2F((p[2].x + p[3].x) * 0.5f, (p[2].y + p[3].y) * 0.5f);

    D2D1_POINT_2F r2 = D2D1::Point2F((q1.x + q2.x) * 0.5f, (q1.y + q2.y) * 0.5f);
    D2D1_POINT_2F r3 = D2D1::Point2F((q2.x + q3.x) * 0.5f, (q2.y + q3.y) * 0.5f);

    D2D1_POINT_2F mid = D2D1::Point2F((r2.x + r3.x) * 0.5f, (r2.y + r3.y) * 0.5f);

    left[1] = q1;
    left[2] = r2;
    left[3] = mid;
    right[0] = mid;
    right[1] = r3;
    right[2] = q3;
}

// 平直度检测：用 chord-height 误差
bool IsFlatEnough(const D2D1_POINT_2F p[4], float tol) {
    D2D1_POINT_2F d1 = D2D1::Point2F(p[1].x - p[0].x, p[1].y - p[0].y);
    D2D1_POINT_2F d2 = D2D1::Point2F(p[2].x - p[0].x, p[2].y - p[0].y);
    D2D1_POINT_2F d3 = D2D1::Point2F(p[3].x - p[0].x, p[3].y - p[0].y);

    // 向量叉积 = 2*三角形面积
    float cross1 = d1.x * d2.y - d1.y * d2.x;
    float cross2 = d2.x * d3.y - d2.y * d3.x;
    float chord2 = d3.x * d3.x + d3.y * d3.y;
    if (chord2 < 1e-8f) return true; // 退化
    float err = (fabs(cross1) + fabs(cross2)) / sqrtf(chord2);
    return err <= tol;
}

// 细分直到“足够直”，返回线段序列
// 递归细分，tol 建议 0.25~0.5 像素
static void AdaptiveFlatten(const D2D1_POINT_2F p[4],
                            float tol,
                            std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> &out) {
    if (IsFlatEnough(p, tol)) {
        out.emplace_back(p[0], p[3]);
        return;
    }
    D2D1_POINT_2F L[4], R[4];
    SplitBezier(p, L, R);
    AdaptiveFlatten(L, tol, out);
    AdaptiveFlatten(R, tol, out);
}

std::vector<D2D1_POINT_2F> curveLine(const std::vector<D2D1_POINT_2F> &curve,
                                     D2D1_POINT_2F a, D2D1_POINT_2F b) {
    D2D1_POINT_2F p[4] = {curve[0], curve[1], curve[2], curve[3]};
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs;
    AdaptiveFlatten(p, 0.3f, segs);

    std::vector<D2D1_POINT_2F> out;
    for (const auto &seg : segs) {
        auto pts = lineLine(seg.first, seg.second, a, b);
        out.insert(out.end(), pts.begin(), pts.end());
    }
    return out;
}

std::vector<D2D1_POINT_2F> curveCircle(const std::vector<D2D1_POINT_2F> &curve,
                                       D2D1_POINT_2F ctr, float r) {
    D2D1_POINT_2F p[4] = {curve[0], curve[1], curve[2], curve[3]};
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs;
    AdaptiveFlatten(p, 0.3f, segs);

    std::vector<D2D1_POINT_2F> out;
    for (const auto &seg : segs) {
        auto pts = lineCircle(seg.first, seg.second, ctr, r);
        out.insert(out.end(), pts.begin(), pts.end());
    }
    return out;
}
// 简易包围盒
struct Box {
    float minX, minY, maxX, maxY;
};
Box GetBox(const D2D1_POINT_2F p[4]) {
    Box b{p[0].x, p[0].y, p[0].x, p[0].y};
    for (int i = 1; i < 4; ++i) {
        b.minX = min(b.minX, p[i].x);
        b.maxX = max(b.maxX, p[i].x);
        b.minY = min(b.minY, p[i].y);
        b.maxY = max(b.maxY, p[i].y);
    }
    return b;
}
bool BoxIntersect(const Box &a, const Box &b) {
    return a.maxX >= b.minX && b.maxX >= a.minX && a.maxY >= b.minY && b.maxY >= a.minY;
}

// 递归区间剔除
void CurveCurveRecursive(const D2D1_POINT_2F A[4], const D2D1_POINT_2F B[4],
                         float tol,
                         std::vector<D2D1_POINT_2F> &out) {
    Box bA = GetBox(A), bB = GetBox(B);
    if (!BoxIntersect(bA, bB)) return;

    // 当两条都足够扁，当成直线段求交
    if (IsFlatEnough(A, tol) && IsFlatEnough(B, tol)) {
        auto pts = lineLine(A[0], A[3], B[0], B[3]);
        out.insert(out.end(), pts.begin(), pts.end());
        return;
    }
    // 否则把更“弯”的那条劈一半
    D2D1_POINT_2F AL[4], AR[4], BL[4], BR[4];
    if (!IsFlatEnough(A, tol)) {
        SplitBezier(A, AL, AR);
        SplitBezier(B, BL, BR);
    } else {
        SplitBezier(B, BL, BR);
        std::copy(A, A + 4, AL);
        std::copy(A, A + 4, AR);
    }

    CurveCurveRecursive(AL, BL, tol, out);
    CurveCurveRecursive(AL, BR, tol, out);
    CurveCurveRecursive(AR, BL, tol, out);
    CurveCurveRecursive(AR, BR, tol, out);
}

template <class T>
std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> edges(const T &poly) {
    return poly.GetIntersectionSegments();
}

} // namespace

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
    shape2 = shape;
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

std::vector<D2D1_POINT_2F> IntersectionManager::calculateIntersectionImpl() {
    intersectionPoints.clear();
    if (!shape1 || !shape2) return intersectionPoints;

    Shape &a = *shape1;
    Shape &b = *shape2;

    // 直线 vs 直线
    if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::LINE) {
        Line &l1 = static_cast<Line &>(a);
        Line &l2 = static_cast<Line &>(b);
        intersectionPoints = lineLine(l1.GetStart(), l1.GetEnd(), l2.GetStart(), l2.GetEnd());
    }
    // 直线 vs 圆
    else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::CIRCLE) {
        Line &l = static_cast<Line &>(a);
        Circle &c = static_cast<Circle &>(b);
        intersectionPoints = lineCircle(l.GetStart(), l.GetEnd(), c.GetCenter(), c.GetRadius());
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::LINE) {
        Circle &c = static_cast<Circle &>(a);
        Line &l = static_cast<Line &>(b);
        intersectionPoints = lineCircle(l.GetStart(), l.GetEnd(), c.GetCenter(), c.GetRadius());
    }
    // 圆 vs 圆
    else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::CIRCLE) {
        Circle &c1 = static_cast<Circle &>(a);
        Circle &c2 = static_cast<Circle &>(b);
        intersectionPoints = circleCircle(c1.GetCenter(), c1.GetRadius(), c2.GetCenter(), c2.GetRadius());
    }
    // 直线 vs 矩形
    else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::RECTANGLE) {
        Line &l = static_cast<Line &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(l.GetStart(), l.GetEnd(), segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::LINE) {
        Rect &r = static_cast<Rect &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(l.GetStart(), l.GetEnd(), segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 圆 vs 矩形
    else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::RECTANGLE) {
        Circle &c = static_cast<Circle &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(segs[i].first, segs[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::CIRCLE) {
        Rect &r = static_cast<Rect &>(a);
        Circle &c = static_cast<Circle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(segs[i].first, segs[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 矩形 vs 矩形
    else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::RECTANGLE) {
        Rect &r1 = static_cast<Rect &>(a);
        Rect &r2 = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(r1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(r2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 三角形 vs 直线
    else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::LINE) {
        Triangle &t = static_cast<Triangle &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(t);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(s[i].first, s[i].second, l.GetStart(), l.GetEnd());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::TRIANGLE) {
        Line &l = static_cast<Line &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(t);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(l.GetStart(), l.GetEnd(), s[i].first, s[i].second);
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 圆 vs 三角形
    else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::TRIANGLE) {
        Circle &c = static_cast<Circle &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(t);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::CIRCLE) {
        Triangle &t = static_cast<Triangle &>(a);
        Circle &c = static_cast<Circle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(t);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 三角形 vs 三角形
    else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::TRIANGLE) {
        Triangle &t1 = static_cast<Triangle &>(a);
        Triangle &t2 = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(t1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(t2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 菱形 vs 直线
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::LINE) {
        Diamond &d = static_cast<Diamond &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(d);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(s[i].first, s[i].second, l.GetStart(), l.GetEnd());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::DIAMOND) {
        Line &l = static_cast<Line &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(d);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(l.GetStart(), l.GetEnd(), s[i].first, s[i].second);
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 菱形 vs 圆
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::CIRCLE) {
        Diamond &d = static_cast<Diamond &>(a);
        Circle &c = static_cast<Circle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(d);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::DIAMOND) {
        Circle &c = static_cast<Circle &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(d);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 菱形 vs 矩形
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::RECTANGLE) {
        Diamond &d = static_cast<Diamond &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(r);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::DIAMOND) {
        Rect &r = static_cast<Rect &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(r);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(d);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 菱形 vs 三角形
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::TRIANGLE) {
        Diamond &d = static_cast<Diamond &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(t);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::DIAMOND) {
        Triangle &t = static_cast<Triangle &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(t);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(d);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 菱形 vs 菱形
    else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::DIAMOND) {
        Diamond &d1 = static_cast<Diamond &>(a);
        Diamond &d2 = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(d2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 平行四边形 vs 直线
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::LINE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(s[i].first, s[i].second, l.GetStart(), l.GetEnd());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::PARALLELOGRAM) {
        Line &l = static_cast<Line &>(a);
        Parallelogram &p = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(l.GetStart(), l.GetEnd(), s[i].first, s[i].second);
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 平行四边形 vs 圆
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::CIRCLE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Circle &c = static_cast<Circle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::PARALLELOGRAM) {
        Circle &c = static_cast<Circle &>(a);
        Parallelogram &p = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 平行四边形 vs 矩形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::RECTANGLE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(r);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::PARALLELOGRAM) {
        Rect &r = static_cast<Rect &>(a);
        Parallelogram &p = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(r);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 平行四边形 vs 三角形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::TRIANGLE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(t);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::PARALLELOGRAM) {
        Triangle &t = static_cast<Triangle &>(a);
        Parallelogram &p = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(t);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 平行四边形 vs 菱形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::DIAMOND) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(d);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::PARALLELOGRAM) {
        Diamond &d = static_cast<Diamond &>(a);
        Parallelogram &p = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 平行四边形 vs 平行四边形
    else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::PARALLELOGRAM) {
        Parallelogram &p1 = static_cast<Parallelogram &>(a);
        Parallelogram &p2 = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 多段线 vs 直线
    else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::LINE) {
        Poly &p = static_cast<Poly &>(a);
        Line &l = static_cast<Line &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(s[i].first, s[i].second, l.GetStart(), l.GetEnd());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::POLYLINE) {
        Line &l = static_cast<Line &>(a);
        Poly &p = static_cast<Poly &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineLine(l.GetStart(), l.GetEnd(), s[i].first, s[i].second);
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 多段线 vs 圆
    else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::CIRCLE) {
        Poly &p = static_cast<Poly &>(a);
        Circle &c = static_cast<Circle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::POLYLINE) {
        Circle &c = static_cast<Circle &>(a);
        Poly &p = static_cast<Poly &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s = edges(p);
        for (size_t i = 0; i < s.size(); ++i) {
            std::vector<D2D1_POINT_2F> pts = lineCircle(s[i].first, s[i].second, c.GetCenter(), c.GetRadius());
            intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
        }
    }
    // 多段线 vs 矩形
    else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::RECTANGLE) {
        Poly &p = static_cast<Poly &>(a);
        Rect &r = static_cast<Rect &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(r);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::POLYLINE) {
        Rect &r = static_cast<Rect &>(a);
        Poly &p = static_cast<Poly &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(r);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 多段线 vs 三角形
    else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::TRIANGLE) {
        Poly &p = static_cast<Poly &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(t);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::POLYLINE) {
        Triangle &t = static_cast<Triangle &>(a);
        Poly &p = static_cast<Poly &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(t);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 多段线 vs 菱形
    else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::DIAMOND) {
        Poly &p = static_cast<Poly &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(d);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::POLYLINE) {
        Diamond &d = static_cast<Diamond &>(a);
        Poly &p = static_cast<Poly &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(d);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 多段线 vs 平行四边形
    else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::PARALLELOGRAM) {
        Poly &p = static_cast<Poly &>(a);
        Parallelogram &pg = static_cast<Parallelogram &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(pg);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    } else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::POLYLINE) {
        Parallelogram &pg = static_cast<Parallelogram &>(a);
        Poly &p = static_cast<Poly &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(pg);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }
    // 多段线 vs 多段线
    else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::POLYLINE) {
        Poly &p1 = static_cast<Poly &>(a);
        Poly &p2 = static_cast<Poly &>(b);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s1 = edges(p1);
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> s2 = edges(p2);
        for (size_t i = 0; i < s1.size(); ++i)
            for (size_t j = 0; j < s2.size(); ++j) {
                std::vector<D2D1_POINT_2F> pts = lineLine(s1[i].first, s1[i].second, s2[j].first, s2[j].second);
                intersectionPoints.insert(intersectionPoints.end(), pts.begin(), pts.end());
            }
    }

    // 曲线 vs 直线
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::LINE) {
        Curve &cv = static_cast<Curve &>(a);
        Line &l = static_cast<Line &>(b);
        auto pts = cv.GetPoints(); // 返回 4 个控制点
        intersectionPoints = curveLine(pts, l.GetStart(), l.GetEnd());
    } else if (a.GetType() == ShapeType::LINE && b.GetType() == ShapeType::CURVE) {
        Line &l = static_cast<Line &>(a);
        Curve &cv = static_cast<Curve &>(b);
        auto pts = cv.GetPoints();
        intersectionPoints = curveLine(pts, l.GetStart(), l.GetEnd());
    }
    // 曲线 vs 圆
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::CIRCLE) {
        Curve &cv = static_cast<Curve &>(a);
        Circle &c = static_cast<Circle &>(b);
        auto pts = cv.GetPoints();
        intersectionPoints = curveCircle(pts, c.GetCenter(), c.GetRadius());
    } else if (a.GetType() == ShapeType::CIRCLE && b.GetType() == ShapeType::CURVE) {
        Circle &c = static_cast<Circle &>(a);
        Curve &cv = static_cast<Curve &>(b);
        auto pts = cv.GetPoints();
        intersectionPoints = curveCircle(pts, c.GetCenter(), c.GetRadius());
    }
    // 曲线 vs 矩形
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::RECTANGLE) {
        Curve &cv = static_cast<Curve &>(a);
        Rect &r = static_cast<Rect &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    } else if (a.GetType() == ShapeType::RECTANGLE && b.GetType() == ShapeType::CURVE) {
        Rect &r = static_cast<Rect &>(a);
        Curve &cv = static_cast<Curve &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(r);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    }
    // 曲线 vs 三角形
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::TRIANGLE) {
        Curve &cv = static_cast<Curve &>(a);
        Triangle &t = static_cast<Triangle &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(t);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    } else if (a.GetType() == ShapeType::TRIANGLE && b.GetType() == ShapeType::CURVE) {
        Triangle &t = static_cast<Triangle &>(a);
        Curve &cv = static_cast<Curve &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(t);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    }
    // 曲线 vs 菱形
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::DIAMOND) {
        Curve &cv = static_cast<Curve &>(a);
        Diamond &d = static_cast<Diamond &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(d);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    } else if (a.GetType() == ShapeType::DIAMOND && b.GetType() == ShapeType::CURVE) {
        Diamond &d = static_cast<Diamond &>(a);
        Curve &cv = static_cast<Curve &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(d);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    }
    // 曲线 vs 平行四边形
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::PARALLELOGRAM) {
        Curve &cv = static_cast<Curve &>(a);
        Parallelogram &p = static_cast<Parallelogram &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(p);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    } else if (a.GetType() == ShapeType::PARALLELOGRAM && b.GetType() == ShapeType::CURVE) {
        Parallelogram &p = static_cast<Parallelogram &>(a);
        Curve &cv = static_cast<Curve &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(p);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    }
    // 曲线 vs 多段线
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::POLYLINE) {
        Curve &cv = static_cast<Curve &>(a);
        Poly &p = static_cast<Poly &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(p);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    } else if (a.GetType() == ShapeType::POLYLINE && b.GetType() == ShapeType::CURVE) {
        Poly &p = static_cast<Poly &>(a);
        Curve &cv = static_cast<Curve &>(b);
        auto pts = cv.GetPoints();
        auto segs = edges(p);
        for (size_t i = 0; i < segs.size(); ++i) {
            auto tmp = curveLine(pts, segs[i].first, segs[i].second);
            intersectionPoints.insert(intersectionPoints.end(), tmp.begin(), tmp.end());
        }
    }
    // 曲线 vs 曲线
    else if (a.GetType() == ShapeType::CURVE && b.GetType() == ShapeType::CURVE) {
        Curve &cv1 = static_cast<Curve &>(a);
        Curve &cv2 = static_cast<Curve &>(b);
        auto pts1 = cv1.GetPoints();
        auto pts2 = cv2.GetPoints();
        D2D1_POINT_2F bez1[4] = {pts1[0], pts1[1], pts1[2], pts1[3]};
        D2D1_POINT_2F bez2[4] = {pts2[0], pts2[1], pts2[2], pts2[3]};
        CurveCurveRecursive(bez1, bez2, 0.3f, intersectionPoints);
    }

    // ---------------- 去重 ----------------
    std::vector<D2D1_POINT_2F>::iterator it =
        std::unique(intersectionPoints.begin(), intersectionPoints.end(),
                    [](const D2D1_POINT_2F &p, const D2D1_POINT_2F &q) {
                        return std::fabs(p.x - q.x) < EPS && std::fabs(p.y - q.y) < EPS;
                    });
    intersectionPoints.erase(it, intersectionPoints.end());
    return intersectionPoints;
}
