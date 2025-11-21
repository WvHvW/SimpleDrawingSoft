#pragma once
#include <d2d1.h>
#include <d2d1helper.h>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "CommonType.h" // 包含公共类型定义

class Shape {
public:
    Shape(ShapeType type) :
        m_type(type), m_isSelected(false) {
    }
    virtual ~Shape() = default;

    virtual void Draw(ID2D1RenderTarget *pRenderTarget,
                      ID2D1SolidColorBrush *pBrush,
                      ID2D1SolidColorBrush *pSelectedBrush,
                      ID2D1StrokeStyle *pDashStrokeStyle) = 0;
    virtual bool HitTest(D2D1_POINT_2F point) = 0;
    virtual void Move(float dx, float dy) = 0;
    virtual void Rotate(float angle) = 0;
    virtual void Scale(float scale) = 0;
    virtual D2D1_POINT_2F GetCenter() const = 0;
    virtual D2D1_RECT_F GetBounds() const = 0;

    ShapeType GetType() const {
        return m_type;
    }
    bool IsSelected() const {
        return m_isSelected;
    }
    void SetSelected(bool selected) {
        m_isSelected = selected;
    }

    // 提供默认实现返回边界框，子类可以重写
    virtual std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const {
        D2D1_RECT_F bounds = GetBounds();
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;

        D2D1_POINT_2F p1 = {bounds.left, bounds.top};
        D2D1_POINT_2F p2 = {bounds.right, bounds.top};
        D2D1_POINT_2F p3 = {bounds.right, bounds.bottom};
        D2D1_POINT_2F p4 = {bounds.left, bounds.bottom};

        segments.push_back({p1, p2});
        segments.push_back({p2, p3});
        segments.push_back({p3, p4});
        segments.push_back({p4, p1});

        return segments;
    }

    // 默认返回false，Circle类重写
    virtual bool HasCircleProperties() const {
        return false;
    }
    // 默认返回false，Circle类重写
    virtual bool GetCircleGeometry(D2D1_POINT_2F &center, float &radius) const {
        return false;
    }

    virtual std::string Serialize() = 0;
    static std::shared_ptr<Shape> Deserialize(const std::string &data);

protected:
    ShapeType m_type;
    bool m_isSelected;
};

// 直线类
class Line : public Shape {
public:
    Line(D2D1_POINT_2F start, D2D1_POINT_2F end);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;

    D2D1_POINT_2F GetStart() const {
        return m_start;
    }
    D2D1_POINT_2F GetEnd() const {
        return m_end;
    }

    D2D1_POINT_2F GetCenter() const override {
        return D2D1::Point2F((m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2);
    }

    D2D1_RECT_F GetBounds() const override {
        return D2D1::RectF(
            min(m_start.x, m_end.x),
            min(m_start.y, m_end.y),
            max(m_start.x, m_end.x),
            max(m_start.y, m_end.y));
    }

    // 重写离散线段函数
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;
        segments.push_back({m_start, m_end});
        return segments;
    }

private:
    D2D1_POINT_2F m_start, m_end;
};

// 中点画线法直线类
class MidpointLine : public Shape {
public:
    MidpointLine(D2D1_POINT_2F start, D2D1_POINT_2F end);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;

    D2D1_POINT_2F GetStart() const {
        return m_start;
    }
    D2D1_POINT_2F GetEnd() const {
        return m_end;
    }

    D2D1_POINT_2F GetCenter() const override {
        return D2D1::Point2F((m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2);
    }

    D2D1_RECT_F GetBounds() const override {
        return D2D1::RectF(
            min(m_start.x, m_end.x),
            min(m_start.y, m_end.y),
            max(m_start.x, m_end.x),
            max(m_start.y, m_end.y));
    }

    // 重写离散线段函数
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;
        segments.push_back({m_start, m_end});
        return segments;
    }

    // 获取中点画线法生成的像素点
    std::vector<D2D1_POINT_2F> GetMidpointPixels() const;

private:
    D2D1_POINT_2F m_start, m_end;
    std::vector<D2D1_POINT_2F> m_pixels; // 存储中点画线法生成的像素点
    void CalculateMidpointPixels(); // 计算中点画线法像素点
};

// Bresenham画线法直线类
class BresenhamLine : public Shape {
public:
    BresenhamLine(D2D1_POINT_2F start, D2D1_POINT_2F end);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;

    D2D1_POINT_2F GetStart() const {
        return m_start;
    }
    D2D1_POINT_2F GetEnd() const {
        return m_end;
    }

    D2D1_POINT_2F GetCenter() const override {
        return D2D1::Point2F((m_start.x + m_end.x) / 2, (m_start.y + m_end.y) / 2);
    }

