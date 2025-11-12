#include "Shape.h"
#include <cmath>
#include <sstream>
#include <algorithm>

std::shared_ptr<Shape> Shape::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type;

    if (type == "Line") {
        D2D1_POINT_2F start, end;
        iss >> start.x >> start.y >> end.x >> end.y;
        return std::make_shared<Line>(start, end);
    } else if (type == "Circle") {
        D2D1_POINT_2F center;
        float radius;
        iss >> center.x >> center.y >> radius;
        return std::make_shared<Circle>(center, radius);
    } else if (type == "Rect") {
        D2D1_POINT_2F start, end;
        iss >> start.x >> start.y >> end.x >> end.y;
        return std::make_shared<Rect>(start, end);
    } else if (type == "Triangle") {
        D2D1_POINT_2F p1, p2, p3;
        iss >> p1.x >> p1.y >> p2.x >> p2.y >> p3.x >> p3.y;
        return std::make_shared<Triangle>(p1, p2, p3);
    } else if (type == "Diamond") {
        D2D1_POINT_2F center;
        float radiusX, radiusY, angle;
        iss >> center.x >> center.y >> radiusX >> radiusY >> angle;
        return std::make_shared<Diamond>(center, radiusX, radiusY, angle);
    } else if (type == "Parallelogram") {
        D2D1_POINT_2F p1, p2, p3;
        iss >> p1.x >> p1.y >> p2.x >> p2.y >> p3.x >> p3.y;
        return std::make_shared<Parallelogram>(p1, p2, p3);
    } else if (type == "Curve") {
        D2D1_POINT_2F start, control1, control2, end;
        iss >> start.x >> start.y >> control1.x >> control1.y
            >> control2.x >> control2.y >> end.x >> end.y;
        return std::make_shared<Curve>(start, control1, control2, end);
    } else if (type == "Polyline") {
        std::vector<D2D1_POINT_2F> points;
        size_t pointCount;
        iss >> pointCount;
        for (size_t i = 0; i < pointCount; ++i) {
            D2D1_POINT_2F point;
            iss >> point.x >> point.y;
            points.push_back(point);
        }
        return std::make_shared<Poly>(points);
    }

    return nullptr; // 未知类型
}

// 点到线段距离平方，若 < distThresh 则命中
static bool PointNearSegment(D2D1_POINT_2F p,
                             D2D1_POINT_2F a,
                             D2D1_POINT_2F b,
                             float distThresh = 5.0f) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float len2 = dx * dx + dy * dy;
    if (len2 == 0.f) // 退化
        return (p.x - a.x) * (p.x - a.x) + (p.y - a.y) * (p.y - a.y)
               <= distThresh * distThresh;

    float t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
    t = max(0.0f, min(1.0f, t)); // 投影裁剪

    float nearX = a.x + t * dx;
    float nearY = a.y + t * dy;
    float dist2 = (p.x - nearX) * (p.x - nearX) + (p.y - nearY) * (p.y - nearY);
    return dist2 <= distThresh * distThresh;
}

// Line 实现
Line::Line(D2D1_POINT_2F start, D2D1_POINT_2F end) :
    Shape(ShapeType::LINE), m_start(start), m_end(end) {
}

void Line::Draw(ID2D1RenderTarget *pRenderTarget,
                ID2D1SolidColorBrush *pBrush,
                ID2D1SolidColorBrush *pSelectedBrush,
                ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pBrush;
    if (m_isSelected && pDashStrokeStyle)
        pRenderTarget->DrawLine(m_start, m_end, currentBrush, 2.0f, pDashStrokeStyle);
    else
        pRenderTarget->DrawLine(m_start, m_end, currentBrush, 2.0f);
}

