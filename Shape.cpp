#include "Shape.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <vector>

// 扫描线填充辅助函数
namespace {
    // 绘制像素点集合
    void DrawPixels(ID2D1RenderTarget* pRenderTarget, ID2D1SolidColorBrush* pBrush,
                    const std::vector<D2D1_POINT_2F>& pixels) {
        for (const auto& pixel : pixels) {
            D2D1_ELLIPSE ellipse = D2D1::Ellipse(pixel, 0.5f, 0.5f);
            pRenderTarget->FillEllipse(ellipse, pBrush);
        }
    }
    
    // 根据线型样式决定是否绘制某个位置的像素
    bool ShouldDrawPixelForLineStyle(float distance, LineStyle lineStyle) {
        const float dashLength = 8.0f;    // 虚线段长度
        const float dotLength = 2.0f;     // 点的长度
        const float gapLength = 4.0f;     // 间隔长度
        
        // 将距离转换为模式位置
        float position = fmod(distance, dashLength + gapLength);
        
        switch (lineStyle) {
            case LineStyle::SOLID:
                return true;
                
            case LineStyle::DASH:
                // 虚线：8像素线段 + 4像素间隔
                return position < dashLength;
                
            case LineStyle::DOT:
                // 点线：2像素点 + 4像素间隔
                position = fmod(distance, dotLength + gapLength);
                return position < dotLength;
                
            case LineStyle::DASH_DOT: {
                // 点划线：8像素线段 + 2像素间隔 + 2像素点 + 2像素间隔
                float pattern = dashLength + gapLength + dotLength + gapLength; // 16像素周期
                position = fmod(distance, pattern);
                if (position < dashLength) return true; // 线段部分
                if (position >= dashLength + gapLength && position < dashLength + gapLength + dotLength) return true; // 点部分
                return false;
            }
            
            case LineStyle::DASH_DOT_DOT: {
                // 双点划线：8像素线段 + 2像素间隔 + 2像素点 + 2像素间隔 + 2像素点 + 2像素间隔
                float pattern = dashLength + gapLength + dotLength + gapLength + dotLength + gapLength; // 20像素周期
                position = fmod(distance, pattern);
                if (position < dashLength) return true; // 线段部分
                if (position >= dashLength + gapLength && position < dashLength + gapLength + dotLength) return true; // 第一个点
                if (position >= dashLength + gapLength + dotLength + gapLength && 
                    position < dashLength + gapLength + dotLength + gapLength + dotLength) return true; // 第二个点
                return false;
            }
            
            default:
                return true;
        }
    }
}

std::shared_ptr<Shape> Shape::Deserialize(const std::string &data) {
    std::istringstream iss(data);
    std::string type;
    iss >> type;

    if (type == "Line") {
        D2D1_POINT_2F start, end;
        iss >> start.x >> start.y >> end.x >> end.y;
        return std::make_shared<Line>(start, end);
    } else if (type == "MidpointLine") {
        D2D1_POINT_2F start, end;
        iss >> start.x >> start.y >> end.x >> end.y;
        return std::make_shared<MidpointLine>(start, end);
    } else if (type == "BresenhamLine") {
        D2D1_POINT_2F start, end;
        iss >> start.x >> start.y >> end.x >> end.y;
        return std::make_shared<BresenhamLine>(start, end);
    } else if (type == "MidpointCircle") {
        D2D1_POINT_2F center;
        float radius;
        iss >> center.x >> center.y >> radius;
        return std::make_shared<MidpointCircle>(center, radius);
    } else if (type == "BresenhamCircle") {
        D2D1_POINT_2F center;
        float radius;
        iss >> center.x >> center.y >> radius;
        return std::make_shared<BresenhamCircle>(center, radius);
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
    } else if (type == "MultiBezier") {
        std::vector<D2D1_POINT_2F> points;
        size_t pointCount;
        iss >> pointCount;
        for (size_t i = 0; i < pointCount; ++i) {
            D2D1_POINT_2F point;
            iss >> point.x >> point.y;
            points.push_back(point);
        }
        auto multiBezier = std::make_shared<MultiBezier>();
        for (const auto& point : points) {
            multiBezier->AddControlPoint(point);
        }
        return multiBezier;
    } else if (type == "Polygon") {
        std::vector<D2D1_POINT_2F> points;
        size_t pointCount;
        iss >> pointCount;
        for (size_t i = 0; i < pointCount; ++i) {
            D2D1_POINT_2F point;
            iss >> point.x >> point.y;
            points.push_back(point);
        }
        return std::make_shared<Polygon>(points);
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
                ID2D1StrokeStyle *pStrokeStyle) {
    if (!pRenderTarget || !pBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pBrush;
    int lineWidth = GetLineWidthValue();
    
    // 使用Direct2D的DrawLine方法，支持线型和线宽
    pRenderTarget->DrawLine(m_start, m_end, currentBrush, (float)lineWidth, pStrokeStyle);
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

// MidpointLine 实现
MidpointLine::MidpointLine(D2D1_POINT_2F start, D2D1_POINT_2F end) :
    Shape(ShapeType::LINE), m_start(start), m_end(end) {
    CalculateMidpointPixels();
}

void MidpointLine::CalculateMidpointPixels() {
    m_pixels.clear();
    
    int x0 = static_cast<int>(m_start.x);
    int y0 = static_cast<int>(m_start.y);
    int x1 = static_cast<int>(m_end.x);
    int y1 = static_cast<int>(m_end.y);
    
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    
    if (dx > dy) {
        // 斜率 <= 1 的情况
        int d = 2 * dy - dx;
        int x = x0, y = y0;
        
        m_pixels.push_back(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)));
        
        while (x != x1) {
            x += sx;
            if (d > 0) {
                y += sy;
                d += 2 * (dy - dx);
            } else {
                d += 2 * dy;
            }
            m_pixels.push_back(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)));
        }
    } else {
        // 斜率 > 1 的情况
        int d = 2 * dx - dy;
        int x = x0, y = y0;
        
        m_pixels.push_back(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)));
        
        while (y != y1) {
            y += sy;
            if (d > 0) {
                x += sx;
                d += 2 * (dx - dy);
            } else {
                d += 2 * dx;
            }
            m_pixels.push_back(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)));
        }
    }
}

