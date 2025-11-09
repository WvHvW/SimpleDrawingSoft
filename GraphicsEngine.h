#pragma once
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <vector>
#include <memory>
#include <string>
#include "CommonType.h"  // 包含公共类型定义

// 前置声明
class Shape;
class Line;
class Circle;
class Rectangle;

class GraphicsEngine {
public:
    GraphicsEngine();
    ~GraphicsEngine();

    HRESULT Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void Render();
    void Cleanup();

    // 绘图操作
    void BeginDraw() { if (m_pRenderTarget) m_pRenderTarget->BeginDraw(); }
    HRESULT EndDraw() { return m_pRenderTarget ? m_pRenderTarget->EndDraw() : S_OK; }

    // 图元管理
    void AddShape(std::shared_ptr<Shape> shape);
    void DeleteSelectedShape();
    std::shared_ptr<Shape> SelectShape(D2D1_POINT_2F point);
    void ClearSelection();

    // 变换操作
    void MoveSelectedShape(float dx, float dy);
    void RotateSelectedShape(float angle);
    void ScaleSelectedShape(float scale);

    // 图形计算
    std::vector<D2D1_POINT_2F> CalculateIntersections(std::shared_ptr<Shape> shape1, std::shared_ptr<Shape> shape2);
    std::shared_ptr<Line> CreatePerpendicularLine(std::shared_ptr<Line> line, D2D1_POINT_2F point);
    D2D1_POINT_2F GetCircleCenter(std::shared_ptr<Circle> circle);
    std::vector<std::shared_ptr<Line>> CreateTangents(D2D1_POINT_2F point, std::shared_ptr<Circle> circle);

    ID2D1RenderTarget* GetRenderTarget() { return m_pRenderTarget; }
    void SetDrawingMode(DrawingMode mode) { m_currentMode = mode; }
    DrawingMode GetDrawingMode() const { return m_currentMode; }

private:
    HWND m_hwnd;
    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush* m_pNormalBrush;
    ID2D1SolidColorBrush* m_pSelectedBrush;
    IDWriteFactory* m_pDWriteFactory;
    ID2D1StrokeStyle *m_pStrokeStyle;

    std::vector<std::shared_ptr<Shape>> m_shapes;
    std::shared_ptr<Shape> m_selectedShape;
    DrawingMode m_currentMode;

    HRESULT CreateDeviceResources();
    void DrawCoordinateInfo();
};