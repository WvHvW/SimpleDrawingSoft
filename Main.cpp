#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <memory>
#include <cmath>
#include "GraphicsEngine.h"
#include "Shape.h"
#include "Resource.h"

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
    D2D1_POINT_2F m_startPoint, m_endPoint, m_midPoint;
    bool m_isDrawing = false;
    std::vector<D2D1_POINT_2F> m_polyPoints;
    int m_clickCount = 0;
    std::shared_ptr<Shape> m_tempShape;
    std::shared_ptr<Curve> m_currentCurve;
    bool m_isDrawingCurve = false;
    std::shared_ptr<Line> m_tempPolyLine;

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
            // 点击空白处，取消选择和变换
            m_graphicsEngine->ClearSelection();
            CancelTransform();
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        }
        break;

    case DrawingMode::LINE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<Line>(m_startPoint, currentPoint);
        } else {
            m_graphicsEngine->AddShape(std::make_shared<Line>(m_startPoint, currentPoint));
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
            m_graphicsEngine->AddShape(std::make_shared<Circle>(m_startPoint, radius));
            ResetDrawingState();
        }
        break;

    case DrawingMode::RECTANGLE:
        if (m_clickCount == 0) {
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<Rectangle>(m_startPoint, currentPoint);
        } else {
            m_graphicsEngine->AddShape(std::make_shared<Rectangle>(m_startPoint, currentPoint));
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
            m_startPoint = currentPoint;
            m_clickCount = 1;
            m_isDrawing = true;
            m_tempShape = std::make_shared<Diamond>(m_startPoint, currentPoint);
        } else {
            m_graphicsEngine->AddShape(std::make_shared<Diamond>(m_startPoint, currentPoint));
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
        // 曲线绘制：按下左键开始绘制，移动时连续添加点，右键结束
        if (!m_isDrawingCurve) {
            // 开始新曲线
            m_currentCurve = std::make_shared<Curve>(std::vector<D2D1_POINT_2F>());
            m_currentCurve->AddPoint(currentPoint);
            m_isDrawingCurve = true;
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

    case DrawingMode::TANGENT: // 假设这是切线模式的枚举值
        if (!m_isDrawingTangent) {
            // 第一次点击：选择圆
            if (auto selectedShape = m_graphicsEngine->SelectShape(currentPoint)) {
                if (selectedShape->GetType() == ShapeType::CIRCLE) {
                    m_selectedCircleForTangent = std::dynamic_pointer_cast<Circle>(selectedShape);
                    m_isDrawingTangent = true;
                }
            }
        } else {
            // 第二次点击：确定切线方向并绘制切线
            if (m_selectedCircleForTangent) {
                auto tangents = m_graphicsEngine->CreateTangents(currentPoint, m_selectedCircleForTangent);
                for (auto &tangent : tangents) {
                    m_graphicsEngine->AddShape(tangent);
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
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::OnLButtonUp(int x, int y) {
    // 结束变换操作
    if (m_isTransforming) {
        EndTransform();
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

        case DrawingMode::CIRCLE: {
            float radius = CalculateDistance(m_startPoint, currentPoint);
            m_tempShape = std::make_shared<Circle>(m_startPoint, radius);
            break;
        }

        case DrawingMode::RECTANGLE:
            m_tempShape = std::make_shared<Rectangle>(m_startPoint, currentPoint);
            break;

        case DrawingMode::TRIANGLE:
            m_tempShape = CreateEquilateralTriangle(m_startPoint, currentPoint);
            break;

        case DrawingMode::DIAMOND:
            m_tempShape = std::make_shared<Diamond>(m_startPoint, currentPoint);
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
    // 曲线绘制：实时添加点
    if (m_currentMode == DrawingMode::CURVE && m_isDrawingCurve && m_currentCurve) {
        // 只在移动时添加点，避免重复添加相同点
        static D2D1_POINT_2F lastPoint = {-1, -1};
        float distance = CalculateDistance(lastPoint, currentPoint);

        if (distance > 2.0f) { // 避免添加过于密集的点
            m_currentCurve->AddPoint(currentPoint);
            lastPoint = currentPoint;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    // 多段线模式：实时预览当前线段
    if (m_currentMode == DrawingMode::POLYLINE && !m_polyPoints.empty()) {
        // 创建从最后一个点到当前鼠标位置的预览线段
        m_tempPolyLine = std::make_shared<Line>(m_polyPoints.back(), currentPoint);
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

// 辅助函数
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
    m_currentCurve.reset();
    m_tempPolyLine.reset();
    ResetTangentState();
    ResetCenterState();
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
    // 右键结束曲线绘制
    if (m_currentMode == DrawingMode::CURVE && m_isDrawingCurve && m_currentCurve) {
        // 完成曲线绘制
        if (m_currentCurve->GetPoints().size() >= 2) {
            m_graphicsEngine->AddShape(m_currentCurve);
        }
        ResetDrawingState();
    } else if (m_currentMode == DrawingMode::POLYLINE) {
        // 结束多段线绘制
        if (m_polyPoints.size() >= 2) {
            auto polyline = std::make_shared<Curve>(m_polyPoints);
            m_graphicsEngine->AddShape(polyline);
        }
        m_polyPoints.clear();
        m_tempPolyLine.reset();
    } else {
        // 其他模式的右键取消
        ResetDrawingState();
        m_graphicsEngine->ClearSelection();
        CancelTransform();
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}
void MainWindow::OnKeyDown(WPARAM wParam) {
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

        // 绘制切线预览
        if (m_currentMode == DrawingMode::TANGENT && m_isDrawingTangent && !m_tempTangents.empty()) {
            ID2D1RenderTarget *pRenderTarget = m_graphicsEngine->GetRenderTarget();
            ID2D1SolidColorBrush *tempBrush = nullptr;
            HRESULT hr = pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Orange), &tempBrush);

            if (SUCCEEDED(hr) && tempBrush) {
                for (auto &tangent : m_tempTangents) {
                    // 绘制切线
                    tangent->Draw(pRenderTarget, tempBrush, tempBrush, nullptr);

                    // 获取切线的起点和终点（终点是切点）
                    D2D1_POINT_2F startPoint = tangent->GetStart();
                    D2D1_POINT_2F endPoint = tangent->GetEnd();

                    // 绘制切点标记
                    D2D1_ELLIPSE tangentPoint = D2D1::Ellipse(endPoint, 4.0f, 4.0f);
                    pRenderTarget->FillEllipse(tangentPoint, tempBrush);

                    // 绘制切点坐标文本背景
                    ID2D1SolidColorBrush *textBgBrush = nullptr;
                    pRenderTarget->CreateSolidColorBrush(
                        D2D1::ColorF(D2D1::ColorF::White, 0.8f), &textBgBrush);

                    if (textBgBrush) {
                        // 格式化坐标文本
                        WCHAR coordText[100];
                        swprintf_s(coordText, L"切点: (%.1f, %.1f)", endPoint.x, endPoint.y);

                        // 计算文本位置（确保不会超出屏幕）
                        float textX = endPoint.x + 10.0f;
                        float textY = endPoint.y - 15.0f;

                        // 如果切点在屏幕右侧，调整文本位置到左侧
                        RECT clientRect;
                        GetClientRect(m_hwnd, &clientRect);
                        if (textX + 120.0f > clientRect.right) {
                            textX = endPoint.x - 130.0f;
                        }

                        // 如果切点在屏幕底部，调整文本位置到上方
                        if (textY < clientRect.top + 10) {
                            textY = endPoint.y + 10.0f;
                        }

                        D2D1_RECT_F textRect = D2D1::RectF(
                            textX, textY, textX + 120.0f, textY + 20.0f);

                        // 绘制文本背景
                        pRenderTarget->FillRectangle(textRect, textBgBrush);

                        // 绘制坐标文本边框
                        pRenderTarget->DrawRectangle(textRect, tempBrush, 1.0f);

                        textBgBrush->Release();

                        // 使用GDI绘制文本（避免DirectWrite依赖）
                        HDC hdc = GetDC(m_hwnd);
                        SetTextColor(hdc, RGB(255, 165, 0)); // 橙色文本
                        SetBkColor(hdc, RGB(255, 255, 255)); // 白色背景

                        // 转换坐标到屏幕坐标
                        POINT screenPoint = {
                            static_cast<LONG>(textX + 5),
                            static_cast<LONG>(textY + 2)};

                        // 绘制文本
                        TextOutW(hdc, screenPoint.x, screenPoint.y, coordText, wcslen(coordText));
                        ReleaseDC(m_hwnd, hdc);
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

        m_graphicsEngine->EndDraw();
    }
}

void MainWindow::OnCommand(WPARAM wParam) {
    // 处理菜单和工具栏命令
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
    case 32781:
        m_currentMode = DrawingMode ::PERPENDICULAR;
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
    case 5: m_graphicsEngine->DeleteSelectedShape(); break;
    }

    // 更新图形引擎的绘图模式
    if (m_graphicsEngine) {
        m_graphicsEngine->SetDrawingMode(m_currentMode);
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::SaveToFile() {
    // 实现文件保存
}

void MainWindow::LoadFromFile() {
    // 实现文件加载
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