bool Line::HitTest(D2D1_POINT_2F point) {
    // 简单的点击测试：计算点到直线的距离
    float a = point.x - m_start.x;
    float b = point.y - m_start.y;
    float c = m_end.x - m_start.x;
    float d = m_end.y - m_start.y;

    float dot = a * c + b * d;
    float lenSq = c * c + d * d;
    float param = -1;
    if (lenSq != 0) // 避免除以0
        param = dot / lenSq;

    float xx, yy;

    if (param < 0) {
        xx = m_start.x;
        yy = m_start.y;
    } else if (param > 1) {
        xx = m_end.x;
        yy = m_end.y;
    } else {
        xx = m_start.x + param * c;
        yy = m_start.y + param * d;
    }

    float dx = point.x - xx;
    float dy = point.y - yy;
    return (dx * dx + dy * dy) < 100.0f; // 10像素以内算点击
}

void Line::Move(float dx, float dy) {
    m_start.x += dx;
    m_start.y += dy;
    m_end.x += dx;
    m_end.y += dy;
}

void Line::Rotate(float angle) {
    // 旋转基于中心点
    D2D1_POINT_2F center = {(m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2};
    float s = sinf(angle);
    float c = cosf(angle);

    // 平移至原点
    m_start.x -= center.x;
    m_start.y -= center.y;
    m_end.x -= center.x;
    m_end.y -= center.y;

    // 旋转
    float newStartX = m_start.x * c - m_start.y * s;
    float newStartY = m_start.x * s + m_start.y * c;
    float newEndX = m_end.x * c - m_end.y * s;
    float newEndY = m_end.x * s + m_end.y * c;

    // 平移回原位置
    m_start.x = newStartX + center.x;
    m_start.y = newStartY + center.y;
    m_end.x = newEndX + center.x;
    m_end.y = newEndY + center.y;
}

void Line::Scale(float scale) {
    D2D1_POINT_2F center = {(m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2};
    m_start.x = center.x + (m_start.x - center.x) * scale;
    m_start.y = center.y + (m_start.y - center.y) * scale;
    m_end.x = center.x + (m_end.x - center.x) * scale;
    m_end.y = center.y + (m_end.y - center.y) * scale;
}

std::string Line::Serialize() {
    std::ostringstream oss;
    oss << "Line " << m_start.x << " " << m_start.y << " " << m_end.x << " " << m_end.y;
    return oss.str();
}

// Circle 实现
Circle::Circle(D2D1_POINT_2F center, float radius) :
    Shape(ShapeType::CIRCLE), m_center(center), m_radius(radius) {
}

void Circle::Draw(ID2D1RenderTarget *pRenderTarget,
                  ID2D1SolidColorBrush *pNormalBrush,
                  ID2D1SolidColorBrush *pSelectedBrush,
                  ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;
    if (m_isSelected && pDashStrokeStyle)
        pRenderTarget->DrawEllipse(D2D1::Ellipse(m_center, m_radius, m_radius), currentBrush, 2.0f, pDashStrokeStyle);
    else
        pRenderTarget->DrawEllipse(D2D1::Ellipse(m_center, m_radius, m_radius), currentBrush, 2.0f);
}

bool Circle::HitTest(D2D1_POINT_2F point) {
    float dx = point.x - m_center.x;
    float dy = point.y - m_center.y;
    float d = sqrtf(dx * dx + dy * dy);
    return fabs(d - m_radius) <= 5.0f; // 5 px 线宽
}

void Circle::Move(float dx, float dy) {
    m_center.x += dx;
    m_center.y += dy;
}

void Circle::Scale(float scale) {
    m_radius *= scale;
}

std::string Circle::Serialize() {
    std::ostringstream oss;
    oss << "Circle " << m_center.x << " " << m_center.y << " " << m_radius;
    return oss.str();
}

// Rectangle 实现
Rect::Rect(D2D1_POINT_2F start, D2D1_POINT_2F end) :
    Shape(ShapeType::RECTANGLE) {
    // 初始化四个顶点
    m_points[0] = start;                         // 左上角
    m_points[1] = D2D1::Point2F(end.x, start.y); // 右上角
    m_points[2] = end;                           // 右下角
    m_points[3] = D2D1::Point2F(start.x, end.y); // 左下角
}

void Rect::Draw(ID2D1RenderTarget *pRenderTarget,
                ID2D1SolidColorBrush *pNormalBrush,
                ID2D1SolidColorBrush *pSelectedBrush,
                ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;

    // 创建矩形路径
    ID2D1PathGeometry *pPathGeometry = nullptr;
    ID2D1Factory *pFactory = nullptr;
    pRenderTarget->GetFactory(&pFactory);

    if (SUCCEEDED(pFactory->CreatePathGeometry(&pPathGeometry))) {
        ID2D1GeometrySink *pSink = nullptr;
        if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
            pSink->BeginFigure(m_points[0], D2D1_FIGURE_BEGIN_FILLED);
            for (int i = 1; i < 4; i++) {
                pSink->AddLine(m_points[i]);
            }
            pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
            pSink->Close();
            pSink->Release();
        }
        if (m_isSelected && pDashStrokeStyle)
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f, pDashStrokeStyle);
        else
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f);
        pPathGeometry->Release();
    }

    if (pFactory) pFactory->Release();
}

