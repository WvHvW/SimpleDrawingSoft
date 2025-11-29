#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include "CommonType.h"
#include "GraphicsEngine.h"
#include "Shape.h"
#include "Resource.h"
#include "FillAlgorithms.h"

class MainWindow {
public:
    MainWindow();
    ~MainWindow() = default;

    HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);
    int Run();

private:
    HWND m_hwnd;
    std::unique_ptr<GraphicsEngine> m_graphicsEngine;
    DrawingMode m_currentMode = DrawingMode::SELECT;
    LineWidth m_currentLineWidth = LineWidth::WIDTH_1PX;
    LineStyle m_currentLineStyle = LineStyle::SOLID;
    D2D1_POINT_2F m_startPoint, m_endPoint, m_midPoint;
    bool m_isDrawing = false;
    std::vector<D2D1_POINT_2F> m_polyPoints;
    int m_clickCount = 0;
    std::shared_ptr<Shape> m_tempShape;
    std::shared_ptr<Curve> m_currentCurve;
    bool m_isDrawingCurve = false;
    std::shared_ptr<Line> m_tempPolyLine;
    std::shared_ptr<MultiBezier> m_currentMultiBezier;
    bool m_isDrawingMultiBezier = false;
    IDWriteFactory *m_pDW = nullptr;
    IDWriteTextFormat *m_pTF = nullptr;
    IDWriteTextLayout *m_pModeLayout = nullptr; // 缓存模式文本布局

    // 菱形绘制参数
    D2D1_POINT_2F m_diamondCenter;
    float m_diamondRadiusX;
    float m_diamondRadiusY;
    float m_diamondAngle;

    // 变换状态
    TransformMode m_transformMode = TransformMode::NONE;
    bool m_isTransforming = false;
    D2D1_POINT_2F m_transformStartPoint;
    D2D1_POINT_2F m_transformReferencePoint; // 用于旋转和缩放的参考点

    // 贝塞尔曲线控制点
    D2D1_POINT_2F m_bezierControl1, m_bezierControl2;
    int m_bezierClickCount = 0;

    // 切线绘制状态
    bool m_isDrawingTangent = false;
    std::shared_ptr<Circle> m_selectedCircleForTangent;
    std::vector<std::shared_ptr<Line>> m_tempTangents;

    // 圆心显示状态
    bool m_showingCenter = false;
    D2D1_POINT_2F m_centerPoint;
    std::shared_ptr<Circle> m_selectedCircle;

    // 求交相关状态
    bool m_isIntersectionMode = false;

    // 多边形绘制状态
    std::vector<D2D1_POINT_2F> m_polygonPoints;
    bool m_isDrawingPolygon = false;
    std::shared_ptr<Polygon> m_currentPolygon;
    bool m_showInvalidPointFlash = false;
    D2D1_POINT_2F m_invalidPoint;
    DWORD m_flashStartTime = 0;

    // 交点显示
    void DrawIntersectionPoints(ID2D1RenderTarget *pRenderTarget);
    void DrawSelectedIntersectionShapes(ID2D1RenderTarget *pRenderTarget);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void OnLButtonDown(int x, int y);
    void OnRButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnMouseMove(int x, int y);
    void OnKeyDown(WPARAM wParam);
    void OnCommand(WPARAM wParam);
    void ResetDrawingState();
    void ResetTangentState();
    void ResetCenterState();
    float CalculateDistance(D2D1_POINT_2F startPoint, D2D1_POINT_2F endPoint);
    std::shared_ptr<Triangle> CreateEquilateralTriangle(D2D1_POINT_2F vertex1, D2D1_POINT_2F vertex2);

    // 变换方法
    void StartTransform(TransformMode mode, D2D1_POINT_2F point);
    void UpdateTransform(D2D1_POINT_2F point);
    void EndTransform();
    void CancelTransform();

    void SaveToFile();
    void LoadFromFile();
};

MainWindow::MainWindow() {
    m_graphicsEngine = std::make_unique<GraphicsEngine>();
}

