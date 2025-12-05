#pragma once
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <vector>
#include <memory>
#include <string>
#include "CommonType.h" // 包含公共类型定义

// 前置声明
class Shape;
class Line;
class Circle;
class Rect;

class GraphicsEngine {
public:
    GraphicsEngine();
    ~GraphicsEngine();

    HRESULT Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void Render();
    void Cleanup();

    // 绘图操作
    void BeginDraw() {
        if (m_pRenderTarget) m_pRenderTarget->BeginDraw();
    }
    HRESULT EndDraw() {
        return m_pRenderTarget ? m_pRenderTarget->EndDraw() : S_OK;
    }

    // 图元管理
    void AddShape(std::shared_ptr<Shape> shape);
    void DeleteSelectedShape();
    std::shared_ptr<Shape> SelectShape(D2D1_POINT_2F point);
    void ClearSelection();

    // 获取所有图形
    const std::vector<std::shared_ptr<Shape>> &GetShapes() const {
        return m_shapes;
    }

    // 变换操作
    void MoveSelectedShape(float dx, float dy);
    void RotateSelectedShape(float angle);
    void ScaleSelectedShape(float scale);
    void RotateAroundPoint(float angle, D2D1_POINT_2F center);

    // 绘制垂线
    std::shared_ptr<Line> CreatePerpendicularLine(std::shared_ptr<Line> line, D2D1_POINT_2F point);

    // 绘制切线
    std::vector<std::shared_ptr<Line>> CreateTangents(D2D1_POINT_2F point, std::shared_ptr<Circle> circle);

    ID2D1RenderTarget *GetRenderTarget() {
        return m_pRenderTarget;
    }
    void SetDrawingMode(DrawingMode mode) {
        m_currentMode = mode;
    }
    DrawingMode GetDrawingMode() const {
        return m_currentMode;
    }

    // 新增选择状态检查方法
    bool IsShapeSelected() const {
        return m_selectedShape != nullptr;
    }

    bool IsShapeSelected(const std::shared_ptr<Shape> &shape) const {
        return m_selectedShape == shape;
    }

    bool IsShapeSelected(int index) const {
        if (index >= 0 && index < m_shapes.size()) {
            return m_shapes[index] == m_selectedShape;
        }
        return false;
    }

    // 获取当前选中的形状
    std::shared_ptr<Shape> GetSelectedShape() const {
        return m_selectedShape;
    }

    // 获取选中形状的索引
    int GetSelectedShapeIndex() const {
        if (!m_selectedShape) return -1;

        for (int i = 0; i < m_shapes.size(); ++i) {
            if (m_shapes[i] == m_selectedShape) {
                return i;
            }
        }
        return -1;
    }

    // 求交功能
    bool selectShapeForIntersection(std::shared_ptr<Shape> shape);
    void calculateIntersection();
    void clearIntersection();
    const std::vector<D2D1_POINT_2F> &getIntersectionPoints() const;
    bool isIntersectionReady() const;

    // 获取选中的求交图元（用于高亮显示）
    std::shared_ptr<Shape> getFirstIntersectionShape() const;
    std::shared_ptr<Shape> getSecondIntersectionShape() const;

    void ClearAllShapes() {
        m_shapes.clear();
    }

    // 获取指定线型的笔划样式
    ID2D1StrokeStyle* GetStrokeStyle(LineStyle lineStyle);

private:
    HWND m_hwnd;
    ID2D1Factory *m_pD2DFactory;
    ID2D1HwndRenderTarget *m_pRenderTarget;
    ID2D1SolidColorBrush *m_pNormalBrush;
    ID2D1SolidColorBrush *m_pSelectedBrush;
    IDWriteFactory *m_pDWriteFactory;
    ID2D1StrokeStyle *m_pStrokeStyle;

    // 线型样式集合
    ID2D1StrokeStyle *m_pSolidStrokeStyle;
    ID2D1StrokeStyle *m_pDashStrokeStyle;
    ID2D1StrokeStyle *m_pDotStrokeStyle;
    ID2D1StrokeStyle *m_pDashDotStrokeStyle;
    ID2D1StrokeStyle *m_pDashDotDotStrokeStyle;

    std::vector<std::shared_ptr<Shape>> m_shapes;
    std::shared_ptr<Shape> m_selectedShape;
    DrawingMode m_currentMode;

    HRESULT CreateDeviceResources();
};