bool Rect::HitTest(D2D1_POINT_2F point) {
    // 四条边依次判断
    for (int i = 0; i < 4; ++i)
        if (PointNearSegment(point, m_points[i], m_points[(i + 1) & 3]))
            return true;
    return false;
}

void Rect::Move(float dx, float dy) {
    for (int i = 0; i < 4; i++) {
        m_points[i].x += dx;
        m_points[i].y += dy;
    }
}

void Rect::Rotate(float angle) {
    D2D1_POINT_2F center = GetCenter();
    float s = sinf(angle);
    float c = cosf(angle);

    for (int i = 0; i < 4; i++) {
        // 将顶点平移到原点（相对于中心点）
        float x = m_points[i].x - center.x;
        float y = m_points[i].y - center.y;

        // 应用旋转
        float newX = x * c - y * s;
        float newY = x * s + y * c;

        // 平移回原位置
        m_points[i].x = newX + center.x;
        m_points[i].y = newY + center.y;
    }
}

void Rect::Scale(float scale) {
    D2D1_POINT_2F center = GetCenter();
    for (int i = 0; i < 4; i++) {
        m_points[i].x = center.x + (m_points[i].x - center.x) * scale;
        m_points[i].y = center.y + (m_points[i].y - center.y) * scale;
    }
}

std::string Rect::Serialize() {
    std::ostringstream oss;
    oss << "Rectangle ";
    for (int i = 0; i < 4; i++) {
        oss << m_points[i].x << " " << m_points[i].y << " ";
    }
    return oss.str();
}

// Triangle 实现
Triangle::Triangle(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3) :
    Shape(ShapeType::TRIANGLE) {
    m_points[0] = p1;
    m_points[1] = p2;
    m_points[2] = p3;
}

void Triangle::Draw(ID2D1RenderTarget *pRenderTarget,
                    ID2D1SolidColorBrush *pNormalBrush,
                    ID2D1SolidColorBrush *pSelectedBrush,
                    ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;

    // 创建三角形路径
    ID2D1PathGeometry *pPathGeometry = nullptr;
    ID2D1Factory *pFactory = nullptr;
    pRenderTarget->GetFactory(&pFactory);

    if (SUCCEEDED(pFactory->CreatePathGeometry(&pPathGeometry))) {
        ID2D1GeometrySink *pSink = nullptr;
        if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
            pSink->BeginFigure(m_points[0], D2D1_FIGURE_BEGIN_FILLED);
            pSink->AddLine(m_points[1]);
            pSink->AddLine(m_points[2]);
            pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
            pSink->Close();
            pSink->Release();
        }
        if (m_isSelected && pDashStrokeStyle)
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f, pDashStrokeStyle);
        else
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f);
        pPathGeometry->Release();
    }

    if (pFactory) pFactory->Release();
}

bool Triangle::HitTest(D2D1_POINT_2F point) {
    for (int i = 0; i < 3; ++i)
        if (PointNearSegment(point, m_points[i], m_points[(i + 1) % 3]))
            return true;
    return false;
}