// 窗口过程函数
LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow *pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
        pThis = reinterpret_cast<MainWindow *>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<MainWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        if (m_pModeLayout) {
            m_pModeLayout->Release();
            m_pModeLayout = nullptr;
        }
        return 0;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_RBUTTONDOWN:
        OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;

    case WM_KEYDOWN:
        OnKeyDown(wParam);
        return 0;

    case WM_COMMAND:
        OnCommand(wParam);
        return 0;

    case WM_SIZE:
        if (m_graphicsEngine) {
            m_graphicsEngine->Resize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            // 清除红色闪烁效果
            m_showInvalidPointFlash = false;
            KillTimer(m_hwnd, 1);
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1; // 防止闪烁
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::OnLButtonDown(int x, int y) {
    D2D1_POINT_2F currentPoint = D2D1::Point2F(static_cast<float>(x), static_cast<float>(y));

    switch (m_currentMode) {
    case DrawingMode::SELECT:
        // 首先尝试选择图形
        if (m_graphicsEngine->SelectShape(currentPoint)) {
            SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
            // 如果有图形被选中，根据当前变换模式开始变换
            if (m_transformMode != TransformMode::NONE) {
                StartTransform(m_transformMode, currentPoint);
            }
        } else {
            m_graphicsEngine->ClearSelection();
            CancelTransform();
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            m_graphicsEngine->clearIntersection();
        }
        break;

    case DrawingMode::LINE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<Line>(m_startPoint, currentPoint);
        } else {
            auto line = std::make_shared<Line>(m_startPoint, currentPoint);
            line->SetLineWidth(m_currentLineWidth);
            line->SetLineStyle(m_currentLineStyle);
            m_graphicsEngine->AddShape(line);
            ResetDrawingState();
        }
        break;

    case DrawingMode::MIDPOINT_LINE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<MidpointLine>(m_startPoint, currentPoint);
        } else {
            auto line = std::make_shared<MidpointLine>(m_startPoint, currentPoint);
            line->SetLineWidth(m_currentLineWidth);
            line->SetLineStyle(m_currentLineStyle);
            m_graphicsEngine->AddShape(line);
            ResetDrawingState();
        }
        break;

    case DrawingMode::BRESENHAM_LINE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<BresenhamLine>(m_startPoint, currentPoint);
        } else {
            auto line = std::make_shared<BresenhamLine>(m_startPoint, currentPoint);
            line->SetLineWidth(m_currentLineWidth);
            line->SetLineStyle(m_currentLineStyle);
            m_graphicsEngine->AddShape(line);
            ResetDrawingState();
        }
        break;

    case DrawingMode::MIDPOINT_CIRCLE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<MidpointCircle>(m_startPoint, 0);
        } else {
            float radius = CalculateDistance(m_startPoint, currentPoint);
            auto circle = std::make_shared<MidpointCircle>(m_startPoint, radius);
            circle->SetLineWidth(m_currentLineWidth);
            circle->SetLineStyle(m_currentLineStyle);
            m_graphicsEngine->AddShape(circle);
            ResetDrawingState();
        }
        break;

    case DrawingMode::BRESENHAM_CIRCLE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<BresenhamCircle>(m_startPoint, 0);
        } else {
            float radius = CalculateDistance(m_startPoint, currentPoint);
            auto circle = std::make_shared<BresenhamCircle>(m_startPoint, radius);
            circle->SetLineWidth(m_currentLineWidth);
            circle->SetLineStyle(m_currentLineStyle);
            m_graphicsEngine->AddShape(circle);
            ResetDrawingState();
        }
        break;

    case DrawingMode::CIRCLE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<Circle>(m_startPoint, 0);
        } else {
            float radius = CalculateDistance(m_startPoint, currentPoint);
            auto circle = std::make_shared<Circle>(m_startPoint, radius);
            circle->SetLineWidth(m_currentLineWidth);
            circle->SetLineStyle(m_currentLineStyle);
            m_graphicsEngine->AddShape(circle);
            ResetDrawingState();
        }
        break;

    case DrawingMode::RECTANGLE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<Rect>(m_startPoint, currentPoint);
        } else {
            m_graphicsEngine->AddShape(std::make_shared<Rect>(m_startPoint, currentPoint));
            ResetDrawingState();
        }
        break;

    case DrawingMode::TRIANGLE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            // 创建等边三角形预览
            m_tempShape = CreateEquilateralTriangle(m_startPoint, currentPoint);
        } else {
            // 创建等边三角形
            auto triangle = CreateEquilateralTriangle(m_startPoint, currentPoint);
            m_graphicsEngine->AddShape(triangle);
            ResetDrawingState();
        }
        break;

    case DrawingMode::DIAMOND:
        if (m_clickCount == 0) {
            m_diamondCenter = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            // 预览用当前鼠标作为半径/角度来源
            float dx = currentPoint.x - m_diamondCenter.x;
            float dy = currentPoint.y - m_diamondCenter.y;
            m_diamondRadiusX = hypotf(dx, dy);
            m_diamondRadiusY = m_diamondRadiusX * 0.6f;
            m_diamondAngle = atan2f(dy, dx);
            m_tempShape = std::make_shared<Diamond>(
                m_diamondCenter, m_diamondRadiusX, m_diamondRadiusY, m_diamondAngle);
        } else {
            // 第二次点击：固定参数并正式添加
            m_graphicsEngine->AddShape(std::make_shared<Diamond>(
                m_diamondCenter, m_diamondRadiusX, m_diamondRadiusY, m_diamondAngle));
            ResetDrawingState();
        }
        break;

    case DrawingMode::PARALLELOGRAM:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            // 平行四边形需要第三个点，先用起点作为临时点
            m_tempShape = std::make_shared<Parallelogram>(m_startPoint, currentPoint, currentPoint);
        } else if (m_clickCount == 1) {
            m_midPoint = currentPoint;
            m_clickCount = 2;
            // 更新预览
            m_tempShape = std::make_shared<Parallelogram>(m_startPoint, m_midPoint, currentPoint);
        } else {
            m_graphicsEngine->AddShape(std::make_shared<Parallelogram>(m_startPoint, m_midPoint, currentPoint));
            ResetDrawingState();
        }
        break;

    case DrawingMode::POLYLINE:
        m_polyPoints.push_back(currentPoint);
        break;

    case DrawingMode::CURVE:
        if (m_bezierClickCount == 0) {
            // 第一次点击：设置起点
            m_startPoint = currentPoint;
            m_bezierClickCount = 1;
            m_isDrawing = true;
            // 创建临时曲线预览，起点和终点相同，控制点也相同
            m_tempShape = std::make_shared<Curve>(
                m_startPoint,
                m_startPoint,
                m_startPoint,
                m_startPoint);
        } else if (m_bezierClickCount == 1) {
            // 第二次点击：设置第一个控制点
            m_bezierControl1 = currentPoint;
            m_bezierClickCount = 2;
            // 更新预览，起点到第一个控制点的直线
            m_tempShape = std::make_shared<Curve>(
                m_startPoint,
                m_bezierControl1,
                m_bezierControl1,
                m_bezierControl1);
        } else if (m_bezierClickCount == 2) {
            // 第三次点击：设置第二个控制点
            m_bezierControl2 = currentPoint;
            m_bezierClickCount = 3;
            // 更新预览，包含两个控制点的曲线
            m_tempShape = std::make_shared<Curve>(
                m_startPoint,
                m_bezierControl1,
                m_bezierControl2,
                m_bezierControl2);
        } else if (m_bezierClickCount == 3) {
            // 第四次点击：设置终点，完成曲线
            m_graphicsEngine->AddShape(std::make_shared<Curve>(
                m_startPoint,
                m_bezierControl1,
                m_bezierControl2,
                currentPoint));
            ResetDrawingState();
        }
        break;

    case DrawingMode::MULTI_BEZIER:
        if (!m_isDrawingMultiBezier) {
            // 开始绘制新的多点Bezier曲线
            m_currentMultiBezier = std::make_shared<MultiBezier>();
            m_currentMultiBezier->SetEditing(true); // 设置为编辑状态
            m_currentMultiBezier->AddControlPoint(currentPoint);
            m_isDrawingMultiBezier = true;
            OutputDebugStringA("开始绘制多点Bezier曲线，添加第一个控制点\n");
        } else {
            // 添加控制点
            m_currentMultiBezier->AddControlPoint(currentPoint);
            int pointCount = m_currentMultiBezier->GetControlPoints().size();

            char debugMsg[100];
            sprintf_s(debugMsg, "添加控制点 #%d\n", pointCount);
            OutputDebugStringA(debugMsg);
        }
        InvalidateRect(m_hwnd, nullptr, FALSE);
        break;
    case DrawingMode::SCANLINE_FILL:
    case DrawingMode::SEED_FILL:
        // 填充模式：查找点击位置的封闭图形并应用填充算法
        {
            bool foundShape = false;
            for (const auto &shape : m_graphicsEngine->GetShapes()) {
                ShapeType type = shape->GetType();
                // 只对封闭图形进行填充（包括多义线组成的封闭多边形）
                if (type == ShapeType::CIRCLE || type == ShapeType::RECTANGLE || type == ShapeType::TRIANGLE || type == ShapeType::DIAMOND || type == ShapeType::PARALLELOGRAM || type == ShapeType::POLYLINE) {
                    // 简单检查点是否在边界框内
                    D2D1_RECT_F bounds = shape->GetBounds();
                    if (currentPoint.x >= bounds.left && currentPoint.x <= bounds.right && currentPoint.y >= bounds.top && currentPoint.y <= bounds.bottom) {
                        // 应用填充算法
                        std::vector<D2D1_POINT_2F> fillPixels;
                        if (m_currentMode == DrawingMode::SCANLINE_FILL) {
                            fillPixels = FillAlgorithms::ScanlineFill(shape.get(), currentPoint);
                            OutputDebugStringA("应用栅栏填充算法\n");
                        } else {
                            fillPixels = FillAlgorithms::SeedFill(shape.get(), currentPoint);
                            OutputDebugStringA("应用种子填充算法\n");
                        }

                        if (!fillPixels.empty()) {
                            shape->SetFillPixels(fillPixels);
                            char debugMsg[100];
                            sprintf_s(debugMsg, "填充了 %zu 个像素\n", fillPixels.size());
                            OutputDebugStringA(debugMsg);
                            foundShape = true;
                            break;
                        }
                    }
                }
            }

            if (!foundShape) {
                OutputDebugStringA("未找到可填充的封闭图形\n");
            }
        }
        break;

    case DrawingMode::PERPENDICULAR:
        // 选择直线
        if (auto selectedShape = m_graphicsEngine->SelectShape(currentPoint)) {
            if (selectedShape->GetType() == ShapeType::LINE) {
                // 转换为 Line 指针
                auto line = std::dynamic_pointer_cast<Line>(selectedShape);
                if (line) {
                    // 创建垂直线
                    auto perpendicularLine = m_graphicsEngine->CreatePerpendicularLine(line, currentPoint);
                    if (perpendicularLine) {
                        m_graphicsEngine->AddShape(perpendicularLine);
                    }
                }
            }
        }
        break;

    case DrawingMode::TANGENT:
        if (!m_isDrawingTangent) {
            // 第一次点击：选择圆
            if (auto selectedShape = m_graphicsEngine->SelectShape(currentPoint)) {
                if (selectedShape && selectedShape->GetType() == ShapeType::CIRCLE) {
                    m_selectedCircleForTangent = std::dynamic_pointer_cast<Circle>(selectedShape);
                    if (m_selectedCircleForTangent) { // 添加空指针检查
                        m_isDrawingTangent = true;
                    }
                }
            }
        } else {
            // 第二次点击：确定切线方向并绘制切线
            if (m_selectedCircleForTangent) {
                auto tangents = m_graphicsEngine->CreateTangents(currentPoint, m_selectedCircleForTangent);
                for (auto &tangent : tangents) {
                    if (tangent) { // 检查切线是否有效
                        m_graphicsEngine->AddShape(tangent);
                    }
                }
            }
            ResetTangentState();
        }
        break;

    case DrawingMode::CENTER:
        // 选择圆并显示圆心
        if (auto selectedShape = m_graphicsEngine->SelectShape(currentPoint)) {
            if (selectedShape->GetType() == ShapeType::CIRCLE) {
                m_selectedCircle = std::dynamic_pointer_cast<Circle>(selectedShape);
                if (m_selectedCircle) {
                    m_centerPoint = m_selectedCircle->GetCenter();
                    m_showingCenter = true;
                }
            } else {
                // 如果选择的不是圆，清除圆心显示
                m_showingCenter = false;
                m_selectedCircle.reset();
            }
        } else {
            // 点击空白处，清除圆心显示
            m_showingCenter = false;
            m_selectedCircle.reset();
        }
        break;

    case DrawingMode::INTERSECT:
        // 选择图元用于求交
        if (auto selectedShape = m_graphicsEngine->SelectShape(currentPoint)) {
            if (m_graphicsEngine->selectShapeForIntersection(selectedShape)) {
                // 选择成功，如果已经有两个图元则自动计算交点
                if (m_graphicsEngine->isIntersectionReady()) {
                    m_graphicsEngine->calculateIntersection();
                }
            }
        }
        break;

    case DrawingMode::POLYGON:
        if (!m_isDrawingPolygon) {
            // 开始绘制新的多边形
            m_polygonPoints.clear();
            m_polygonPoints.push_back(currentPoint);
            m_isDrawingPolygon = true;
            m_currentPolygon = std::make_shared<Polygon>(m_polygonPoints);
            OutputDebugStringA("开始绘制多边形，添加第一个点\n");
        } else {
            // 用当前已确定的点创建临时多边形来检查相交
            char debugMsg[200];
            sprintf_s(debugMsg, "OnLButtonDown: m_polygonPoints有%zu个点, currentPoint=(%.1f,%.1f)\n", 
                      m_polygonPoints.size(), currentPoint.x, currentPoint.y);
            OutputDebugStringA(debugMsg);
            
            auto tempPolygon = std::make_shared<Polygon>(m_polygonPoints);
            
            // 检查是否会导致自相交
            if (tempPolygon->WouldCauseIntersection(currentPoint)) {
                // 显示红色闪烁提示
                m_showInvalidPointFlash = true;
                m_invalidPoint = currentPoint;
                m_flashStartTime = GetTickCount();
                OutputDebugStringA("检测到自相交，拒绝添加点\n");
                // 触发重绘以显示闪烁效果
                InvalidateRect(m_hwnd, nullptr, FALSE);
                // 设置定时器以清除闪烁效果
                SetTimer(m_hwnd, 1, 300, nullptr);
            } else {
                // 添加点
                m_polygonPoints.push_back(currentPoint);
                m_currentPolygon = std::make_shared<Polygon>(m_polygonPoints);
                char debugMsg2[100];
                sprintf_s(debugMsg2, "添加多边形顶点 #%zu\n", m_polygonPoints.size());
                OutputDebugStringA(debugMsg2);
            }
        }
        break;
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::OnMouseMove(int x, int y) {
    D2D1_POINT_2F currentPoint = D2D1::Point2F(static_cast<float>(x), static_cast<float>(y));

    // 在SELECT模式下检查鼠标是否悬停在图元上
    if (m_currentMode == DrawingMode::SELECT) {
        bool isOverShape = false;

        // 检查鼠标是否在任何一个图元上
        for (const auto &shape : m_graphicsEngine->GetShapes()) {
            if (shape->HitTest(currentPoint)) {
                isOverShape = true;
                break;
            }
        }

        // 根据是否悬停在图元上设置不同的光标
        if (isOverShape) {
            SetCursor(LoadCursor(nullptr, IDC_HAND)); // 手型光标
        } else {
            SetCursor(LoadCursor(nullptr, IDC_ARROW)); // 默认箭头光标
        }
    }

    // 变换操作
    if (m_isTransforming) {
        UpdateTransform(currentPoint);
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }

    if (m_isDrawing && m_tempShape) {
        switch (m_currentMode) {
        case DrawingMode::LINE:
            m_tempShape = std::make_shared<Line>(m_startPoint, currentPoint);
            break;

        case DrawingMode::MIDPOINT_LINE:
            m_tempShape = std::make_shared<MidpointLine>(m_startPoint, currentPoint);
            break;

        case DrawingMode::BRESENHAM_LINE:
            m_tempShape = std::make_shared<BresenhamLine>(m_startPoint, currentPoint);
            break;

        case DrawingMode::MIDPOINT_CIRCLE: {
            float radius = CalculateDistance(m_startPoint, currentPoint);
            m_tempShape = std::make_shared<MidpointCircle>(m_startPoint, radius);
            break;
        }

        case DrawingMode::BRESENHAM_CIRCLE: {
            float radius = CalculateDistance(m_startPoint, currentPoint);
            m_tempShape = std::make_shared<BresenhamCircle>(m_startPoint, radius);
            break;
        }

        case DrawingMode::CIRCLE: {
            float radius = CalculateDistance(m_startPoint, currentPoint);
            m_tempShape = std::make_shared<Circle>(m_startPoint, radius);
            break;
        }

        case DrawingMode::RECTANGLE:
            m_tempShape = std::make_shared<Rect>(m_startPoint, currentPoint);
            break;

        case DrawingMode::TRIANGLE:
            m_tempShape = CreateEquilateralTriangle(m_startPoint, currentPoint);
            break;

        case DrawingMode::DIAMOND:
            if (m_isDrawing && m_clickCount == 1) {
                // 用当前鼠标更新半径/角度
                float dx = currentPoint.x - m_diamondCenter.x;
                float dy = currentPoint.y - m_diamondCenter.y;
                m_diamondRadiusX = hypotf(dx, dy);
                m_diamondRadiusY = m_diamondRadiusX * 0.6f;
                m_diamondAngle = atan2f(dy, dx);
                m_tempShape = std::make_shared<Diamond>(
                    m_diamondCenter, m_diamondRadiusX, m_diamondRadiusY, m_diamondAngle);
            }
            break;

        case DrawingMode::PARALLELOGRAM:
            if (m_clickCount == 1) {
                // 第一次点击后移动：确定平行四边形的一条边
                m_tempShape = std::make_shared<Parallelogram>(m_startPoint, currentPoint, currentPoint);
            } else if (m_clickCount == 2) {
                // 第二次点击后移动：确定平行四边形的形状
                m_tempShape = std::make_shared<Parallelogram>(m_startPoint, m_midPoint, currentPoint);
            }
            break;
        }

        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 曲线模式：实时预览
    if (m_currentMode == DrawingMode::CURVE && m_isDrawing && m_tempShape) {
        if (m_bezierClickCount == 1) {
            // 第一次点击后移动：预览从起点到当前点的直线
            m_tempShape = std::make_shared<Curve>(
                m_startPoint,
                currentPoint,
                currentPoint,
                currentPoint);
        } else if (m_bezierClickCount == 2) {
            // 第二次点击后移动：预览包含第一个控制点的曲线
            m_tempShape = std::make_shared<Curve>(
                m_startPoint,
                m_bezierControl1,
                currentPoint,
                currentPoint);
        } else if (m_bezierClickCount == 3) {
            // 第三次点击后移动：预览包含两个控制点的完整曲线
            m_tempShape = std::make_shared<Curve>(
                m_startPoint,
                m_bezierControl1,
                m_bezierControl2,
                currentPoint);
        }
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 多段线模式：实时预览当前线段
    if (m_currentMode == DrawingMode::POLYLINE && !m_polyPoints.empty()) {
        // 创建从最后一个点到当前鼠标位置的预览线段
        m_tempPolyLine = std::make_shared<Line>(m_polyPoints.back(), currentPoint);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 多点Bezier曲线模式：实时预览鼠标移动
    if (m_currentMode == DrawingMode::MULTI_BEZIER && m_isDrawingMultiBezier && m_currentMultiBezier) {
        // 更新预览点为当前鼠标位置
        m_currentMultiBezier->SetPreviewPoint(currentPoint);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 多边形模式：实时预览当前边并检测闭合边相交
    if (m_currentMode == DrawingMode::POLYGON && m_isDrawingPolygon && !m_polygonPoints.empty()) {
        // 更新当前多边形预览（包含鼠标位置）
        std::vector<D2D1_POINT_2F> previewPoints = m_polygonPoints;
        previewPoints.push_back(currentPoint);
        m_currentPolygon = std::make_shared<Polygon>(previewPoints);
        
        // 如果已有至少3个点，检测闭合边是否会相交
        if (m_polygonPoints.size() >= 3) {
            auto tempPolygon = std::make_shared<Polygon>(m_polygonPoints);
            // 检查从当前鼠标位置到第一个点的闭合边是否会相交
            if (tempPolygon->WouldCauseIntersection(currentPoint, true)) {
                // 闭合边会相交，显示红色提示（但不设置定时器，因为鼠标一直在移动）
                m_showInvalidPointFlash = true;
                m_invalidPoint = currentPoint;
            } else {
                // 闭合边不相交，清除红色提示
                m_showInvalidPointFlash = false;
            }
        }
        
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // 切线模式预览
    if (m_currentMode == DrawingMode::TANGENT && m_isDrawingTangent && m_selectedCircleForTangent) {
        m_tempTangents = m_graphicsEngine->CreateTangents(currentPoint, m_selectedCircleForTangent);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // CENTER模式光标反馈
    if (m_currentMode == DrawingMode::CENTER) {
        bool isOverCircle = false;

        // 检查鼠标是否在圆上
        for (const auto &shape : m_graphicsEngine->GetShapes()) {
            if (shape->GetType() == ShapeType::CIRCLE && shape->HitTest(currentPoint)) {
                isOverCircle = true;
                break;
            }
        }

        if (isOverCircle) {
            SetCursor(LoadCursor(nullptr, IDC_CROSS)); // 十字光标
        } else {
            SetCursor(LoadCursor(nullptr, IDC_ARROW)); // 默认箭头光标
        }
    }
}

float MainWindow::CalculateDistance(D2D1_POINT_2F p1, D2D1_POINT_2F p2) {
    return sqrtf(powf(p2.x - p1.x, 2) + powf(p2.y - p1.y, 2));
}

std::shared_ptr<Triangle> MainWindow::CreateEquilateralTriangle(D2D1_POINT_2F p1, D2D1_POINT_2F p2) {
    // 计算等边三角形的第三个点
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;

    // 计算边长
    float sideLength = CalculateDistance(p1, p2);

    // 计算等边三角形的高
    float height = sideLength * sqrtf(3.0f) / 2.0f;

    // 计算第三个点（在垂直平分线上）
    D2D1_POINT_2F p3;
    p3.x = p1.x + dx / 2 - height * (p2.y - p1.y) / sideLength;
    p3.y = p1.y + dy / 2 + height * (p2.x - p1.x) / sideLength;

    return std::make_shared<Triangle>(p1, p2, p3);
}

void MainWindow::ResetDrawingState() {
    m_clickCount = 0;
    m_isDrawing = false;
    m_tempShape.reset();
    m_polyPoints.clear();
    m_isDrawingCurve = false;
    m_diamondRadiusX = 50.0f;
    m_diamondRadiusY = 30.0f;
    m_diamondAngle = 0.0f;
    m_currentCurve.reset();
    m_tempPolyLine.reset();
    ResetTangentState();
    ResetCenterState();
    m_bezierClickCount = 0;
    m_bezierControl1 = D2D1::Point2F(0, 0);
    m_bezierControl2 = D2D1::Point2F(0, 0);
    // 重置多边形状态
    m_polygonPoints.clear();
    m_isDrawingPolygon = false;
    m_currentPolygon.reset();
    m_showInvalidPointFlash = false;
}

void MainWindow::ResetTangentState() {
    m_isDrawingTangent = false;
    m_selectedCircleForTangent.reset();
    m_tempTangents.clear();
}

void MainWindow::ResetCenterState() {
    m_showingCenter = false;
    m_selectedCircle.reset();
}

void MainWindow::OnRButtonDown(int x, int y) {
    // 右键完成多点Bezier曲线绘制
    if (m_isDrawingMultiBezier && m_currentMultiBezier) {
        m_currentMultiBezier->ClearPreviewPoint(); // 清除预览点
        m_currentMultiBezier->SetEditing(false);   // 清除编辑状态
        if (m_currentMultiBezier->GetControlPoints().size() >= 2) {
            m_graphicsEngine->AddShape(m_currentMultiBezier);
            OutputDebugStringA("多点Bezier曲线绘制完成（使用De Casteljau算法）\n");
        } else {
            OutputDebugStringA("控制点不足2个，无法形成曲线\n");
        }
        m_currentMultiBezier.reset();
        m_isDrawingMultiBezier = false;
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }

    // 右键结束曲线绘制
    if (m_currentMode == DrawingMode::CURVE && m_isDrawingCurve && m_currentCurve) {
        ResetDrawingState();
    } else if (m_currentMode == DrawingMode::POLYLINE) {
        // 结束多段线绘制
        if (m_polyPoints.size() >= 2) {
            auto polyline = std::make_shared<Poly>(m_polyPoints);
            m_graphicsEngine->AddShape(polyline);
        }
        m_polyPoints.clear();
        m_tempPolyLine.reset();
    } else if (m_currentMode == DrawingMode::POLYGON) {
        // 右键完成多边形绘制
        if (m_polygonPoints.size() >= 3) {
            // 检查闭合边是否会导致自相交
            auto tempPolygon = std::make_shared<Polygon>(m_polygonPoints);
            D2D1_POINT_2F lastPoint = m_polygonPoints.back();
            
            // 使用checkClosingEdge=true来检查从lastPoint到firstPoint的闭合边
            if (tempPolygon->WouldCauseIntersection(lastPoint, true)) {
                // 闭合边会导致自相交，显示红色闪烁并拒绝完成
                m_showInvalidPointFlash = true;
                m_invalidPoint = lastPoint;
                m_flashStartTime = GetTickCount();
                OutputDebugStringA("闭合边会导致自相交，无法完成多边形\n");
                InvalidateRect(m_hwnd, nullptr, FALSE);
                SetTimer(m_hwnd, 1, 300, nullptr);
            } else {
                // 创建最终的多边形
                auto finalPolygon = std::make_shared<Polygon>(m_polygonPoints);
                finalPolygon->SetLineWidth(m_currentLineWidth);
                finalPolygon->SetLineStyle(m_currentLineStyle);
                m_graphicsEngine->AddShape(finalPolygon);
                OutputDebugStringA("多边形绘制完成\n");
                
                // 清理状态
                m_polygonPoints.clear();
                m_currentPolygon.reset();
                m_isDrawingPolygon = false;
                m_showInvalidPointFlash = false;
            }
        } else {
            OutputDebugStringA("顶点不足3个，无法形成多边形\n");
            // 清理状态
            m_polygonPoints.clear();
            m_currentPolygon.reset();
            m_isDrawingPolygon = false;
            m_showInvalidPointFlash = false;
        }
    } else {
        // 其他模式的右键取消
        ResetDrawingState();
        m_graphicsEngine->ClearSelection();
        CancelTransform();
        m_graphicsEngine->clearIntersection();
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}
void MainWindow::OnKeyDown(WPARAM wParam) {
    // 测试线宽功能的键盘快捷键
    if (wParam >= '1' && wParam <= '5') {
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    LineWidth newWidth;
                    switch (wParam) {
                    case '1': newWidth = LineWidth::WIDTH_1PX; break;
                    case '2': newWidth = LineWidth::WIDTH_2PX; break;
                    case '3': newWidth = LineWidth::WIDTH_4PX; break;
                    case '4': newWidth = LineWidth::WIDTH_8PX; break;
                    case '5': newWidth = LineWidth::WIDTH_16PX; break;
                    }
                    selectedShape->SetLineWidth(newWidth);
                    m_currentLineWidth = newWidth;

                    char debugMsg[100];
                    sprintf_s(debugMsg, "Line width set to %dpx via keyboard\n", static_cast<int>(newWidth));
                    OutputDebugStringA(debugMsg);

                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return;
                }
            }
        }
    }

    // 按C键清除所有选择，确保图形显示为黑色
    if (wParam == 'C') {
        m_graphicsEngine->ClearSelection();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        OutputDebugStringA("All selections cleared\n");
        return;
    }
    const float MOVE_STEP = 5.0f;   // 移动步长
    const float ROTATE_STEP = 0.1f; // 旋转步长
    const float SCALE_STEP = 0.1f;  // 缩放步长

    switch (wParam) {
    case VK_ESCAPE:
        // ESC键：取消选择和变换
        m_graphicsEngine->ClearSelection();
        CancelTransform();
        break;

    case VK_DELETE:
    case 'D':
        // Delete键：删除选中图形
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->DeleteSelectedShape();
            CancelTransform();
        }
        break;

    // 精细控制（在有选中图形时）
    case VK_LEFT:
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->MoveSelectedShape(-MOVE_STEP, 0);
        }
        break;
    case VK_RIGHT:
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->MoveSelectedShape(MOVE_STEP, 0);
        }
        break;
    case VK_UP:
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->MoveSelectedShape(0, -MOVE_STEP);
        }
        break;
    case VK_DOWN:
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->MoveSelectedShape(0, MOVE_STEP);
        }
        break;

    case 'Q':
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->RotateSelectedShape(-ROTATE_STEP);
        }
        break;
    case 'E':
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->RotateSelectedShape(ROTATE_STEP);
        }
        break;

    case 'Z':
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->ScaleSelectedShape(1.0f - SCALE_STEP);
        }
        break;
    case 'X':
        if (m_graphicsEngine->IsShapeSelected()) {
            m_graphicsEngine->ScaleSelectedShape(1.0f + SCALE_STEP);
        }
        break;
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::StartTransform(TransformMode mode, D2D1_POINT_2F point) {
    if (!m_graphicsEngine->IsShapeSelected()) return;
    m_transformMode = mode;
    m_transformStartPoint = point;
    m_isTransforming = true;

    // 对于旋转和缩放，需要计算参考点（图形中心）
    if (mode == TransformMode::ROTATE || mode == TransformMode::SCALE) {
        auto selectedShape = m_graphicsEngine->GetSelectedShape();
        if (selectedShape)
            m_transformReferencePoint = selectedShape->GetCenter();
        else
            m_transformReferencePoint = point;
    }
}

