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
        m_type(type), m_isSelected(false), m_transform(D2D1::Matrix3x2F::Identity()) {
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

    virtual std::string Serialize() = 0;
    virtual void Deserialize(const std::string &data) = 0;

protected:
    ShapeType m_type;
    bool m_isSelected;
    D2D1_MATRIX_3X2_F m_transform;
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
    void Deserialize(const std::string &data) override;

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

private:
    D2D1_POINT_2F m_start, m_end;
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
    void Deserialize(const std::string &data) override;

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

private:
    D2D1_POINT_2F m_center;
    float m_radius;
};

// 矩形类
class Rectangle : public Shape {
public:
    Rectangle(D2D1_POINT_2F start, D2D1_POINT_2F end);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    D2D1_POINT_2F GetCenter() const override {
        return D2D1::Point2F(
            (m_start.x + m_end.x) / 2,
            (m_start.y + m_end.y) / 2);
    }

    D2D1_RECT_F GetBounds() const override {
        return D2D1::RectF(
            min(m_start.x, m_end.x),
            min(m_start.y, m_end.y),
            max(m_start.x, m_end.x),
            max(m_start.y, m_end.y));
    }

    std::string Serialize() override;
    void Deserialize(const std::string &data) override;

private:
    D2D1_POINT_2F m_start, m_end;
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
    void Deserialize(const std::string &data) override;

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

private:
    D2D1_POINT_2F m_points[3];
};

// 菱形类
class Diamond : public Shape {
public:
    Diamond(D2D1_POINT_2F center, D2D1_POINT_2F corner);

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;
    void Deserialize(const std::string &data) override;
    D2D1_POINT_2F GetCenter() const override {
        return m_center;
    }

    D2D1_RECT_F GetBounds() const override {
        float width = std::abs(m_corner.x - m_center.x);
        float height = std::abs(m_corner.y - m_center.y);
        return D2D1::RectF(
            m_center.x - width,
            m_center.y - height,
            m_center.x + width,
            m_center.y + height);
    }

private:
    D2D1_POINT_2F m_center;
    D2D1_POINT_2F m_corner;
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
    void Deserialize(const std::string &data) override;

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

private:
    D2D1_POINT_2F m_points[4]; // 四个顶点
};

// 曲线类（使用贝塞尔曲线）
class Curve : public Shape {
public:
    Curve(const std::vector<D2D1_POINT_2F> &points);
    Curve(D2D1_POINT_2F start, D2D1_POINT_2F control, D2D1_POINT_2F end);                          // 二次贝塞尔曲线
    Curve(D2D1_POINT_2F start, D2D1_POINT_2F control1, D2D1_POINT_2F control2, D2D1_POINT_2F end); // 三次贝塞尔曲线

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;
    void Deserialize(const std::string &data) override;

    void AddPoint(D2D1_POINT_2F point);
    void ClearPoints();
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

private:
    std::vector<D2D1_POINT_2F> m_points;
    bool m_isBezier; // 是否为贝塞尔曲线
};

// 多段线类
class Polyline : public Shape {
public:
    Polyline(const std::vector<D2D1_POINT_2F> &points = {});

    void Draw(ID2D1RenderTarget *pRenderTarget,
              ID2D1SolidColorBrush *pBrush,
              ID2D1SolidColorBrush *pSelectedBrush,
              ID2D1StrokeStyle *pDashStrokeStyle) override;

    bool HitTest(D2D1_POINT_2F point) override;
    void Move(float dx, float dy) override;
    void Rotate(float angle) override;
    void Scale(float scale) override;

    std::string Serialize() override;
    void Deserialize(const std::string &data) override;

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

private:
    std::vector<D2D1_POINT_2F> m_points;
};