void Triangle::Move(float dx, float dy) {
    for (int i = 0; i < 3; i++) {
        m_points[i].x += dx;
        m_points[i].y += dy;
    }
}

void Triangle::Rotate(float angle) {
    // 计算中心点
    D2D1_POINT_2F center = {
        (m_points[0].x + m_points[1].x + m_points[2].x) / 3,
        (m_points[0].y + m_points[1].y + m_points[2].y) / 3};

    float s = sinf(angle);
    float c = cosf(angle);

    for (int i = 0; i < 3; i++) {
        // 平移至原点
        float x = m_points[i].x - center.x;
        float y = m_points[i].y - center.y;

        // 旋转
        float newX = x * c - y * s;
        float newY = x * s + y * c;

        // 平移回原位置
        m_points[i].x = newX + center.x;
        m_points[i].y = newY + center.y;
    }
}

void Triangle::Scale(float scale) {
    D2D1_POINT_2F center = {
        (m_points[0].x + m_points[1].x + m_points[2].x) / 3,
        (m_points[0].y + m_points[1].y + m_points[2].y) / 3};

    for (int i = 0; i < 3; i++) {
        m_points[i].x = center.x + (m_points[i].x - center.x) * scale;
        m_points[i].y = center.y + (m_points[i].y - center.y) * scale;
    }
}

std::string Triangle::Serialize() {
    std::ostringstream oss;
    oss << "Triangle "
        << m_points[0].x << " " << m_points[0].y << " "
        << m_points[1].x << " " << m_points[1].y << " "
        << m_points[2].x << " " << m_points[2].y;
    return oss.str();
}

// Diamond 实现
static void GetDiamondPoints(const D2D1_POINT_2F &c,
                             float rx, float ry, float ang,
                             D2D1_POINT_2F pts[4]) {
    float co = cosf(ang), sn = sinf(ang);
    D2D1_POINT_2F ux = {rx * co, rx * sn};
    D2D1_POINT_2F uy = {-ry * sn, ry * co};

    pts[0] = D2D1::Point2F(c.x + ux.x, c.y + ux.y); // 上
    pts[1] = D2D1::Point2F(c.x + uy.x, c.y + uy.y); // 右
    pts[2] = D2D1::Point2F(c.x - ux.x, c.y - ux.y); // 下
    pts[3] = D2D1::Point2F(c.x - uy.x, c.y - uy.y); // 左
}

Diamond::Diamond(D2D1_POINT_2F center, float radiusX, float radiusY, float angle) :
    Shape(ShapeType::DIAMOND), m_center(center),
    m_radiusX(radiusX), m_radiusY(radiusY), m_angle(angle) {
}

void Diamond::Draw(ID2D1RenderTarget *rt,
                   ID2D1SolidColorBrush *nrm,
                   ID2D1SolidColorBrush *sel,
                   ID2D1StrokeStyle *dash) {
    if (!rt || !nrm || !sel) return;
    ID2D1SolidColorBrush *cur = m_isSelected ? sel : nrm;
    D2D1_POINT_2F pts[4];
    GetDiamondPoints(m_center, m_radiusX, m_radiusY, m_angle, pts);

    ID2D1Factory *f = nullptr;
    rt->GetFactory(&f);
    ID2D1PathGeometry *geo = nullptr;
    if (SUCCEEDED(f->CreatePathGeometry(&geo))) {
        ID2D1GeometrySink *s = nullptr;
        if (SUCCEEDED(geo->Open(&s))) {
            s->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
            for (int i = 1; i < 4; ++i) s->AddLine(pts[i]);
            s->EndFigure(D2D1_FIGURE_END_CLOSED);
            s->Close();
            s->Release();
        }
        if (m_isSelected && dash)
            rt->DrawGeometry(geo, cur, 2.0f, dash);
        else
            rt->DrawGeometry(geo, cur, 2.0f);
        geo->Release();
    }
    if (f) f->Release();
}

