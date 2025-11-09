#include "Shape.h"
#include <cmath>
#include <sstream>
#include <algorithm>

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

void Line::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type >> m_start.x >> m_start.y >> m_end.x >> m_end.y;
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
    float distance = sqrtf(dx * dx + dy * dy);
    return abs(distance - m_radius) < 10.0f; // 10像素以内算点击
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

void Circle::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type >> m_center.x >> m_center.y >> m_radius;
}

// Rectangle 实现
Rectangle::Rectangle(D2D1_POINT_2F start, D2D1_POINT_2F end) :
    Shape(ShapeType::RECTANGLE), m_start(start), m_end(end) {
}

void Rectangle::Draw(ID2D1RenderTarget *pRenderTarget,
                     ID2D1SolidColorBrush *pNormalBrush,
                     ID2D1SolidColorBrush *pSelectedBrush,
                     ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;
    D2D1_RECT_F rect = D2D1::RectF(m_start.x, m_start.y, m_end.x, m_end.y);
    if (m_isSelected && pDashStrokeStyle)
        pRenderTarget->DrawRectangle(rect, currentBrush, 2.0f, pDashStrokeStyle);
    else
        pRenderTarget->DrawRectangle(rect, currentBrush, 2.0f);
}

bool Rectangle::HitTest(D2D1_POINT_2F point) {
    float left = min(m_start.x, m_end.x);
    float right = max(m_start.x, m_end.x);
    float top = min(m_start.y, m_end.y);
    float bottom = max(m_start.y, m_end.y);

    return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
}

void Rectangle::Move(float dx, float dy) {
    m_start.x += dx;
    m_start.y += dy;
    m_end.x += dx;
    m_end.y += dy;
}

void Rectangle::Rotate(float angle) {
    // 实现矩形旋转
    // 暂时留空，可以后续实现
}

void Rectangle::Scale(float scale) {
    D2D1_POINT_2F center = {(m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2};
    m_start.x = center.x + (m_start.x - center.x) * scale;
    m_start.y = center.y + (m_start.y - center.y) * scale;
    m_end.x = center.x + (m_end.x - center.x) * scale;
    m_end.y = center.y + (m_end.y - center.y) * scale;
}

std::string Rectangle::Serialize() {
    std::ostringstream oss;
    oss << "Rectangle " << m_start.x << " " << m_start.y << " " << m_end.x << " " << m_end.y;
    return oss.str();
}

void Rectangle::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type >> m_start.x >> m_start.y >> m_end.x >> m_end.y;
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
    // 简单的边界框测试
    float minX = min(min(m_points[0].x, m_points[1].x), m_points[2].x);
    float maxX = max(max(m_points[0].x, m_points[1].x), m_points[2].x);
    float minY = min(min(m_points[0].y, m_points[1].y), m_points[2].y);
    float maxY = max(max(m_points[0].y, m_points[1].y), m_points[2].y);
    return point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY;
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

void Triangle::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type >> m_points[0].x >> m_points[0].y
        >> m_points[1].x >> m_points[1].y
        >> m_points[2].x >> m_points[2].y;
}

// Diamond 实现
Diamond::Diamond(D2D1_POINT_2F center, D2D1_POINT_2F corner) :
    Shape(ShapeType::DIAMOND), m_center(center), m_corner(corner) {
}