    D2D1_RECT_F GetBounds() const override {
        return D2D1::RectF(
            min(m_start.x, m_end.x),
            min(m_start.y, m_end.y),
            max(m_start.x, m_end.x),
            max(m_start.y, m_end.y));
    }

    // 重写离散线段函数
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;
        segments.push_back({m_start, m_end});
        return segments;
    }

    // 获取Bresenham画线法生成的像素点
    std::vector<D2D1_POINT_2F> GetBresenhamPixels() const;

private:
    D2D1_POINT_2F m_start, m_end;
    std::vector<D2D1_POINT_2F> m_pixels; // 存储Bresenham画线法生成的像素点
    void CalculateBresenhamPixels(); // 计算Bresenham画线法像素点
};

// 中点画圆法圆形类
class MidpointCircle : public Shape {
public:
    MidpointCircle(D2D1_POINT_2F center, float radius);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override { /* 圆形旋转无意义 */ }
    void Scale(float scale) override;

    std::string Serialize() override;

    D2D1_POINT_2F GetCenter() const override {
        return m_center;
    }
    float GetRadius() const {
        return m_radius;
    }

    D2D1_RECT_F GetBounds() const override {
        return D2D1::RectF(
            m_center.x - m_radius,
            m_center.y - m_radius,
            m_center.x + m_radius,
            m_center.y + m_radius);
    }

    // 重写离散线段函数 - 将圆离散为多边形
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;
        const int segmentsCount = 32;
        std::vector<D2D1_POINT_2F> points;

        for (int i = 0; i < segmentsCount; ++i) {
            float angle = 2.0f * 3.14159265358979323846f * i / segmentsCount;
            D2D1_POINT_2F point;
            point.x = m_center.x + m_radius * cosf(angle);
            point.y = m_center.y + m_radius * sinf(angle);
            points.push_back(point);
        }

        for (int i = 0; i < segmentsCount; ++i) {
            segments.push_back({points[i], points[(i + 1) % segmentsCount]});
        }
        return segments;
    }

    // 重写圆形相关函数
    bool HasCircleProperties() const override {
        return true;
    }

    bool GetCircleGeometry(D2D1_POINT_2F &center, float &radius) const override {
        center = m_center;
        radius = m_radius;
        return true;
    }

    // 获取中点画圆法生成的像素点
    std::vector<D2D1_POINT_2F> GetMidpointPixels() const;

private:
    D2D1_POINT_2F m_center;
    float m_radius;
    std::vector<D2D1_POINT_2F> m_pixels; // 存储中点画圆法生成的像素点
    void CalculateMidpointPixels(); // 计算中点画圆法像素点
};

// Bresenham画圆法圆形类
class BresenhamCircle : public Shape {
public:
    BresenhamCircle(D2D1_POINT_2F center, float radius);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override { /* 圆形旋转无意义 */ }
    void Scale(float scale) override;

    std::string Serialize() override;

    D2D1_POINT_2F GetCenter() const override {
        return m_center;
    }
    float GetRadius() const {
        return m_radius;
    }

    D2D1_RECT_F GetBounds() const override {
        return D2D1::RectF(
            m_center.x - m_radius,
            m_center.y - m_radius,
            m_center.x + m_radius,
            m_center.y + m_radius);
    }

    // 重写离散线段函数 - 将圆离散为多边形
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;
        const int segmentsCount = 32;
        std::vector<D2D1_POINT_2F> points;

        for (int i = 0; i < segmentsCount; ++i) {
            float angle = 2.0f * 3.14159265358979323846f * i / segmentsCount;
            D2D1_POINT_2F point;
            point.x = m_center.x + m_radius * cosf(angle);
            point.y = m_center.y + m_radius * sinf(angle);
            points.push_back(point);
        }

        for (int i = 0; i < segmentsCount; ++i) {
            segments.push_back({points[i], points[(i + 1) % segmentsCount]});
        }
        return segments;
    }

    // 重写圆形相关函数
    bool HasCircleProperties() const override {
        return true;
    }

    bool GetCircleGeometry(D2D1_POINT_2F &center, float &radius) const override {
        center = m_center;
        radius = m_radius;
        return true;
    }

    // 获取Bresenham画圆法生成的像素点
    std::vector<D2D1_POINT_2F> GetBresenhamPixels() const;

private:
    D2D1_POINT_2F m_center;
    float m_radius;
    std::vector<D2D1_POINT_2F> m_pixels; // 存储Bresenham画圆法生成的像素点
    void CalculateBresenhamPixels(); // 计算Bresenham画圆法像素点
};

// 圆形类
class Circle : public Shape {
public:
    Circle(D2D1_POINT_2F center, float radius);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override { /* 圆形旋转无意义 */
    }
    void Scale(float scale) override;