// HitTest：逆旋转后菱形方程
bool Diamond::HitTest(D2D1_POINT_2F p) {
    D2D1_POINT_2F pts[4];
    GetDiamondPoints(m_center, m_radiusX, m_radiusY, m_angle, pts);
    for (int i = 0; i < 4; ++i)
        if (PointNearSegment(p, pts[i], pts[(i + 1) & 3]))
            return true;
    return false;
}

// Move
void Diamond::Move(float dx, float dy) {
    m_center.x += dx;
    m_center.y += dy;
}

// Rotate
void Diamond::Rotate(float angle) {
    m_angle += angle;
}

// Scale
void Diamond::Scale(float scale) {
    m_radiusX *= scale;
    m_radiusY *= scale;
}

// 包围盒
D2D1_RECT_F Diamond::GetBounds() const {
    D2D1_POINT_2F pts[4];
    GetDiamondPoints(m_center, m_radiusX, m_radiusY, m_angle, pts);
    float minX = pts[0].x, maxX = pts[0].x;
    float minY = pts[0].y, maxY = pts[0].y;
    for (int i = 1; i < 4; ++i) {
        minX = min(minX, pts[i].x);
        maxX = max(maxX, pts[i].x);
        minY = min(minY, pts[i].y);
        maxY = max(maxY, pts[i].y);
    }
    return D2D1::RectF(minX, minY, maxX, maxY);
}

// 离散线段
std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> Diamond::GetIntersectionSegments() const {
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs;
    D2D1_POINT_2F pts[4];
    GetDiamondPoints(m_center, m_radiusX, m_radiusY, m_angle, pts);
    for (int i = 0; i < 4; ++i)
        segs.push_back(std::make_pair(pts[i], pts[(i + 1) & 3]));
    return segs;
}

// 序列化：Diamond center.x center.y radiusX radiusY angle
std::string Diamond::Serialize() {
    std::ostringstream oss;
    oss << "Diamond " << m_center.x << " " << m_center.y << " "
        << m_radiusX << " " << m_radiusY << " " << m_angle;
    return oss.str();
}

// Parallelogram 实现
Parallelogram::Parallelogram(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3) :
    Shape(ShapeType::PARALLELOGRAM) {
    m_points[0] = p1; // 起点
    m_points[1] = p2; // 对顶点
    m_points[2] = p3; // 确定方向的点

    // 计算第四个点
    m_points[3].x = p1.x + (p3.x - p2.x);
    m_points[3].y = p1.y + (p3.y - p2.y);
}

void Parallelogram::Draw(ID2D1RenderTarget *pRenderTarget,
                         ID2D1SolidColorBrush *pNormalBrush,
                         ID2D1SolidColorBrush *pSelectedBrush,
                         ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;

    // 创建平行四边形路径
    ID2D1PathGeometry *pPathGeometry = nullptr;
    ID2D1Factory *pFactory = nullptr;
    pRenderTarget->GetFactory(&pFactory);

    if (SUCCEEDED(pFactory->CreatePathGeometry(&pPathGeometry))) {
        ID2D1GeometrySink *pSink = nullptr;
        if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
            pSink->BeginFigure(m_points[0], D2D1_FIGURE_BEGIN_FILLED);
            for (int i = 1; i < 4; i++) {
                pSink->AddLine(m_points[i]);
            }
            pSink->EndFigure(D2D1_FIGURE_END_CLOSED);
            pSink->Close();
            pSink->Release();
        }
        if (m_isSelected && pDashStrokeStyle)
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f, pDashStrokeStyle);
        else
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f);
        pPathGeometry->Release();
    }

    if (pFactory) pFactory->Release();
}

bool Parallelogram::HitTest(D2D1_POINT_2F point) {
    for (int i = 0; i < 4; ++i)
        if (PointNearSegment(point, m_points[i], m_points[(i + 1) & 3]))
            return true;
    return false;
}

void Parallelogram::Move(float dx, float dy) {
    for (int i = 0; i < 4; i++) {
        m_points[i].x += dx;
        m_points[i].y += dy;
    }
}