void MidpointLine::Draw(ID2D1RenderTarget *pRenderTarget,
                        ID2D1SolidColorBrush *pBrush,
                        ID2D1SolidColorBrush *pSelectedBrush,
                        ID2D1StrokeStyle *pStrokeStyle) {
    if (!pRenderTarget || !pBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pBrush;
    int lineWidth = GetLineWidthValue();
    
    // 对于中点画线法，我们让算法生成的每个像素点都根据线型模式来决定是否绘制
    
    // 计算线段的总长度和方向
    float lineLength = sqrtf((m_end.x - m_start.x) * (m_end.x - m_start.x) + 
                            (m_end.y - m_start.y) * (m_end.y - m_start.y));
    
    if (lineWidth == 1) {
        for (const auto& pixel : m_pixels) {
            // 计算当前像素在线段上的位置比例
            float pixelDistance = sqrtf((pixel.x - m_start.x) * (pixel.x - m_start.x) + 
                                      (pixel.y - m_start.y) * (pixel.y - m_start.y));
            
            // 根据线型样式决定是否绘制这个像素
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(pixelDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(pixel, 0.8f, 0.8f);
                pRenderTarget->FillEllipse(ellipse, currentBrush);
            }
        }
    } else {
        // 线宽大于1时，为每个像素点扩展
        std::vector<D2D1_POINT_2F> expandedPixels;
        float halfWidth = lineWidth / 2.0f;
        
        for (const auto& pixel : m_pixels) {
            // 计算当前像素在线段上的位置
            float pixelDistance = sqrtf((pixel.x - m_start.x) * (pixel.x - m_start.x) + 
                                      (pixel.y - m_start.y) * (pixel.y - m_start.y));
            
            // 根据线型样式决定是否绘制这个像素区域
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(pixelDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                for (int dy = -static_cast<int>(halfWidth); dy <= static_cast<int>(halfWidth); ++dy) {
                    for (int dx = -static_cast<int>(halfWidth); dx <= static_cast<int>(halfWidth); ++dx) {
                        if (dx * dx + dy * dy <= halfWidth * halfWidth) {
                            expandedPixels.push_back({pixel.x + dx, pixel.y + dy});
                        }
                    }
                }
            }
        }
        DrawPixels(pRenderTarget, currentBrush, expandedPixels);
    }
}

bool MidpointLine::HitTest(D2D1_POINT_2F point) {
    return PointNearSegment(point, m_start, m_end, 5.0f);
}

void MidpointLine::Move(float dx, float dy) {
    m_start.x += dx;
    m_start.y += dy;
    m_end.x += dx;
    m_end.y += dy;
    CalculateMidpointPixels(); // 重新计算像素点
}

void MidpointLine::Rotate(float angle) {
    // 围绕中心点旋转
    D2D1_POINT_2F center = {(m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2};
    float s = sinf(angle);
    float c = cosf(angle);

    // 旋转起点
    float newStartX = (m_start.x - center.x) * c - (m_start.y - center.y) * s;
    float newStartY = (m_start.x - center.x) * s + (m_start.y - center.y) * c;
    m_start.x = newStartX + center.x;
    m_start.y = newStartY + center.y;

    // 旋转终点
    float newEndX = (m_end.x - center.x) * c - (m_end.y - center.y) * s;
    float newEndY = (m_end.x - center.x) * s + (m_end.y - center.y) * c;
    m_end.x = newEndX + center.x;
    m_end.y = newEndY + center.y;
    
    CalculateMidpointPixels(); // 重新计算像素点
}

void MidpointLine::Scale(float scale) {
    D2D1_POINT_2F center = {(m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2};
    m_start.x = center.x + (m_start.x - center.x) * scale;
    m_start.y = center.y + (m_start.y - center.y) * scale;
    m_end.x = center.x + (m_end.x - center.x) * scale;
    m_end.y = center.y + (m_end.y - center.y) * scale;
    CalculateMidpointPixels(); // 重新计算像素点
}

std::string MidpointLine::Serialize() {
    std::ostringstream oss;
    oss << "MidpointLine " << m_start.x << " " << m_start.y << " " << m_end.x << " " << m_end.y;
    return oss.str();
}

std::vector<D2D1_POINT_2F> MidpointLine::GetMidpointPixels() const {
    return m_pixels;
}

// BresenhamLine 实现
BresenhamLine::BresenhamLine(D2D1_POINT_2F start, D2D1_POINT_2F end) :
    Shape(ShapeType::LINE), m_start(start), m_end(end) {
    CalculateBresenhamPixels();
}

void BresenhamLine::CalculateBresenhamPixels() {
    m_pixels.clear();
    
    int x0 = static_cast<int>(m_start.x);
    int y0 = static_cast<int>(m_start.y);
    int x1 = static_cast<int>(m_end.x);
    int y1 = static_cast<int>(m_end.y);
    
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    int x = x0, y = y0;
    
    while (true) {
        m_pixels.push_back(D2D1::Point2F(static_cast<float>(x), static_cast<float>(y)));
        
        if (x == x1 && y == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void BresenhamLine::Draw(ID2D1RenderTarget *pRenderTarget,
                         ID2D1SolidColorBrush *pBrush,
                         ID2D1SolidColorBrush *pSelectedBrush,
                         ID2D1StrokeStyle *pStrokeStyle) {
    if (!pRenderTarget || !pBrush || !pSelectedBrush) return;

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pBrush;
    int lineWidth = GetLineWidthValue();
    
    // 对于Bresenham画线法，我们让算法生成的每个像素点都根据线型模式来决定是否绘制
    
    // 计算线段的总长度和方向
    float lineLength = sqrtf((m_end.x - m_start.x) * (m_end.x - m_start.x) + 
                            (m_end.y - m_start.y) * (m_end.y - m_start.y));
    
    if (lineWidth == 1) {
        for (const auto& pixel : m_pixels) {
            // 计算当前像素在线段上的位置比例
            float pixelDistance = sqrtf((pixel.x - m_start.x) * (pixel.x - m_start.x) + 
                                      (pixel.y - m_start.y) * (pixel.y - m_start.y));
            
            // 根据线型样式决定是否绘制这个像素
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(pixelDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(pixel, 0.8f, 0.8f);
                pRenderTarget->FillEllipse(ellipse, currentBrush);
            }
        }
    } else {
        // 线宽大于1时，为每个像素点扩展
        std::vector<D2D1_POINT_2F> expandedPixels;
        float halfWidth = lineWidth / 2.0f;
        
        for (const auto& pixel : m_pixels) {
            // 计算当前像素在线段上的位置
            float pixelDistance = sqrtf((pixel.x - m_start.x) * (pixel.x - m_start.x) + 
                                      (pixel.y - m_start.y) * (pixel.y - m_start.y));
            
            // 根据线型样式决定是否绘制这个像素区域
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(pixelDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                for (int dy = -static_cast<int>(halfWidth); dy <= static_cast<int>(halfWidth); ++dy) {
                    for (int dx = -static_cast<int>(halfWidth); dx <= static_cast<int>(halfWidth); ++dx) {
                        if (dx * dx + dy * dy <= halfWidth * halfWidth) {
                            expandedPixels.push_back({pixel.x + dx, pixel.y + dy});
                        }
                    }
                }
            }
        }
        DrawPixels(pRenderTarget, currentBrush, expandedPixels);
    }
}

bool BresenhamLine::HitTest(D2D1_POINT_2F point) {
    return PointNearSegment(point, m_start, m_end, 5.0f);
}

void BresenhamLine::Move(float dx, float dy) {
    m_start.x += dx;
    m_start.y += dy;
    m_end.x += dx;
    m_end.y += dy;
    CalculateBresenhamPixels(); // 重新计算像素点
}

void BresenhamLine::Rotate(float angle) {
    // 围绕中心点旋转
    D2D1_POINT_2F center = {(m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2};
    float s = sinf(angle);
    float c = cosf(angle);

    // 旋转起点
    float newStartX = (m_start.x - center.x) * c - (m_start.y - center.y) * s;
    float newStartY = (m_start.x - center.x) * s + (m_start.y - center.y) * c;
    m_start.x = newStartX + center.x;
    m_start.y = newStartY + center.y;

    // 旋转终点
    float newEndX = (m_end.x - center.x) * c - (m_end.y - center.y) * s;
    float newEndY = (m_end.x - center.x) * s + (m_end.y - center.y) * c;
    m_end.x = newEndX + center.x;
    m_end.y = newEndY + center.y;
    
    CalculateBresenhamPixels(); // 重新计算像素点
}

void BresenhamLine::Scale(float scale) {
    D2D1_POINT_2F center = {(m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2};
    m_start.x = center.x + (m_start.x - center.x) * scale;
    m_start.y = center.y + (m_start.y - center.y) * scale;
    m_end.x = center.x + (m_end.x - center.x) * scale;
    m_end.y = center.y + (m_end.y - center.y) * scale;
    CalculateBresenhamPixels(); // 重新计算像素点
}

std::string BresenhamLine::Serialize() {
    std::ostringstream oss;
    oss << "BresenhamLine " << m_start.x << " " << m_start.y << " " << m_end.x << " " << m_end.y;
    return oss.str();
}

std::vector<D2D1_POINT_2F> BresenhamLine::GetBresenhamPixels() const {
    return m_pixels;
}

// MidpointCircle 实现
MidpointCircle::MidpointCircle(D2D1_POINT_2F center, float radius) :
    Shape(ShapeType::CIRCLE), m_center(center), m_radius(radius) {
    CalculateMidpointPixels();
}

void MidpointCircle::CalculateMidpointPixels() {
    m_pixels.clear();
    
    int r = static_cast<int>(m_radius);
    int x = 0;
    int y = r;
    int d = 1 - r;  // 初始判别式
    
    // 添加8个对称点的辅助函数
    auto addSymmetricPoints = [&](int px, int py) {
        m_pixels.push_back(D2D1::Point2F(m_center.x + px, m_center.y + py));
        m_pixels.push_back(D2D1::Point2F(m_center.x - px, m_center.y + py));
        m_pixels.push_back(D2D1::Point2F(m_center.x + px, m_center.y - py));
        m_pixels.push_back(D2D1::Point2F(m_center.x - px, m_center.y - py));
        m_pixels.push_back(D2D1::Point2F(m_center.x + py, m_center.y + px));
        m_pixels.push_back(D2D1::Point2F(m_center.x - py, m_center.y + px));
        m_pixels.push_back(D2D1::Point2F(m_center.x + py, m_center.y - px));
        m_pixels.push_back(D2D1::Point2F(m_center.x - py, m_center.y - px));
    };
    
    addSymmetricPoints(x, y);
    
    while (x < y) {
        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
        addSymmetricPoints(x, y);
    }
}

void MidpointCircle::Draw(ID2D1RenderTarget *pRenderTarget,
                          ID2D1SolidColorBrush *pBrush,
                          ID2D1SolidColorBrush *pSelectedBrush,
                          ID2D1StrokeStyle *pStrokeStyle) {
    if (!pRenderTarget || !pBrush || !pSelectedBrush) return;

    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(pRenderTarget);

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pBrush;
    int lineWidth = GetLineWidthValue();
    
    // 对于中点画圆法，我们让算法生成的每个像素点都根据线型模式来决定是否绘制
    
    if (lineWidth == 1) {
        for (const auto& pixel : m_pixels) {
            // 计算当前像素相对于圆心的角度
            float dx = pixel.x - m_center.x;
            float dy = pixel.y - m_center.y;
            float angle = atan2f(dy, dx);
            if (angle < 0) angle += 2.0f * 3.14159f; // 转换为0-2π范围
            
            // 将角度转换为圆周上的弧长距离
            float arcDistance = angle * m_radius;
            
            // 根据线型样式决定是否绘制这个像素
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(arcDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(pixel, 0.8f, 0.8f);
                pRenderTarget->FillEllipse(ellipse, currentBrush);
            }
        }
    } else {
        // 线宽大于1时，为每个像素点扩展
        std::vector<D2D1_POINT_2F> expandedPixels;
        float halfWidth = lineWidth / 2.0f;
        
        for (const auto& pixel : m_pixels) {
            // 计算当前像素相对于圆心的角度
            float dx = pixel.x - m_center.x;
            float dy = pixel.y - m_center.y;
            float angle = atan2f(dy, dx);
            if (angle < 0) angle += 2.0f * 3.14159f; // 转换为0-2π范围
            
            // 将角度转换为圆周上的弧长距离
            float arcDistance = angle * m_radius;
            
            // 根据线型样式决定是否绘制这个像素区域
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(arcDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                for (int dy = -static_cast<int>(halfWidth); dy <= static_cast<int>(halfWidth); ++dy) {
                    for (int dx = -static_cast<int>(halfWidth); dx <= static_cast<int>(halfWidth); ++dx) {
                        if (dx * dx + dy * dy <= halfWidth * halfWidth) {
                            expandedPixels.push_back({pixel.x + dx, pixel.y + dy});
                        }
                    }
                }
            }
        }
        DrawPixels(pRenderTarget, currentBrush, expandedPixels);
    }
}

bool MidpointCircle::HitTest(D2D1_POINT_2F point) {
    float dx = point.x - m_center.x;
    float dy = point.y - m_center.y;
    float distance = sqrtf(dx * dx + dy * dy);
    return fabsf(distance - m_radius) < 5.0f; // 5像素容差
}

void MidpointCircle::Move(float dx, float dy) {
    m_center.x += dx;
    m_center.y += dy;
    CalculateMidpointPixels(); // 重计算像素点
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
}

void MidpointCircle::Scale(float scale) {
    m_radius *= scale;
    CalculateMidpointPixels(); // 重新计算像素点
    TransformFillPixelsScale(scale, m_center);  // 缩放填充像素
}

std::string MidpointCircle::Serialize() {
    std::ostringstream oss;
    oss << "MidpointCircle " << m_center.x << " " << m_center.y << " " << m_radius;
    return oss.str();
}

std::vector<D2D1_POINT_2F> MidpointCircle::GetMidpointPixels() const {
    return m_pixels;
}

// BresenhamCircle 实现
BresenhamCircle::BresenhamCircle(D2D1_POINT_2F center, float radius) :
    Shape(ShapeType::CIRCLE), m_center(center), m_radius(radius) {
    CalculateBresenhamPixels();
}

void BresenhamCircle::CalculateBresenhamPixels() {
    m_pixels.clear();
    
    int r = static_cast<int>(m_radius);
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;  // Bresenham判别式
    
    // 添加8个对称点的辅助函数
    auto addSymmetricPoints = [&](int px, int py) {
        m_pixels.push_back(D2D1::Point2F(m_center.x + px, m_center.y + py));
        m_pixels.push_back(D2D1::Point2F(m_center.x - px, m_center.y + py));
        m_pixels.push_back(D2D1::Point2F(m_center.x + px, m_center.y - py));
        m_pixels.push_back(D2D1::Point2F(m_center.x - px, m_center.y - py));
        m_pixels.push_back(D2D1::Point2F(m_center.x + py, m_center.y + px));
        m_pixels.push_back(D2D1::Point2F(m_center.x - py, m_center.y + px));
        m_pixels.push_back(D2D1::Point2F(m_center.x + py, m_center.y - px));
        m_pixels.push_back(D2D1::Point2F(m_center.x - py, m_center.y - px));
    };
    
    addSymmetricPoints(x, y);
    
    while (x <= y) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        addSymmetricPoints(x, y);
    }
}

void BresenhamCircle::Draw(ID2D1RenderTarget *pRenderTarget,
                           ID2D1SolidColorBrush *pBrush,
                           ID2D1SolidColorBrush *pSelectedBrush,
                           ID2D1StrokeStyle *pStrokeStyle) {
    if (!pRenderTarget || !pBrush || !pSelectedBrush) return;

    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(pRenderTarget);

    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pBrush;
    int lineWidth = GetLineWidthValue();
    
    // 对于Bresenham画圆法，我们让算法生成的每个像素点都根据线型模式来决定是否绘制
    
    if (lineWidth == 1) {
        for (const auto& pixel : m_pixels) {
            // 计算当前像素相对于圆心的角度
            float dx = pixel.x - m_center.x;
            float dy = pixel.y - m_center.y;
            float angle = atan2f(dy, dx);
            if (angle < 0) angle += 2.0f * 3.14159f; // 转换为0-2π范围
            
            // 将角度转换为圆周上的弧长距离
            float arcDistance = angle * m_radius;
            
            // 根据线型样式决定是否绘制这个像素
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(arcDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(pixel, 0.8f, 0.8f);
                pRenderTarget->FillEllipse(ellipse, currentBrush);
            }
        }
    } else {
        // 线宽大于1时，为每个像素点扩展
        std::vector<D2D1_POINT_2F> expandedPixels;
        float halfWidth = lineWidth / 2.0f;
        
        for (const auto& pixel : m_pixels) {
            // 计算当前像素相对于圆心的角度
            float dx = pixel.x - m_center.x;
            float dy = pixel.y - m_center.y;
            float angle = atan2f(dy, dx);
            if (angle < 0) angle += 2.0f * 3.14159f; // 转换为0-2π范围
            
            // 将角度转换为圆周上的弧长距离
            float arcDistance = angle * m_radius;
            
            // 根据线型样式决定是否绘制这个像素区域
            bool shouldDraw = true;
            if (pStrokeStyle && GetLineStyle() != LineStyle::SOLID) {
                shouldDraw = ShouldDrawPixelForLineStyle(arcDistance, GetLineStyle());
            }
            
            if (shouldDraw) {
                for (int dy = -static_cast<int>(halfWidth); dy <= static_cast<int>(halfWidth); ++dy) {
                    for (int dx = -static_cast<int>(halfWidth); dx <= static_cast<int>(halfWidth); ++dx) {
                        if (dx * dx + dy * dy <= halfWidth * halfWidth) {
                            expandedPixels.push_back({pixel.x + dx, pixel.y + dy});
                        }
                    }
                }
            }
        }
        DrawPixels(pRenderTarget, currentBrush, expandedPixels);
    }
}

bool BresenhamCircle::HitTest(D2D1_POINT_2F point) {
    float dx = point.x - m_center.x;
    float dy = point.y - m_center.y;
    float distance = sqrtf(dx * dx + dy * dy);
    return fabsf(distance - m_radius) < 5.0f; // 5像素容差
}

void BresenhamCircle::Move(float dx, float dy) {
    m_center.x += dx;
    m_center.y += dy;
    CalculateBresenhamPixels(); // 重计算像素点
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
}

void BresenhamCircle::Scale(float scale) {
    m_radius *= scale;
    CalculateBresenhamPixels(); // 重新计算像素点
    TransformFillPixelsScale(scale, m_center);  // 缩放填充像素
}

std::string BresenhamCircle::Serialize() {
    std::ostringstream oss;
    oss << "BresenhamCircle " << m_center.x << " " << m_center.y << " " << m_radius;
    return oss.str();
}

std::vector<D2D1_POINT_2F> BresenhamCircle::GetBresenhamPixels() const {
    return m_pixels;
}

// Circle 实现
Circle::Circle(D2D1_POINT_2F center, float radius) :
    Shape(ShapeType::CIRCLE), m_center(center), m_radius(radius) {
}

void Circle::Draw(ID2D1RenderTarget *pRenderTarget,
                  ID2D1SolidColorBrush *pNormalBrush,
                  ID2D1SolidColorBrush *pSelectedBrush,
                  ID2D1StrokeStyle *pStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;

    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(pRenderTarget);
    
    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;
    int lineWidth = GetLineWidthValue();
    
    // 使用Direct2D的DrawEllipse方法，支持线型和线宽
    pRenderTarget->DrawEllipse(D2D1::Ellipse(m_center, m_radius, m_radius), currentBrush, (float)lineWidth, pStrokeStyle);
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
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
}

void Circle::Scale(float scale) {
    m_radius *= scale;
    TransformFillPixelsScale(scale, m_center);  // 缩放填充像素
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

    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(pRenderTarget);
    
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
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
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
    TransformFillPixelsRotate(angle, center);  // 旋转填充像素
}

void Rect::Scale(float scale) {
    D2D1_POINT_2F center = GetCenter();
    for (int i = 0; i < 4; i++) {
        m_points[i].x = center.x + (m_points[i].x - center.x) * scale;
        m_points[i].y = center.y + (m_points[i].y - center.y) * scale;
    }
    TransformFillPixelsScale(scale, center);  // 缩放填充像素
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

    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(pRenderTarget);

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
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
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
    TransformFillPixelsRotate(angle, center);  // 旋转填充像素
}

void Triangle::Scale(float scale) {
    D2D1_POINT_2F center = {
        (m_points[0].x + m_points[1].x + m_points[2].x) / 3,
        (m_points[0].y + m_points[1].y + m_points[2].y) / 3};

    for (int i = 0; i < 3; i++) {
        m_points[i].x = center.x + (m_points[i].x - center.x) * scale;
        m_points[i].y = center.y + (m_points[i].y - center.y) * scale;
    }
    TransformFillPixelsScale(scale, center);  // 缩放填充像素
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
    
    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(rt);
    
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
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
}

// Rotate
void Diamond::Rotate(float angle) {
    m_angle += angle;
    TransformFillPixelsRotate(angle, m_center);  // 旋转填充像素
}

// Scale
void Diamond::Scale(float scale) {
    m_radiusX *= scale;
    m_radiusY *= scale;
    TransformFillPixelsScale(scale, m_center);  // 缩放填充像素
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

    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(pRenderTarget);

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
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
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
    TransformFillPixelsRotate(angle, center);  // 旋转填充像素
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
    TransformFillPixelsScale(scale, center);  // 缩放填充像素
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

    // 绘制填充（使用基类的通用方法）
    DrawFillPixels(pRenderTarget);

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
    TransformFillPixelsMove(dx, dy);  // 移动填充像素
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
    TransformFillPixelsRotate(angle, center);  // 旋转填充像素
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
    TransformFillPixelsScale(scale, center);  // 缩放填充像素
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

// 多点Bezier曲线实现
MultiBezier::MultiBezier() : Shape(ShapeType::MULTI_BEZIER) {
}

// De Casteljau算法：递归计算任意阶Bezier曲线在参数t处的点
D2D1_POINT_2F MultiBezier::DeCasteljau(
    const std::vector<D2D1_POINT_2F>& controlPoints, float t) {
    if (controlPoints.empty()) {
        return D2D1::Point2F(0, 0);
    }
    if (controlPoints.size() == 1) {
        return controlPoints[0];
    }
    
    // 创建临时数组存储每一层的插值结果
    std::vector<D2D1_POINT_2F> tempPoints = controlPoints;
    int n = static_cast<int>(tempPoints.size());
    
    // 逐层计算线性插值
    for (int level = n - 1; level > 0; --level) {
        for (int i = 0; i < level; ++i) {
            tempPoints[i].x = (1.0f - t) * tempPoints[i].x + t * tempPoints[i + 1].x;
            tempPoints[i].y = (1.0f - t) * tempPoints[i].y + t * tempPoints[i + 1].y;
        }
    }
    
    return tempPoints[0];
}

void MultiBezier::AddControlPoint(D2D1_POINT_2F point) {
    m_controlPoints.push_back(point);
}

void MultiBezier::Draw(ID2D1RenderTarget *pRenderTarget,
                       ID2D1SolidColorBrush *pNormalBrush,
                       ID2D1SolidColorBrush *pSelectedBrush,
                       ID2D1StrokeStyle *pDashStrokeStyle) {
    if (!pRenderTarget || !pNormalBrush || !pSelectedBrush) return;
    
    // 只在编辑状态下绘制控制点和预览
    if (m_isEditing) {
        // 绘制已有的控制点（用小圆圈标记）
        ID2D1SolidColorBrush *controlBrush = nullptr;
        pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 0.7f), &controlBrush);
        if (controlBrush) {
            for (const auto& point : m_controlPoints) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(point, 4.0f, 4.0f);
                pRenderTarget->FillEllipse(ellipse, controlBrush);
            }
            controlBrush->Release();
        }
        
        // 绘制预览线段（从最后一个点到鼠标位置）
        if (m_hasPreview && !m_controlPoints.empty()) {
            ID2D1SolidColorBrush *previewBrush = nullptr;
            pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 0.5f), &previewBrush);
            if (previewBrush) {
                pRenderTarget->DrawLine(m_controlPoints.back(), m_previewPoint, previewBrush, 1.0f, pDashStrokeStyle);
                
                // 绘制预览点
                D2D1_ELLIPSE previewEllipse = D2D1::Ellipse(m_previewPoint, 3.0f, 3.0f);
                pRenderTarget->DrawEllipse(previewEllipse, previewBrush, 1.0f);
                
                previewBrush->Release();
            }
        }
    }
    
    // 如果控制点不足2个，只绘制控制点和预览，不绘制曲线
    if (m_controlPoints.size() < 2) return;
    
    ID2D1SolidColorBrush *currentBrush = m_isSelected ? pSelectedBrush : pNormalBrush;
    
    // 使用De Casteljau算法绘制整条Bezier曲线
    // 将曲线离散化为线段
    if (m_controlPoints.size() >= 2) {
        D2D1_POINT_2F prevPoint = DeCasteljau(m_controlPoints, 0.0f);
        
        for (int i = 1; i <= CURVE_SEGMENTS; ++i) {
            float t = static_cast<float>(i) / CURVE_SEGMENTS;
            D2D1_POINT_2F currentPoint = DeCasteljau(m_controlPoints, t);
            
            float strokeWidth = m_isSelected ? 2.0f : 2.0f;
            if (m_isSelected && pDashStrokeStyle) {
                pRenderTarget->DrawLine(prevPoint, currentPoint, currentBrush, strokeWidth, pDashStrokeStyle);
            } else {
                pRenderTarget->DrawLine(prevPoint, currentPoint, currentBrush, strokeWidth);
            }
            
            prevPoint = currentPoint;
        }
    }
    
    if (m_isSelected) {
        ID2D1SolidColorBrush *controlBrush = nullptr;
        pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &controlBrush);
        if (controlBrush) {
            for (const auto& point : m_controlPoints) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(point, 3.0f, 3.0f);
                pRenderTarget->FillEllipse(ellipse, controlBrush);
            }
            controlBrush->Release();
        }
    }
}

bool MultiBezier::HitTest(D2D1_POINT_2F point) {
    if (m_controlPoints.size() < 2) return false;
    
    // 使用De Casteljau算法离散化曲线并检测点击
    for (int i = 0; i < CURVE_SEGMENTS; ++i) {
        float t1 = static_cast<float>(i) / CURVE_SEGMENTS;
        float t2 = static_cast<float>(i + 1) / CURVE_SEGMENTS;
        
        D2D1_POINT_2F pt1 = DeCasteljau(m_controlPoints, t1);
        D2D1_POINT_2F pt2 = DeCasteljau(m_controlPoints, t2);
        
        float dx = pt2.x - pt1.x;
        float dy = pt2.y - pt1.y;
        float lenSq = dx * dx + dy * dy;
        
        if (lenSq == 0) continue;
        
        float t = ((point.x - pt1.x) * dx + (point.y - pt1.y) * dy) / lenSq;
        t = max(0.0f, min(1.0f, t));
        
        float nearX = pt1.x + t * dx;
        float nearY = pt1.y + t * dy;
        
        float distSq = (point.x - nearX) * (point.x - nearX) + 
                      (point.y - nearY) * (point.y - nearY);
        
        if (distSq < 25.0f) return true;
    }
    return false;
}

void MultiBezier::Move(float dx, float dy) {
    for (auto& point : m_controlPoints) {
        point.x += dx;
        point.y += dy;
    }
}

void MultiBezier::Rotate(float angle) {
    if (m_controlPoints.empty()) return;
    
    D2D1_POINT_2F center = GetCenter();
    float s = sinf(angle);
    float c = cosf(angle);
    
    for (auto& point : m_controlPoints) {
        float x = point.x - center.x;
        float y = point.y - center.y;
        
        float newX = x * c - y * s;
        float newY = x * s + y * c;
        
        point.x = newX + center.x;
        point.y = newY + center.y;
    }
}

void MultiBezier::Scale(float scale) {
    if (m_controlPoints.empty()) return;
    
    D2D1_POINT_2F center = GetCenter();
    
    for (auto& point : m_controlPoints) {
        point.x = center.x + (point.x - center.x) * scale;
        point.y = center.y + (point.y - center.y) * scale;
    }
}

D2D1_POINT_2F MultiBezier::GetCenter() const {
    if (m_controlPoints.empty()) {
        return D2D1::Point2F(0, 0);
    }
    
    float sumX = 0, sumY = 0;
    for (const auto& point : m_controlPoints) {
        sumX += point.x;
        sumY += point.y;
    }
    
    return D2D1::Point2F(sumX / m_controlPoints.size(), sumY / m_controlPoints.size());
}

D2D1_RECT_F MultiBezier::GetBounds() const {
    if (m_controlPoints.empty()) {
        return D2D1::RectF(0, 0, 0, 0);
    }
    
    float minX = m_controlPoints[0].x;
    float minY = m_controlPoints[0].y;
    float maxX = m_controlPoints[0].x;
    float maxY = m_controlPoints[0].y;
    
    for (const auto& point : m_controlPoints) {
        minX = min(minX, point.x);
        minY = min(minY, point.y);
        maxX = max(maxX, point.x);
        maxY = max(maxY, point.y);
    }
    
    return D2D1::RectF(minX, minY, maxX, maxY);
}

std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> MultiBezier::GetIntersectionSegments() const {
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;
    
    if (m_controlPoints.size() < 2) return segments;
    
    // 使用De Casteljau算法离散化整条曲线
    for (int i = 0; i < CURVE_SEGMENTS; ++i) {
        float t1 = static_cast<float>(i) / CURVE_SEGMENTS;
        float t2 = static_cast<float>(i + 1) / CURVE_SEGMENTS;
        
        D2D1_POINT_2F pt1 = DeCasteljau(m_controlPoints, t1);
        D2D1_POINT_2F pt2 = DeCasteljau(m_controlPoints, t2);
        
        segments.push_back({pt1, pt2});
    }
    
    return segments;
}

std::string MultiBezier::Serialize() {
    std::ostringstream oss;
    oss << "MultiBezier " << m_controlPoints.size();
    for (const auto& point : m_controlPoints) {
        oss << " " << point.x << " " << point.y;
    }
    return oss.str();
}

// ==================== Polygon 实现 ====================

Polygon::Polygon(const std::vector<D2D1_POINT_2F> &points) : Shape(ShapeType::POLYGON) {
    m_points = points;
}

void Polygon::Draw(ID2D1RenderTarget *pRenderTarget,
                   ID2D1SolidColorBrush *pBrush,
                   ID2D1SolidColorBrush *pSelectedBrush,
                   ID2D1StrokeStyle *pDashStrokeStyle) {
    if (m_points.size() < 2 || !pRenderTarget) return;

    // 先绘制填充
    DrawFillPixels(pRenderTarget);

    ID2D1SolidColorBrush *drawBrush = m_isSelected ? pSelectedBrush : pBrush;
    float strokeWidth = static_cast<float>(GetLineWidthValue());

    // 绘制多边形的边
    for (size_t i = 1; i < m_points.size(); ++i) {
        pRenderTarget->DrawLine(m_points[i - 1], m_points[i], drawBrush, strokeWidth);
    }

    // 如果有至少3个点，闭合多边形
    if (m_points.size() >= 3) {
        pRenderTarget->DrawLine(m_points.back(), m_points[0], drawBrush, strokeWidth);
    }
}

bool Polygon::HitTest(D2D1_POINT_2F point) {
    if (m_points.size() < 2) return false;

    const float threshold = 5.0f;

    // 检查是否点击在边上
    for (size_t i = 1; i < m_points.size(); ++i) {
        D2D1_POINT_2F p1 = m_points[i - 1];
        D2D1_POINT_2F p2 = m_points[i];

        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float length = sqrtf(dx * dx + dy * dy);

        if (length < 0.001f) continue;

        float t = ((point.x - p1.x) * dx + (point.y - p1.y) * dy) / (length * length);
        t = max(0.0f, min(1.0f, t));

        float closestX = p1.x + t * dx;
        float closestY = p1.y + t * dy;

        float distance = sqrtf(powf(point.x - closestX, 2) + powf(point.y - closestY, 2));
        if (distance <= threshold) {
            return true;
        }
    }

    // 检查闭合边
    if (m_points.size() >= 3) {
        D2D1_POINT_2F p1 = m_points.back();
        D2D1_POINT_2F p2 = m_points[0];

        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float length = sqrtf(dx * dx + dy * dy);

        if (length >= 0.001f) {
            float t = ((point.x - p1.x) * dx + (point.y - p1.y) * dy) / (length * length);
            t = max(0.0f, min(1.0f, t));

            float closestX = p1.x + t * dx;
            float closestY = p1.y + t * dy;

            float distance = sqrtf(powf(point.x - closestX, 2) + powf(point.y - closestY, 2));
            if (distance <= threshold) {
                return true;
            }
        }
    }

    return false;
}

void Polygon::Move(float dx, float dy) {
    for (auto &point : m_points) {
        point.x += dx;
        point.y += dy;
    }
    TransformFillPixelsMove(dx, dy);
}

void Polygon::Rotate(float angle) {
    if (m_points.empty()) return;

    D2D1_POINT_2F center = GetCenter();
    float s = sinf(angle);
    float c = cosf(angle);

    for (auto &point : m_points) {
        float x = point.x - center.x;
        float y = point.y - center.y;

        float newX = x * c - y * s;
        float newY = x * s + y * c;

        point.x = newX + center.x;
        point.y = newY + center.y;
    }
    TransformFillPixelsRotate(angle, center);
}

void Polygon::Scale(float scale) {
    if (m_points.empty()) return;

    D2D1_POINT_2F center = GetCenter();

    for (auto &point : m_points) {
        point.x = center.x + (point.x - center.x) * scale;
        point.y = center.y + (point.y - center.y) * scale;
    }
    TransformFillPixelsScale(scale, center);
}

void Polygon::AddPoint(D2D1_POINT_2F point) {
    m_points.push_back(point);
}

std::string Polygon::Serialize() {
    std::ostringstream oss;
    oss << "Polygon " << m_points.size();
    for (const auto &point : m_points) {
        oss << " " << point.x << " " << point.y;
    }
    return oss.str();
}

// 检查新点是否会导致自相交
bool Polygon::WouldCauseIntersection(D2D1_POINT_2F newPoint, bool checkClosingEdge) const {
    if (m_points.size() < 2) return false;

    D2D1_POINT_2F lastPoint = m_points.back();
    D2D1_POINT_2F firstPoint = m_points[0];
    
    char debugMsg[200];
    sprintf_s(debugMsg, "检查相交: 当前有%zu个点, 新边从(%.1f,%.1f)到(%.1f,%.1f), 检查闭合边=%d\n", 
              m_points.size(), lastPoint.x, lastPoint.y, newPoint.x, newPoint.y, checkClosingEdge);
    OutputDebugStringA(debugMsg);
    
    // 1. 检查新边 lastPoint -> newPoint 是否与已有的边相交
    for (size_t i = 0; i < m_points.size() - 1; ++i) {
        D2D1_POINT_2F edgeStart = m_points[i];
        D2D1_POINT_2F edgeEnd = m_points[i + 1];
        
        // 跳过最后一条边（与新边共享lastPoint端点）
        if (i == m_points.size() - 2) {
            sprintf_s(debugMsg, "  跳过最后一条边[%zu]: (%.1f,%.1f)到(%.1f,%.1f)\n", 
                      i, edgeStart.x, edgeStart.y, edgeEnd.x, edgeEnd.y);
            OutputDebugStringA(debugMsg);
            continue;
        }
        
        sprintf_s(debugMsg, "  检查新边与边[%zu]: (%.1f,%.1f)到(%.1f,%.1f)\n", 
                  i, edgeStart.x, edgeStart.y, edgeEnd.x, edgeEnd.y);
        OutputDebugStringA(debugMsg);
        
        if (SegmentsIntersect(lastPoint, newPoint, edgeStart, edgeEnd)) {
            OutputDebugStringA("  >>> 新边发现相交！\n");
            return true;
        }
    }

    // 2. 如果需要，检查闭合边 newPoint -> firstPoint 是否与已有边相交
    if (checkClosingEdge && m_points.size() >= 2) {
        sprintf_s(debugMsg, "  检查闭合边: (%.1f,%.1f)到(%.1f,%.1f)\n", 
                  newPoint.x, newPoint.y, firstPoint.x, firstPoint.y);
        OutputDebugStringA(debugMsg);
        
        // 闭合边需要与所有边检查，除了第一条边（共享firstPoint）和最后一条边（共享lastPoint）
        for (size_t i = 1; i < m_points.size() - 1; ++i) {
            D2D1_POINT_2F edgeStart = m_points[i];
            D2D1_POINT_2F edgeEnd = m_points[i + 1];
            
            sprintf_s(debugMsg, "    检查闭合边与边[%zu]: (%.1f,%.1f)到(%.1f,%.1f)\n", 
                      i, edgeStart.x, edgeStart.y, edgeEnd.x, edgeEnd.y);
            OutputDebugStringA(debugMsg);
            
            if (SegmentsIntersect(newPoint, firstPoint, edgeStart, edgeEnd)) {
                OutputDebugStringA("  >>> 闭合边发现相交！\n");
                return true;
            }
        }
    }

    OutputDebugStringA("  没有发现相交\n");
    return false;
}

// 辅助函数：检查两条线段是否相交
bool Polygon::SegmentsIntersect(D2D1_POINT_2F p1, D2D1_POINT_2F p2,
                                 D2D1_POINT_2F p3, D2D1_POINT_2F p4) {
    // 先检查端点重合的情况（不算相交）
    const float epsilon = 5.0f;  // 增大epsilon以更好地处理端点重合
    auto pointsEqual = [epsilon](D2D1_POINT_2F a, D2D1_POINT_2F b) -> bool {
        return fabsf(a.x - b.x) < epsilon && fabsf(a.y - b.y) < epsilon;
    };

    // 如果两条线段共享端点，不算相交
    if (pointsEqual(p1, p3) || pointsEqual(p1, p4) ||
        pointsEqual(p2, p3) || pointsEqual(p2, p4)) {
        return false;
    }

    // 使用叉积判断线段相交
    auto ccw = [](D2D1_POINT_2F A, D2D1_POINT_2F B, D2D1_POINT_2F C) -> float {
        return (C.y - A.y) * (B.x - A.x) - (B.y - A.y) * (C.x - A.x);
    };

    float d1 = ccw(p3, p4, p1);
    float d2 = ccw(p3, p4, p2);
    float d3 = ccw(p1, p2, p3);
    float d4 = ccw(p1, p2, p4);

    // 两条线段相交的条件：p1和p2在p3p4两侧，且p3和p4在p1p2两侧
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
        return true;
    }

    return false;
}