void MainWindow::UpdateTransform(D2D1_POINT_2F point) {
    if (!m_graphicsEngine->IsShapeSelected() || !m_isTransforming) return;

    float dx = point.x - m_transformStartPoint.x;
    float dy = point.y - m_transformStartPoint.y;

    switch (m_transformMode) {
    case TransformMode::MOVE:
        // 移动：直接应用偏移量
        m_graphicsEngine->MoveSelectedShape(dx, dy);
        break;

    case TransformMode::ROTATE:
        // 旋转：基于参考点计算角度
        {
            // 计算从参考点到起始点的向量
            float startVecX = m_transformStartPoint.x - m_transformReferencePoint.x;
            float startVecY = m_transformStartPoint.y - m_transformReferencePoint.y;

            // 计算从参考点到当前点的向量
            float currentVecX = point.x - m_transformReferencePoint.x;
            float currentVecY = point.y - m_transformReferencePoint.y;

            // 计算角度（弧度）
            float startAngle = atan2f(startVecY, startVecX);
            float currentAngle = atan2f(currentVecY, currentVecX);
            float angle = currentAngle - startAngle;

            m_graphicsEngine->RotateSelectedShape(angle);
        }
        break;

    case TransformMode::SCALE:
        // 缩放：基于参考点计算缩放比例
        {
            // 计算起始距离
            float startDistX = m_transformStartPoint.x - m_transformReferencePoint.x;
            float startDistY = m_transformStartPoint.y - m_transformReferencePoint.y;
            float startDistance = sqrtf(startDistX * startDistX + startDistY * startDistY);

            // 计算当前距离
            float currentDistX = point.x - m_transformReferencePoint.x;
            float currentDistY = point.y - m_transformReferencePoint.y;
            float currentDistance = sqrtf(currentDistX * currentDistX + currentDistY * currentDistY);

            // 计算缩放比例
            if (startDistance > 0.1f) { // 避免除以零
                float scale = currentDistance / startDistance;
                m_graphicsEngine->ScaleSelectedShape(scale);
            }
        }
        break;
    }

    // 更新起始点，实现连续变换
    m_transformStartPoint = point;
}