void Parallelogram::Rotate(float angle) {
    // 计算中心点
    D2D1_POINT_2F center = {0, 0};
    for (int i = 0; i < 4; i++) {
        center.x += m_points[i].x;
        center.y += m_points[i].y;
    }
    center.x /= 4;
    center.y /= 4;

    float s = sinf(angle);
    float c = cosf(angle);

    for (int i = 0; i < 4; i++) {
        // 平移至原点
        float x = m_points[i].x - center.x;
        float y = m_points[i].y - center.y;

        // 旋转
        float newX = x * c - y * s;
        float newY = x * s + y * c;

        // 平移回原位置
        m_points[i].x = newX + center.x;
        m_points[i].y = newY + center.y;
    }
}

void Parallelogram::Scale(float scale) {
    D2D1_POINT_2F center = {0, 0};
    for (int i = 0; i < 4; i++) {
        center.x += m_points[i].x;
        center.y += m_points[i].y;
    }
    center.x /= 4;
    center.y /= 4;

    for (int i = 0; i < 4; i++) {
        m_points[i].x = center.x + (m_points[i].x - center.x) * scale;
        m_points[i].y = center.y + (m_points[i].y - center.y) * scale;
    }
}

std::string Parallelogram::Serialize() {
    std::ostringstream oss;
    oss << "Parallelogram "
        << m_points[0].x << " " << m_points[0].y << " "
        << m_points[1].x << " " << m_points[1].y << " "
        << m_points[2].x << " " << m_points[2].y;
    return oss.str();
}

// Curve 实现
Curve::Curve(D2D1_POINT_2F start, D2D1_POINT_2F control1, D2D1_POINT_2F control2, D2D1_POINT_2F end) :
    Shape(ShapeType::CURVE) {
    m_points.push_back(start);
    m_points.push_back(control1);
    m_points.push_back(control2);
    m_points.push_back(end);
}

static D2D1_POINT_2F EvaluateCubicBezier(const D2D1_POINT_2F &p0,
                                         const D2D1_POINT_2F &p1,
                                         const D2D1_POINT_2F &p2,
                                         const D2D1_POINT_2F &p3,
                                         float t) {
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float t2 = t * t;

    D2D1_POINT_2F r;
    r.x = mt2 * mt * p0.x + 3.0f * mt2 * t * p1.x + 3.0f * mt * t2 * p2.x + t2 * t * p3.x;
    r.y = mt2 * mt * p0.y + 3.0f * mt2 * t * p1.y + 3.0f * mt * t2 * p2.y + t2 * t * p3.y;
    return r;
}

void Curve::Draw(ID2D1RenderTarget *pRenderTarget,
                 ID2D1SolidColorBrush *pNormalBrush,
                 ID2D1SolidColorBrush *pSelectedBrush,
                 ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;
    if (m_points.size() != 4) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;

    ID2D1Factory *pFactory = nullptr;
    pRenderTarget->GetFactory(&pFactory);

    ID2D1PathGeometry *pPathGeometry = nullptr;
    if (SUCCEEDED(pFactory->CreatePathGeometry(&pPathGeometry))) {
        ID2D1GeometrySink *pSink = nullptr;
        if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
            // 手工离散 Bézier
            const D2D1_POINT_2F &p0 = m_points[0];
            const D2D1_POINT_2F &p1 = m_points[1];
            const D2D1_POINT_2F &p2 = m_points[2];
            const D2D1_POINT_2F &p3 = m_points[3];

            pSink->BeginFigure(p0, D2D1_FIGURE_BEGIN_HOLLOW);

            for (int i = 1; i <= CURVE_FLATTEN_SEGS; ++i) {
                float t = float(i) / CURVE_FLATTEN_SEGS;
                D2D1_POINT_2F pt = EvaluateCubicBezier(p0, p1, p2, p3, t);
                pSink->AddLine(pt);
            }

            pSink->EndFigure(D2D1_FIGURE_END_OPEN);
            pSink->Close();
            pSink->Release();
        }

        if (m_isSelected && pDashStrokeStyle)
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f, pDashStrokeStyle);
        else
            pRenderTarget->DrawGeometry(pPathGeometry, currentBrush, 2.0f);

        pPathGeometry->Release();
    }

    if (pFactory) pFactory->Release();
}

