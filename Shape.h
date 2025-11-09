#pragma once
#include <d2d1.h>
#include <d2d1helper.h>
#include <string>
#include <vector>
#include <memory>
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

    D2D1_POINT_2F GetCenter() const {
        return m_center;
    }
    float GetRadius() const {
        return m_radius;
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

private:
    std::vector<D2D1_POINT_2F> m_points;
};