void MainWindow::EndTransform() {
    m_isTransforming = false;
    // 注意：不重置 m_transformMode，保持当前变换模式
}

void MainWindow::CancelTransform() {
    m_isTransforming = false;
    m_transformMode = TransformMode::NONE;
}

void MainWindow::DrawIntersectionPoints(ID2D1RenderTarget *rt) {
    auto points = m_graphicsEngine->getIntersectionPoints();
    if (points.empty()) return;

    // 惰性初始化 DirectWrite
    if (!m_pDW) {
        DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown **>(&m_pDW));
        if (m_pDW)
            m_pDW->CreateTextFormat(
                L"Consolas", nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                12.0f, L"en-us", &m_pTF);
    }
    if (!m_pTF) return;

    ID2D1SolidColorBrush *br = nullptr;
    ID2D1SolidColorBrush *bg = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &br);
    rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.9f), &bg);
    if (!br || !bg) {
        if (br) br->Release();
        if (bg) bg->Release();
        return;
    }

    for (const auto &p : points) {
        // 十字
        float sz = 8.0f;
        rt->DrawLine({p.x - sz, p.y}, {p.x + sz, p.y}, br, 2.0f);
        rt->DrawLine({p.x, p.y - sz}, {p.x, p.y + sz}, br, 2.0f);
        rt->FillEllipse(D2D1::Ellipse(p, 4.0f, 4.0f), br);

        // 坐标文本
        WCHAR txt[64];
        swprintf_s(txt, L"%.1f, %.1f", p.x, p.y);
        IDWriteTextLayout *layout = nullptr;
        m_pDW->CreateTextLayout(txt, wcslen(txt), m_pTF, 200, 30, &layout);
        if (layout) {
            DWRITE_TEXT_METRICS m;
            layout->GetMetrics(&m);
            D2D1_RECT_F rc = {p.x + 10, p.y - 20, p.x + 10 + m.width + 4, p.y - 20 + m.height + 4};
            rt->FillRectangle(&rc, bg);
            rt->DrawRectangle(&rc, br, 1.0f);
            rt->DrawTextLayout({p.x + 12, p.y - 18}, layout, br);
            layout->Release();
        }
    }
    br->Release();
    bg->Release();
}
void MainWindow::DrawSelectedIntersectionShapes(ID2D1RenderTarget *pRenderTarget) {
    if (m_currentMode != DrawingMode::INTERSECT) return;

    auto shape1 = m_graphicsEngine->getFirstIntersectionShape();
    auto shape2 = m_graphicsEngine->getSecondIntersectionShape();

    if (!shape1 && !shape2) return;

    ID2D1SolidColorBrush *highlightBrush = nullptr;
    HRESULT hr = pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Yellow), &highlightBrush);

    if (SUCCEEDED(hr) && highlightBrush) {
        // 创建虚线笔划样式用于高亮
        ID2D1StrokeStyle *dashStrokeStyle = nullptr;
        ID2D1Factory *factory = nullptr;
        pRenderTarget->GetFactory(&factory);

        if (factory) {
            float dashes[] = {5.0f, 5.0f};
            factory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_ROUND,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_CUSTOM,
                    0.0f),
                dashes,
                ARRAYSIZE(dashes),
                &dashStrokeStyle);
        }

        // 绘制选中的图元
        if (shape1) {
            shape1->Draw(pRenderTarget, highlightBrush, highlightBrush, dashStrokeStyle);
        }
        if (shape2) {
            shape2->Draw(pRenderTarget, highlightBrush, highlightBrush, dashStrokeStyle);
        }

        if (dashStrokeStyle) dashStrokeStyle->Release();
        if (factory) factory->Release();
        highlightBrush->Release();
    }
}