bool Curve::HitTest(D2D1_POINT_2F point) {
    if (m_points.size() != 4) return false;

    // 将贝塞尔曲线细分为多个小线段，然后检测点是否靠近这些线段
    const int segments = CURVE_FLATTEN_SEGS;
    for (int i = 0; i < segments; i++) {
        float t1 = static_cast<float>(i) / segments;
        float t2 = static_cast<float>(i + 1) / segments;

        // 计算贝塞尔曲线上的两个点
        D2D1_POINT_2F p1 = CalculateBezierPoint(t1);
        D2D1_POINT_2F p2 = CalculateBezierPoint(t2);

        // 计算点到线段的距离
        float a = point.x - p1.x;
        float b = point.y - p1.y;
        float c = p2.x - p1.x;
        float d = p2.y - p1.y;

        float dot = a * c + b * d;
        float lenSq = c * c + d * d;
        float param = -1;
        if (lenSq != 0)
            param = dot / lenSq;

        float xx, yy;

        if (param < 0) {
            xx = p1.x;
            yy = p1.y;
        } else if (param > 1) {
            xx = p2.x;
            yy = p2.y;
        } else {
            xx = p1.x + param * c;
            yy = p1.y + param * d;
        }

        float dx = point.x - xx;
        float dy = point.y - yy;
        if ((dx * dx + dy * dy) < 25.0f) {
            return true;
        }
    }
    return false;
}

D2D1_POINT_2F Curve::CalculateBezierPoint(float t) const {
    if (m_points.size() != 4) {
        return D2D1::Point2F(0, 0);
    }

    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    D2D1_POINT_2F p;
    p.x = uuu * m_points[0].x + 3 * uu * t * m_points[1].x + 3 * u * tt * m_points[2].x + ttt * m_points[3].x;
    p.y = uuu * m_points[0].y + 3 * uu * t * m_points[1].y + 3 * u * tt * m_points[2].y + ttt * m_points[3].y;

    return p;
}

void Curve::Move(float dx, float dy) {
    for (auto &point : m_points) {
        point.x += dx;
        point.y += dy;
    }
}

void Curve::Rotate(float angle) {
    // 计算中心点
    D2D1_POINT_2F center = {0, 0};
    for (const auto &point : m_points) {
        center.x += point.x;
        center.y += point.y;
    }
    center.x /= m_points.size();
    center.y /= m_points.size();

    float s = sinf(angle);
    float c = cosf(angle);

    for (auto &point : m_points) {
        // 平移至原点
        float x = point.x - center.x;
        float y = point.y - center.y;

        // 旋转
        float newX = x * c - y * s;
        float newY = x * s + y * c;

        // 平移回原位置
        point.x = newX + center.x;
        point.y = newY + center.y;
    }
}

void Curve::Scale(float scale) {
    D2D1_POINT_2F center = {0, 0};
    for (const auto &point : m_points) {
        center.x += point.x;
        center.y += point.y;
    }
    center.x /= m_points.size();
    center.y /= m_points.size();

    for (auto &point : m_points) {
        point.x = center.x + (point.x - center.x) * scale;
        point.y = center.y + (point.y - center.y) * scale;
    }
}

std::string Curve::Serialize() {
    std::ostringstream oss;
    oss << "Curve ";
    for (const auto &point : m_points) {
        oss << point.x << " " << point.y << " ";
    }
    return oss.str();
}

std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> Curve::GetIntersectionSegments() const {
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segs;
    const int n = CURVE_FLATTEN_SEGS;
    for (int i = 0; i < n; ++i) {
        float t1 = float(i) / n;
        float t2 = float(i + 1) / n;
        segs.emplace_back(CalculateBezierPoint(t1),
                          CalculateBezierPoint(t2));
    }
    return segs;
}