    std::string Serialize() override;
    D2D1_POINT_2F GetCenter() const override {
        return m_center;
    }
    float GetRadius() const {
        return m_radius;
    }

    D2D1_RECT_F GetBounds() const override {
        return D2D1::RectF(
            m_center.x - m_radius,
            m_center.y - m_radius,
            m_center.x + m_radius,
            m_center.y + m_radius);
    }

    // 重写离散线段函数 - 将圆离散为多边形
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;

        const int segmentsCount = 32;
        std::vector<D2D1_POINT_2F> points;

        for (int i = 0; i < segmentsCount; ++i) {
            float angle = 2.0f * 3.14159265358979323846f * i / segmentsCount;
            D2D1_POINT_2F point;
            point.x = m_center.x + m_radius * cosf(angle);
            point.y = m_center.y + m_radius * sinf(angle);
            points.push_back(point);
        }

        for (int i = 0; i < segmentsCount; ++i) {
            segments.push_back({points[i], points[(i + 1) % segmentsCount]});
        }

        return segments;
    }

    // 重写圆形相关函数
    bool HasCircleProperties() const override {
        return true;
    }

    bool GetCircleGeometry(D2D1_POINT_2F &center, float &radius) const override {
        center = m_center;
        radius = m_radius;
        return true;
    }

private:
    D2D1_POINT_2F m_center;
    float m_radius;
};

// 矩形类
class Rect : public Shape {
public:
    Rect(D2D1_POINT_2F start, D2D1_POINT_2F end);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    D2D1_POINT_2F GetCenter() const override {
        float centerX = (m_points[0].x + m_points[1].x + m_points[2].x + m_points[3].x) / 4;
        float centerY = (m_points[0].y + m_points[1].y + m_points[2].y + m_points[3].y) / 4;
        return D2D1::Point2F(centerX, centerY);
    }

    D2D1_RECT_F GetBounds() const override {
        float minX = m_points[0].x;
        float minY = m_points[0].y;
        float maxX = m_points[0].x;
        float maxY = m_points[0].y;

        for (int i = 1; i < 4; i++) {
            if (m_points[i].x < minX) minX = m_points[i].x;
            if (m_points[i].y < minY) minY = m_points[i].y;
            if (m_points[i].x > maxX) maxX = m_points[i].x;
            if (m_points[i].y > maxY) maxY = m_points[i].y;
        }

        return D2D1::RectF(minX, minY, maxX, maxY);
    }

    std::string Serialize() override;

    // 重写离散线段函数
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;

        segments.push_back({m_points[0], m_points[1]});
        segments.push_back({m_points[1], m_points[2]});
        segments.push_back({m_points[2], m_points[3]});
        segments.push_back({m_points[3], m_points[0]});

        return segments;
    }

private:
    D2D1_POINT_2F m_points[4]; // 存储四个顶点
};

// 三角形类
class Triangle : public Shape {
public:
    Triangle(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;

    D2D1_POINT_2F GetCenter() const override {
        float centerX = (m_points[0].x + m_points[1].x + m_points[2].x) / 3;
        float centerY = (m_points[0].y + m_points[1].y + m_points[2].y) / 3;
        return D2D1::Point2F(centerX, centerY);
    }

    D2D1_RECT_F GetBounds() const override {
        float minX = m_points[0].x;
        float minY = m_points[0].y;
        float maxX = m_points[0].x;
        float maxY = m_points[0].y;

        for (int i = 1; i < 3; i++) {
            if (m_points[i].x < minX) minX = m_points[i].x;
            if (m_points[i].y < minY) minY = m_points[i].y;
            if (m_points[i].x > maxX) maxX = m_points[i].x;
            if (m_points[i].y > maxY) maxY = m_points[i].y;
        }

        return D2D1::RectF(minX, minY, maxX, maxY);
    }

    // 重写离散线段函数
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;

        segments.push_back({m_points[0], m_points[1]});
        segments.push_back({m_points[1], m_points[2]});
        segments.push_back({m_points[2], m_points[0]});

        return segments;
    }

private:
    D2D1_POINT_2F m_points[3];
};

// 菱形
class Diamond : public Shape {
public:
    Diamond(D2D1_POINT_2F center, float radiusX, float radiusY, float angle = 0.f);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pNormalBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    D2D1_POINT_2F GetCenter() const override {
        return m_center;
    }
    D2D1_RECT_F GetBounds() const override;

    std::string Serialize() override;

    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>>
    GetIntersectionSegments() const override;

private:
    D2D1_POINT_2F m_center;
    float m_radiusX;
    float m_radiusY;
    float m_angle;
};

// 平行四边形类
class Parallelogram : public Shape {
public:
    Parallelogram(D2D1_POINT_2F p1, D2D1_POINT_2F p2, D2D1_POINT_2F p3);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;