void MainWindow::OnPaint() {
    if (m_graphicsEngine) {
        m_graphicsEngine->BeginDraw();
        m_graphicsEngine->Render();

        // 绘制临时形状（预览）
        if (m_isDrawing && m_tempShape) {
            ID2D1SolidColorBrush *tempBrush = nullptr;
            HRESULT hr = m_graphicsEngine->GetRenderTarget()->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::LightBlue), &tempBrush);

            if (SUCCEEDED(hr) && tempBrush) {
                m_tempShape->Draw(m_graphicsEngine->GetRenderTarget(), tempBrush, tempBrush, nullptr);
                tempBrush->Release();
            }
        }

        // 绘制当前正在绘制的曲线
        if (m_isDrawingCurve && m_currentCurve) {
            ID2D1SolidColorBrush *tempBrush = nullptr;
            HRESULT hr = m_graphicsEngine->GetRenderTarget()->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Blue), &tempBrush);

            if (SUCCEEDED(hr) && tempBrush) {
                m_currentCurve->Draw(m_graphicsEngine->GetRenderTarget(), tempBrush, tempBrush, nullptr);
                tempBrush->Release();
            }
        }

        // 绘制多段线预览（已确定的线段）
        if (m_currentMode == DrawingMode::POLYLINE && m_polyPoints.size() >= 2) {
            ID2D1SolidColorBrush *polyBrush = nullptr;
            HRESULT hr = m_graphicsEngine->GetRenderTarget()->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Green), &polyBrush);

            if (SUCCEEDED(hr) && polyBrush) {
                // 绘制已确定的多段线线段
                for (size_t i = 1; i < m_polyPoints.size(); i++) {
                    m_graphicsEngine->GetRenderTarget()->DrawLine(
                        m_polyPoints[i - 1], m_polyPoints[i], polyBrush, 2.0f);
                }
                polyBrush->Release();
            }
        }

        // 绘制多段线当前线段预览
        if (m_currentMode == DrawingMode::POLYLINE && m_tempPolyLine) {
            ID2D1SolidColorBrush *tempBrush = nullptr;
            HRESULT hr = m_graphicsEngine->GetRenderTarget()->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::LightBlue), &tempBrush);

            if (SUCCEEDED(hr) && tempBrush) {
                m_tempPolyLine->Draw(m_graphicsEngine->GetRenderTarget(), tempBrush, tempBrush, nullptr);
                tempBrush->Release();
            }
        }

        // 绘制正在编辑的多点Bezier曲线
        if (m_isDrawingMultiBezier && m_currentMultiBezier) {
            ID2D1SolidColorBrush *tempBrush = nullptr;
            HRESULT hr = m_graphicsEngine->GetRenderTarget()->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Blue), &tempBrush);

            if (SUCCEEDED(hr) && tempBrush) {
                m_currentMultiBezier->Draw(m_graphicsEngine->GetRenderTarget(), tempBrush, tempBrush, nullptr);
                tempBrush->Release();
            }
        }

        // 绘制正在绘制的多边形预览
        if (m_currentMode == DrawingMode::POLYGON && m_isDrawingPolygon && m_currentPolygon) {
            ID2D1SolidColorBrush *tempBrush = nullptr;
            HRESULT hr = m_graphicsEngine->GetRenderTarget()->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Green), &tempBrush);

            if (SUCCEEDED(hr) && tempBrush) {
                m_currentPolygon->Draw(m_graphicsEngine->GetRenderTarget(), tempBrush, tempBrush, nullptr);
                tempBrush->Release();
            }
        }

        // 绘制红色闪烁的非法点提示
        if (m_showInvalidPointFlash) {
            ID2D1SolidColorBrush *redBrush = nullptr;
            HRESULT hr = m_graphicsEngine->GetRenderTarget()->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Red), &redBrush);

            if (SUCCEEDED(hr) && redBrush) {
                D2D1_ELLIPSE ellipse = D2D1::Ellipse(m_invalidPoint, 8.0f, 8.0f);
                m_graphicsEngine->GetRenderTarget()->FillEllipse(ellipse, redBrush);
                redBrush->Release();
            }
        }

        // 绘制切线预览和切点坐标
        if (m_currentMode == DrawingMode::TANGENT && m_isDrawingTangent && m_selectedCircleForTangent && !m_tempTangents.empty()) {
            ID2D1RenderTarget *pRenderTarget = m_graphicsEngine->GetRenderTarget();

            ID2D1SolidColorBrush *tempBrush = nullptr;
            HRESULT hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Orange), &tempBrush);

            if (SUCCEEDED(hr) && tempBrush) {
                // 确保 DirectWrite 工厂和文本格式已初始化
                if (!m_pDW) {
                    DWriteCreateFactory(
                        DWRITE_FACTORY_TYPE_SHARED,
                        __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown **>(&m_pDW));
                }
                if (!m_pTF && m_pDW) {
                    m_pDW->CreateTextFormat(
                        L"Consolas", nullptr,
                        DWRITE_FONT_WEIGHT_NORMAL,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        12.0f, L"en-us", &m_pTF);
                }

                for (auto &tangent : m_tempTangents) {
                    if (!tangent) continue;

                    // 绘制切线
                    tangent->Draw(pRenderTarget, tempBrush, tempBrush, nullptr);

                    // 获取切点
                    D2D1_POINT_2F endPoint = tangent->GetEnd();

                    // 绘制切点标记
                    D2D1_ELLIPSE tangentPoint = D2D1::Ellipse(endPoint, 4.0f, 4.0f);
                    pRenderTarget->FillEllipse(tangentPoint, tempBrush);

                    // 绘制坐标文本
                    if (m_pTF) {
                        WCHAR coordText[100];
                        swprintf_s(coordText, L"切点: (%.1f, %.1f)", endPoint.x, endPoint.y);

                        // 计算文本位置
                        float textX = endPoint.x + 10.0f;
                        float textY = endPoint.y - 15.0f;

                        // 绘制文本背景
                        ID2D1SolidColorBrush *bgBrush = nullptr;
                        if (SUCCEEDED(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.8f), &bgBrush))) {
                            D2D1_RECT_F textRect = {textX, textY, textX + 120.0f, textY + 20.0f};
                            pRenderTarget->FillRectangle(textRect, bgBrush);
                            pRenderTarget->DrawRectangle(textRect, tempBrush, 1.0f);
                            bgBrush->Release();
                        }

                        // 绘制文本
                        pRenderTarget->DrawText(
                            coordText,
                            wcslen(coordText),
                            m_pTF,
                            D2D1::RectF(textX + 2, textY + 2, textX + 120.0f, textY + 20.0f),
                            tempBrush);
                    }
                }
                tempBrush->Release();
            }
        }

        // 绘制圆心标记和坐标
        if (m_currentMode == DrawingMode::CENTER && m_showingCenter && m_selectedCircle) {
            ID2D1RenderTarget *pRenderTarget = m_graphicsEngine->GetRenderTarget();

            // 创建红色画笔用于绘制圆心标记
            ID2D1SolidColorBrush *centerBrush = nullptr;
            HRESULT hr = pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Red), &centerBrush);

            if (SUCCEEDED(hr) && centerBrush) {
                // 绘制圆心十字标记
                float crossSize = 8.0f;
                pRenderTarget->DrawLine(
                    D2D1::Point2F(m_centerPoint.x - crossSize, m_centerPoint.y),
                    D2D1::Point2F(m_centerPoint.x + crossSize, m_centerPoint.y),
                    centerBrush, 2.0f);

                pRenderTarget->DrawLine(
                    D2D1::Point2F(m_centerPoint.x, m_centerPoint.y - crossSize),
                    D2D1::Point2F(m_centerPoint.x, m_centerPoint.y + crossSize),
                    centerBrush, 2.0f);

                // 绘制圆心点
                D2D1_ELLIPSE centerDot = D2D1::Ellipse(m_centerPoint, 3.0f, 3.0f);
                pRenderTarget->FillEllipse(centerDot, centerBrush);

                // 创建文本格式和画笔
                IDWriteTextFormat *pTextFormat = nullptr;
                IDWriteFactory *pDWriteFactory = nullptr;
                DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown **>(&pDWriteFactory));

                if (pDWriteFactory) {
                    pDWriteFactory->CreateTextFormat(
                        L"Arial",
                        NULL,
                        DWRITE_FONT_WEIGHT_NORMAL,
                        DWRITE_FONT_STYLE_NORMAL,
                        DWRITE_FONT_STRETCH_NORMAL,
                        12.0f,
                        L"en-us",
                        &pTextFormat);
                }

                if (pTextFormat) {
                    // 格式化坐标文本
                    WCHAR coordText[100];
                    swprintf_s(coordText, L"圆心: (%.1f, %.1f)", m_centerPoint.x, m_centerPoint.y);

                    // 绘制坐标文本背景
                    ID2D1SolidColorBrush *textBgBrush = nullptr;
                    pRenderTarget->CreateSolidColorBrush(
                        D2D1::ColorF(D2D1::ColorF::White, 0.7f), &textBgBrush);

                    if (textBgBrush) {
                        D2D1_RECT_F textRect = D2D1::RectF(
                            m_centerPoint.x + 10.0f,
                            m_centerPoint.y - 20.0f,
                            m_centerPoint.x + 150.0f,
                            m_centerPoint.y);

                        pRenderTarget->FillRectangle(textRect, textBgBrush);
                        textBgBrush->Release();
                    }

                    // 绘制坐标文本
                    D2D1_RECT_F textRect = D2D1::RectF(
                        m_centerPoint.x + 10.0f,
                        m_centerPoint.y - 20.0f,
                        m_centerPoint.x + 150.0f,
                        m_centerPoint.y);

                    pRenderTarget->DrawText(
                        coordText,
                        wcslen(coordText),
                        pTextFormat,
                        textRect,
                        centerBrush);

                    pTextFormat->Release();
                }

                if (pDWriteFactory) pDWriteFactory->Release();
                centerBrush->Release();
            }
        }

        DrawIntersectionPoints(m_graphicsEngine->GetRenderTarget());

        DrawSelectedIntersectionShapes(m_graphicsEngine->GetRenderTarget());

        /* ----- 右上角模式提示 ----- */
        ID2D1RenderTarget *rt = m_graphicsEngine->GetRenderTarget();
        // 1. 第一次进来时把 DirectWrite 工厂和格式建好
        static IDWriteFactory *pDW = nullptr;
        static IDWriteTextFormat *pTF = nullptr;
        if (!pDW) {
            DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown **>(&pDW));
            if (pDW)
                pDW->CreateTextFormat(
                    L"Consolas", nullptr,
                    DWRITE_FONT_WEIGHT_NORMAL,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    14.0f, L"en-us", &pTF);
        }

        // 2. 组装当前模式字符串
        const wchar_t *name = L"UNKNOWN";
        switch (m_currentMode) {
        case DrawingMode::SELECT: name = L"SELECT"; break;
        case DrawingMode::LINE: name = L"LINE"; break;
        case DrawingMode::MIDPOINT_LINE: name = L"MIDPOINT_LINE"; break;
        case DrawingMode::BRESENHAM_LINE: name = L"BRESENHAM_LINE"; break;
        case DrawingMode::MIDPOINT_CIRCLE: name = L"MIDPOINT_CIRCLE"; break;
        case DrawingMode::BRESENHAM_CIRCLE: name = L"BRESENHAM_CIRCLE"; break;
        case DrawingMode::CIRCLE: name = L"CIRCLE"; break;
        case DrawingMode::RECTANGLE: name = L"RECTANGLE"; break;
        case DrawingMode::TRIANGLE: name = L"TRIANGLE"; break;
        case DrawingMode::DIAMOND: name = L"DIAMOND"; break;
        case DrawingMode::PARALLELOGRAM: name = L"PARALLELOGRAM"; break;
        case DrawingMode::POLYLINE: name = L"POLYLINE"; break;
        case DrawingMode::CURVE: name = L"CURVE"; break;
        case DrawingMode::PERPENDICULAR: name = L"PERPENDICULAR"; break;
        case DrawingMode::TANGENT: name = L"TANGENT"; break;
        case DrawingMode::CENTER: name = L"CENTER"; break;
        case DrawingMode::INTERSECT: name = L"INTERSECT"; break;
        case DrawingMode::MULTI_BEZIER: name = L"MULTI_BEZIER"; break;
        case DrawingMode::SCANLINE_FILL: name = L"SCANLINE_FILL"; break;
        case DrawingMode::SEED_FILL: name = L"SEED_FILL"; break;
        case DrawingMode::POLYGON: name = L"POLYGON"; break;
        }
        WCHAR txt[64];
        swprintf_s(txt, L"Mode: %s", name);

        // 3. 建文本布局（每次重建，因为字符串会变）
        IDWriteTextLayout *lay = nullptr;
        pDW->CreateTextLayout(txt, wcslen(txt), pTF, 300, 30, &lay);

        DWRITE_TEXT_METRICS m;
        lay->GetMetrics(&m);

        // 4. 右上角定位
        D2D1_SIZE_F sz = rt->GetSize();
        float left = sz.width - m.width - 10.0f;
        float top = 10.0f;

        // 5. 画背景
        ID2D1SolidColorBrush *bgBr = nullptr;
        ID2D1SolidColorBrush *txtBr = nullptr;
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White, 0.9f), &bgBr);
        rt->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &txtBr);
        if (bgBr && txtBr) {
            D2D1_RECT_F rc = {left - 4, top - 4, left + m.width + 4, top + m.height + 4};
            rt->FillRectangle(&rc, bgBr);
            rt->DrawRectangle(&rc, txtBr, 1.0f);
            rt->DrawTextLayout({left, top}, lay, txtBr);
        }
        if (bgBr) bgBr->Release();
        if (txtBr) txtBr->Release();

        lay->Release();
        /* ------------------------- */

        m_graphicsEngine->EndDraw();
    }
}