void Diamond::Draw(ID2D1RenderTarget *pRenderTarget,
                   ID2D1SolidColorBrush *pNormalBrush,
                   ID2D1SolidColorBrush *pSelectedBrush,
                   ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;

    // 计算菱形的四个顶点
    float dx = m_corner.x - m_center.x;
    float dy = m_corner.y - m_center.y;

    D2D1_POINT_2F points[4] = {
        {m_center.x, m_center.y - dy}, // 上
        {m_center.x + dx, m_center.y}, // 右
        {m_center.x, m_center.y + dy}, // 下
        {m_center.x - dx, m_center.y}  // 左
    };

    // 创建菱形路径
    ID2D1PathGeometry *pPathGeometry = nullptr;
    ID2D1Factory *pFactory = nullptr;
    pRenderTarget->GetFactory(&pFactory);

    if (SUCCEEDED(pFactory->CreatePathGeometry(&pPathGeometry))) {
        ID2D1GeometrySink *pSink = nullptr;
        if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
            pSink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
            for (int i = 1; i < 4; i++) {
                pSink->AddLine(points[i]);
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

bool Diamond::HitTest(D2D1_POINT_2F point) {
    float dx = abs(point.x - m_center.x);
    float dy = abs(point.y - m_center.y);
    float cornerDx = abs(m_corner.x - m_center.x);
    float cornerDy = abs(m_corner.y - m_center.y);

    // 简单的菱形边界测试
    return (dx / cornerDx + dy / cornerDy) <= 1.0f;
}

void Diamond::Move(float dx, float dy) {
    m_center.x += dx;
    m_center.y += dy;
    m_corner.x += dx;
    m_corner.y += dy;
}

void Diamond::Rotate(float angle) {
    float s = sinf(angle);
    float c = cosf(angle);

    // 旋转角点相对于中心
    float relX = m_corner.x - m_center.x;
    float relY = m_corner.y - m_center.y;

    float newRelX = relX * c - relY * s;
    float newRelY = relX * s + relY * c;

    m_corner.x = m_center.x + newRelX;
    m_corner.y = m_center.y + newRelY;
}

void Diamond::Scale(float scale) {
    float relX = m_corner.x - m_center.x;
    float relY = m_corner.y - m_center.y;

    m_corner.x = m_center.x + relX * scale;
    m_corner.y = m_center.y + relY * scale;
}

std::string Diamond::Serialize() {
    std::ostringstream oss;
    oss << "Diamond " << m_center.x << " " << m_center.y << " "
        << m_corner.x << " " << m_corner.y;
    return oss.str();
}

void Diamond::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type >> m_center.x >> m_center.y >> m_corner.x >> m_corner.y;
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
    // 简单的边界框测试

    float minX = min(min(m_points[0].x, m_points[1].x), min(m_points[2].x, m_points[3].x));
    float maxX = max(max(m_points[0].x, m_points[1].x), max(m_points[2].x, m_points[3].x));
    float minY = min(min(m_points[0].y, m_points[1].y), min(m_points[2].y, m_points[3].y));
    float maxY = max(max(m_points[0].y, m_points[1].y), max(m_points[2].y, m_points[3].y));

    return point.x >= minX && point.x <= maxX && point.y >= minY && point.y <= maxY;
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

void Parallelogram::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type >> m_points[0].x >> m_points[0].y
        >> m_points[1].x >> m_points[1].y
        >> m_points[2].x >> m_points[2].y;

    // 重新计算第四个点
    m_points[3].x = m_points[0].x + (m_points[2].x - m_points[1].x);
    m_points[3].y = m_points[0].y + (m_points[2].y - m_points[1].y);
}

// Curve 实现
Curve::Curve(const std::vector<D2D1_POINT_2F> &points) :
    Shape(ShapeType::CURVE), m_points(points), m_isBezier(false) {
}

Curve::Curve(D2D1_POINT_2F start, D2D1_POINT_2F control, D2D1_POINT_2F end) :
    Shape(ShapeType::CURVE), m_isBezier(true) {
    m_points.push_back(start);
    m_points.push_back(control);
    m_points.push_back(end);
}

Curve::Curve(D2D1_POINT_2F start, D2D1_POINT_2F control1, D2D1_POINT_2F control2, D2D1_POINT_2F end) :
    Shape(ShapeType::CURVE), m_isBezier(true) {
    m_points.push_back(start);
    m_points.push_back(control1);
    m_points.push_back(control2);
    m_points.push_back(end);
}

void Curve::Draw(ID2D1RenderTarget *pRenderTarget,
                 ID2D1SolidColorBrush *pNormalBrush,
                 ID2D1SolidColorBrush *pSelectedBrush,
                 ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;
    if (m_points.size() < 2) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;

    if (m_isBezier && m_points.size() == 3) {
        // 二次贝塞尔曲线
        ID2D1PathGeometry *pPathGeometry = nullptr;
        ID2D1Factory *pFactory = nullptr;
        pRenderTarget->GetFactory(&pFactory);

        if (SUCCEEDED(pFactory->CreatePathGeometry(&pPathGeometry))) {
            ID2D1GeometrySink *pSink = nullptr;
            if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
                pSink->BeginFigure(m_points[0], D2D1_FIGURE_BEGIN_HOLLOW);
                pSink->AddQuadraticBezier(D2D1::QuadraticBezierSegment(m_points[1], m_points[2]));
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
    } else if (m_isBezier && m_points.size() == 4) {
        // 三次贝塞尔曲线
        ID2D1PathGeometry *pPathGeometry = nullptr;
        ID2D1Factory *pFactory = nullptr;
        pRenderTarget->GetFactory(&pFactory);

        if (SUCCEEDED(pFactory->CreatePathGeometry(&pPathGeometry))) {
            ID2D1GeometrySink *pSink = nullptr;
            if (SUCCEEDED(pPathGeometry->Open(&pSink))) {
                pSink->BeginFigure(m_points[0], D2D1_FIGURE_BEGIN_HOLLOW);
                pSink->AddBezier(D2D1::BezierSegment(m_points[1], m_points[2], m_points[3]));
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
    } else {
        // 自由曲线 - 使用多段线连接所有点
        for (size_t i = 1; i < m_points.size(); i++) {
            if (m_isSelected && pDashStrokeStyle)
                pRenderTarget->DrawLine(m_points[i - 1], m_points[i], currentBrush, 2.0f, pDashStrokeStyle);
            else
                pRenderTarget->DrawLine(m_points[i - 1], m_points[i], currentBrush, 2.0f);
        }
    }
}

bool Curve::HitTest(D2D1_POINT_2F point) {
    // 简单的点击测试：检查点是否靠近曲线的任何线段
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

void Curve::AddPoint(D2D1_POINT_2F point) {
    m_points.push_back(point);
}

void Curve::ClearPoints() {
    m_points.clear();
}

std::string Curve::Serialize() {
    std::ostringstream oss;
    oss << "Curve " << m_points.size() << " " << (m_isBezier ? 1 : 0);
    for (const auto &point : m_points) {
        oss << " " << point.x << " " << point.y;
    }
    return oss.str();
}

void Curve::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    size_t pointCount;
    int isBezier;

    iss >> type >> pointCount >> isBezier;
    m_isBezier = (isBezier != 0);

    m_points.clear();
    for (size_t i = 0; i < pointCount; i++) {
        D2D1_POINT_2F point;
        iss >> point.x >> point.y;
        m_points.push_back(point);
    }
}

// Polyline 实现
Polyline::Polyline(const std::vector<D2D1_POINT_2F> &points) :
    Shape(ShapeType::POLYLINE), m_points(points) {
}

void Polyline::Draw(ID2D1RenderTarget *pRenderTarget,
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

bool Polyline::HitTest(D2D1_POINT_2F point) {
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

void Polyline::Move(float dx, float dy) {
    for (auto &point : m_points) {
        point.x += dx;
        point.y += dy;
    }
}

void Polyline::Rotate(float angle) {
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

void Polyline::Scale(float scale) {
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

void Polyline::AddPoint(D2D1_POINT_2F point) {
    m_points.push_back(point);
}

std::string Polyline::Serialize() {
    std::ostringstream oss;
    oss << "Polyline " << m_points.size();
    for (const auto &point : m_points) {
        oss << " " << point.x << " " << point.y;
    }
    return oss.str();
}

void Polyline::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    size_t pointCount;

    iss >> type >> pointCount;

    m_points.clear();
    for (size_t i = 0; i < pointCount; i++) {
        D2D1_POINT_2F point;
        iss >> point.x >> point.y;
        m_points.push_back(point);
    }
}