    D2D1_POINT_2F GetCenter() const override {
        float centerX = (m_points[0].x + m_points[1].x + m_points[2].x + m_points[3].x) / 4;
        float centerY = (m_points[0].y + m_points[1].y + m_points[2].y + m_points[3].y) / 4;
        return D2D1::Point2F(centerX, centerY);
    }

    D2D1_RECT_F GetBounds() const override {
        float minX = m_points[0].x;
        float minY = m_points[0].y;
        float maxX = m_points[0].x;
        float maxY = m_points[0].y;

        for (int i = 1; i < 4; i++) {
            if (m_points[i].x < minX) minX = m_points[i].x;
            if (m_points[i].y < minY) minY = m_points[i].y;
            if (m_points[i].x > maxX) maxX = m_points[i].x;
            if (m_points[i].y > maxY) maxY = m_points[i].y;
        }

        return D2D1::RectF(minX, minY, maxX, maxY);
    }

    // 重写离散线段函数
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;

        segments.push_back({m_points[0], m_points[1]});
        segments.push_back({m_points[1], m_points[2]});
        segments.push_back({m_points[2], m_points[3]});
        segments.push_back({m_points[3], m_points[0]});

        return segments;
    }

private:
    D2D1_POINT_2F m_points[4]; // 四个顶点
};

// 曲线类（使用三次贝塞尔曲线）
class Curve : public Shape {
public:
    Curve(D2D1_POINT_2F start, D2D1_POINT_2F control1, D2D1_POINT_2F control2, D2D1_POINT_2F end);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;

    // 获取点集
    const std::vector<D2D1_POINT_2F> &GetPoints() const {
        return m_points;
    }

    D2D1_POINT_2F GetCenter() const override {
        if (m_points.empty()) {
            return D2D1::Point2F(0, 0);
        }

        float sumX = 0, sumY = 0;
        for (const auto &point : m_points) {
            sumX += point.x;
            sumY += point.y;
        }

        return D2D1::Point2F(sumX / m_points.size(), sumY / m_points.size());
    }

    D2D1_RECT_F GetBounds() const override {
        if (m_points.empty()) {
            return D2D1::RectF(0, 0, 0, 0);
        }

        float minX = m_points[0].x;
        float minY = m_points[0].y;
        float maxX = m_points[0].x;
        float maxY = m_points[0].y;

        for (const auto &point : m_points) {
            minX = min(minX, point.x);
            minY = min(minY, point.y);
            maxX = max(maxX, point.x);
            maxY = max(maxY, point.y);
        }

       return D2D1::RectF(minX, minY, maxX, maxY);
    }

    // 离散线段函数
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override;

private:
    std::vector<D2D1_POINT_2F> m_points;
    static const int CURVE_FLATTEN_SEGS = 32; // 32 段 ≈ 0.4 px 误差

    // 计算贝塞尔曲线在参数t处的点
    D2D1_POINT_2F CalculateBezierPoint(float t) const;
};

// 多段线类
class Poly : public Shape {
public:
    Poly(const std::vector<D2D1_POINT_2F> &points = {});

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;

    void AddPoint(D2D1_POINT_2F point);
    const std::vector<D2D1_POINT_2F> &GetPoints() const {
        return m_points;
    }
    void SetPoints(const std::vector<D2D1_POINT_2F> &points) {
        m_points = points;
    }

    D2D1_POINT_2F GetCenter() const override {
        if (m_points.empty()) {
            return D2D1::Point2F(0, 0);
        }

        float sumX = 0, sumY = 0;
        for (const auto &point : m_points) {
            sumX += point.x;
            sumY += point.y;
        }

        return D2D1::Point2F(sumX / m_points.size(), sumY / m_points.size());
    }

    D2D1_RECT_F GetBounds() const override {
        if (m_points.empty()) {
            return D2D1::RectF(0, 0, 0, 0);
        }

        float minX = m_points[0].x;
        float minY = m_points[0].y;
        float maxX = m_points[0].x;
        float maxY = m_points[0].y;

        for (const auto &point : m_points) {
            minX = min(minX, point.x);
            minY = min(minY, point.y);
            maxX = max(maxX, point.x);
            maxY = max(maxY, point.y);
        }

        return D2D1::RectF(minX, minY, maxX, maxY);
    }

    // 重写离散线段函数 - 直接使用折线的各个线段
    std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> GetIntersectionSegments() const override {
        std::vector<std::pair<D2D1_POINT_2F, D2D1_POINT_2F>> segments;

        if (m_points.size() < 2) {
            return segments;
        }

        for (size_t i = 1; i < m_points.size(); ++i) {
            segments.push_back({m_points[i - 1], m_points[i]});
        }

        return segments;
    }

private:
    std::vector<D2D1_POINT_2F> m_points;
};