// Polyline 实现
Poly::Poly(const std::vector<D2D1_POINT_2F> &points) :
    Shape(ShapeType::POLYLINE), m_points(points) {
}

void Poly::Draw(ID2D1RenderTarget *pRenderTarget,
                ID2D1SolidColorBrush *pNormalBrush,
                ID2D1SolidColorBrush *pSelectedBrush,
                ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;
    if (m_points.size() < 2) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;

    // 绘制所有线段
    for (size_t i = 1; i < m_points.size(); i++) {
        if (m_isSelected && pDashStrokeStyle)
            pRenderTarget->DrawLine(m_points[i - 1], m_points[i], currentBrush, 2.0f, pDashStrokeStyle);
        else
            pRenderTarget->DrawLine(m_points[i - 1], m_points[i], currentBrush, 2.0f);
    }

    // 如果被选中，可以添加额外的视觉效果，比如控制点
    if (m_isSelected) {
        // 绘制顶点控制点
        ID2D1SolidColorBrush *controlBrush = nullptr;
        pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &controlBrush);
        if (controlBrush) {
            for (const auto &point : m_points) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(point, 3.0f, 3.0f);
                pRenderTarget->FillEllipse(ellipse, controlBrush);
            }
            controlBrush->Release();
        }
    }
}

bool Poly::HitTest(D2D1_POINT_2F point) {
    // 检查是否点击了任何一段线段
    for (size_t i = 1; i < m_points.size(); i++) {
        D2D1_POINT_2F p1 = m_points[i - 1];
        D2D1_POINT_2F p2 = m_points[i];

        // 计算点到线段的距离
        float a = point.x - p1.x;
        float b = point.y - p1.y;
        float c = p2.x - p1.x;
        float d = p2.y - p1.y;

        float dot = a * c + b * d;
        float lenSq = c * c + d * d;
        float param = -1;
        if (lenSq != 0)
            param = dot / lenSq;

        float xx, yy;

        if (param < 0) {
            xx = p1.x;
            yy = p1.y;
        } else if (param > 1) {
            xx = p2.x;
            yy = p2.y;
        } else {
            xx = p1.x + param * c;
            yy = p1.y + param * d;
        }

        float dx = point.x - xx;
        float dy = point.y - yy;
        if ((dx * dx + dy * dy) < 25.0f) { // 5像素以内算点击
            return true;
        }
    }
    return false;
}

void Poly::Move(float dx, float dy) {
    for (auto &point : m_points) {
        point.x += dx;
        point.y += dy;
    }
}

void Poly::Rotate(float angle) {
    // 计算中心点
    D2D1_POINT_2F center = {0, 0};
    for (const auto &point : m_points) {
        center.x += point.x;
        center.y += point.y;
    }
    center.x /= m_points.size();
    center.y /= m_points.size();

    float s = sinf(angle);
    float c = cosf(angle);

    for (auto &point : m_points) {
        // 平移至原点
        float x = point.x - center.x;
        float y = point.y - center.y;

        // 旋转
        float newX = x * c - y * s;
        float newY = x * s + y * c;

        // 平移回原位置
        point.x = newX + center.x;
        point.y = newY + center.y;
    }
}

void Poly::Scale(float scale) {
    D2D1_POINT_2F center = {0, 0};
    for (const auto &point : m_points) {
        center.x += point.x;
        center.y += point.y;
    }
    center.x /= m_points.size();
    center.y /= m_points.size();

    for (auto &point : m_points) {
        point.x = center.x + (point.x - center.x) * scale;
        point.y = center.y + (point.y - center.y) * scale;
    }
}

void Poly::AddPoint(D2D1_POINT_2F point) {
    m_points.push_back(point);
}

std::string Poly::Serialize() {
    std::ostringstream oss;
    oss << "Polyline " << m_points.size();
    for (const auto &point : m_points) {
        oss << " " << point.x << " " << point.y;
    }
    return oss.str();
}