void MainWindow::OnCommand(WPARAM wParam) {
    DrawingMode previousMode = m_currentMode;
    switch (LOWORD(wParam)) {
    case 32772: m_currentMode = DrawingMode::LINE; break;
    case 32773: m_currentMode = DrawingMode::CIRCLE; break;
    case 32774: m_currentMode = DrawingMode::RECTANGLE; break;
    case 32785: m_currentMode = DrawingMode::SELECT; break;
    case 32775: m_currentMode = DrawingMode::TRIANGLE; break;
    case 32776: m_currentMode = DrawingMode::DIAMOND; break;
    case 32777: m_currentMode = DrawingMode::PARALLELOGRAM; break;
    case 32779: m_currentMode = DrawingMode::POLYLINE; break;
    case 32778: m_currentMode = DrawingMode::CURVE; break;
    case 32780:
        m_currentMode = DrawingMode::INTERSECT;
        m_graphicsEngine->clearIntersection();
        break;
    case 32781:
        m_currentMode = DrawingMode::PERPENDICULAR;
        break;
    case 32782:
        m_currentMode = DrawingMode::CENTER;
        break;
    case 32783:
        m_currentMode = DrawingMode::TANGENT;
        break;
    case 32787:
        m_transformMode = TransformMode::MOVE;
        m_currentMode = DrawingMode::SELECT;
        break;
    case 32788:
        m_transformMode = TransformMode::ROTATE;
        m_currentMode = DrawingMode::SELECT;
        break;
    case 32789:
        m_transformMode = TransformMode::SCALE;
        m_currentMode = DrawingMode::SELECT;
        break;
    case 32790:
        SaveToFile();
        break;
    case 32791:
        LoadFromFile();
        break;
    case 32792:
        m_currentMode = DrawingMode::MIDPOINT_LINE;
        break;
    case 32793:
        m_currentMode = DrawingMode::BRESENHAM_LINE;
        break;
    case 32794:
        m_currentMode = DrawingMode::MIDPOINT_CIRCLE;
        break;
    case 32795:
        m_currentMode = DrawingMode::BRESENHAM_CIRCLE;
        break;
    // 线宽菜单项 - 应用到选中的图形
    case 32799:
        m_currentLineWidth = LineWidth::WIDTH_1PX;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                // 支持所有类型的直线和圆形
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineWidth(m_currentLineWidth);
                    OutputDebugStringA("Line width set to 1PX\n");
                }
            }
        }
        break;
    case 32800:
        m_currentLineWidth = LineWidth::WIDTH_2PX;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineWidth(m_currentLineWidth);
                }
            }
        }
        break;
    case 32801:
        m_currentLineWidth = LineWidth::WIDTH_4PX;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineWidth(m_currentLineWidth);
                }
            }
        }
        break;
    case 32802:
        m_currentLineWidth = LineWidth::WIDTH_8PX;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineWidth(m_currentLineWidth);
                }
            }
        }
        break;
    case 32803:
        m_currentLineWidth = LineWidth::WIDTH_16PX;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineWidth(m_currentLineWidth);
                }
            }
        }
        break;
    case 32806: // 实线线型
        m_currentLineStyle = LineStyle::SOLID;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineStyle(m_currentLineStyle);
                }
            }
        }
        break;
    case 32807: // 点划线（长划线-点-长划线）型
        m_currentLineStyle = LineStyle::DASH_DOT;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineStyle(m_currentLineStyle);
                }
            }
        }
        break;
    case 32808: // 虚线型
        m_currentLineStyle = LineStyle::DASH;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineStyle(m_currentLineStyle);
                }
            }
        }
        break;
    case 32809: // 点线型
        m_currentLineStyle = LineStyle::DOT;
        if (m_graphicsEngine->IsShapeSelected()) {
            auto selectedShape = m_graphicsEngine->GetSelectedShape();
            if (selectedShape) {
                ShapeType type = selectedShape->GetType();
                if (type == ShapeType::LINE || type == ShapeType::CIRCLE) {
                    selectedShape->SetLineStyle(m_currentLineStyle);
                }
            }
        }
        break;
    case 32810: // 多点Bezier曲线
        m_currentMode = DrawingMode::MULTI_BEZIER;
        OutputDebugStringA("多点Bezier曲线模式已激活\n");
        break;
    case 32811: // 栅栏填充法
        m_currentMode = DrawingMode::SCANLINE_FILL;
        OutputDebugStringA("栅栏填充模式已激活\n");
        break;
    case 32812: // 种子填充法
        m_currentMode = DrawingMode::SEED_FILL;
        OutputDebugStringA("种子填充模式已激活\n");
        break;
    case 32813: // 选中图元后，再用鼠标指定旋转点，让图元绕该点旋转
        break;
    case 32814: // 绘制任意多边形
        m_currentMode = DrawingMode::POLYGON;
        OutputDebugStringA("多边形绘制模式已激活\n");
        break;
    case 32816: // 利用LiangBarsky算法裁剪矩形框内直线
        break;
    case 32817: // 利用Sutherland-Hodgman算法裁剪矩形框内多边形
        break;
    case 32818: // 利用Weiler-Atherton算法裁剪矩形内多边形
        break;
    case 5: m_graphicsEngine->DeleteSelectedShape(); break;
    }

    // 更新图形引擎的绘图模式
    if (m_graphicsEngine) {
        m_graphicsEngine->SetDrawingMode(m_currentMode);
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// UTF-8 窄串 ↔ 宽串
static std::wstring StringToWString(const std::string &s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

static std::string WStringToString(const std::wstring &ws) {
    if (ws.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len, nullptr, nullptr);
    return s;
}

void MainWindow::SaveToFile() {
    WCHAR szFile[MAX_PATH] = L"";

    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Drawing Files (*.drawing)\0*.drawing\0All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"drawing";

    if (GetSaveFileName(&ofn)) {
        std::wofstream out(szFile);
        if (out) {
            for (const auto &shape : m_graphicsEngine->GetShapes())
                out << StringToWString(shape->Serialize()) << L'\n';
        }
    }
}

void MainWindow::LoadFromFile() {
    WCHAR szFile[MAX_PATH] = L"";

    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Drawing Files (*.drawing)\0*.drawing\0All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        std::wifstream inFile(szFile);
        if (!inFile) return;

        // 1. 清空画布
        m_graphicsEngine->ClearAllShapes();

        // 2. 加载新数据
        std::wstring line;
        while (std::getline(inFile, line)) {
            if (auto shape = Shape::Deserialize(WStringToString(line)))
                m_graphicsEngine->AddShape(shape);
        }

        // 3. 重置交互状态
        m_graphicsEngine->ClearSelection();
        m_graphicsEngine->clearIntersection();
        ResetDrawingState();
    }
}

HRESULT MainWindow::Initialize(HINSTANCE hInstance, int nCmdShow) {
    // 注册窗口类
    const wchar_t CLASS_NAME[] = L"Simple Drawing App";

    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    // 创建窗口
    m_hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Simple Drawing App",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, this);

    if (m_hwnd == nullptr) {
        return E_FAIL;
    }

    // 初始化图形引擎
    HRESULT hr = m_graphicsEngine->Initialize(m_hwnd);
    if (FAILED(hr)) {
        return hr;
    }

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    return S_OK;
}

int MainWindow::Run() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// 程序入口
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
    MainWindow app;

    if (FAILED(app.Initialize(hInstance, nCmdShow))) {
        return -1;
    }

    return app.Run();
}