#include "GraphicsEngine.h"
#include "Shape.h"
#include "IntersectionManager.h"
#include <cmath>

GraphicsEngine::GraphicsEngine() :
    m_hwnd(nullptr), m_pD2DFactory(nullptr), m_pRenderTarget(nullptr), m_pNormalBrush(nullptr), m_pSelectedBrush(nullptr), m_pDWriteFactory(nullptr), m_currentMode(DrawingMode::SELECT),
    m_pStrokeStyle(nullptr), m_pSolidStrokeStyle(nullptr), m_pDashStrokeStyle(nullptr), m_pDotStrokeStyle(nullptr), m_pDashDotStrokeStyle(nullptr), m_pDashDotDotStrokeStyle(nullptr) {
}

GraphicsEngine::~GraphicsEngine() {
    Cleanup();
}

HRESULT GraphicsEngine::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    // 创建D2D工厂
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (SUCCEEDED(hr)) {
        hr = CreateDeviceResources();
    }

    return hr;
}

HRESULT GraphicsEngine::CreateDeviceResources() {
    HRESULT hr = S_OK;

    if (m_pRenderTarget == nullptr) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRenderTarget);

        if (SUCCEEDED(hr)) {
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pNormalBrush);
        }

        if (SUCCEEDED(hr)) {
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray), &m_pSelectedBrush);
        }

        if (SUCCEEDED(hr)) {
            // 创建实线样式
            hr = m_pD2DFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_SOLID,
                    0.0f),
                nullptr,
                0,
                &m_pSolidStrokeStyle);
        }

        if (SUCCEEDED(hr)) {
            // 创建虚线样式
            FLOAT dashes[] = {5.0f, 5.0f};
            hr = m_pD2DFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_CUSTOM,
                    0.0f),
                dashes,
                ARRAYSIZE(dashes),
                &m_pDashStrokeStyle);
        }

        if (SUCCEEDED(hr)) {
            // 创建点线样式
            FLOAT dots[] = {1.0f, 3.0f};
            hr = m_pD2DFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_ROUND,
                    D2D1_CAP_STYLE_ROUND,
                    D2D1_CAP_STYLE_ROUND,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_CUSTOM,
                    0.0f),
                dots,
                ARRAYSIZE(dots),
                &m_pDotStrokeStyle);
        }

        if (SUCCEEDED(hr)) {
            // 创建点划线样式（长划线-点-长划线）
            FLOAT dashDot[] = {8.0f, 3.0f, 1.0f, 3.0f};
            hr = m_pD2DFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_ROUND,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_CUSTOM,
                    0.0f),
                dashDot,
                ARRAYSIZE(dashDot),
                &m_pDashDotStrokeStyle);
        }

        if (SUCCEEDED(hr)) {
            // 创建双点划线样式（长划线-点-点-长划线）
            FLOAT dashDotDot[] = {8.0f, 3.0f, 1.0f, 3.0f, 1.0f, 3.0f};
            hr = m_pD2DFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_ROUND,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_CUSTOM,
                    0.0f),
                dashDotDot,
                ARRAYSIZE(dashDotDot),
                &m_pDashDotDotStrokeStyle);
        }

        // 保持原有的虚线样式作为默认选择样式
        if (SUCCEEDED(hr)) {
            FLOAT dashes[] = {2.0f, 2.0f};
            hr = m_pD2DFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_CUSTOM,
                    0.0f),
                dashes,
                ARRAYSIZE(dashes),
                &m_pStrokeStyle);
        }
    }

    return hr;
}

void GraphicsEngine::Resize(UINT width, UINT height) {
    if (m_pRenderTarget) {
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void GraphicsEngine::Render() {
    if (m_pRenderTarget == nullptr) {
        return;
    }

    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    for (auto &shape : m_shapes) {
        // 获取形状的线型样式
        ID2D1StrokeStyle* shapeStrokeStyle = GetStrokeStyle(shape->GetLineStyle());
        // 如果形状被选中，使用选择样式，否则使用形状自己的线型样式
        ID2D1StrokeStyle* strokeStyleToUse = shape->IsSelected() ? m_pStrokeStyle : shapeStrokeStyle;
        shape->Draw(m_pRenderTarget, m_pNormalBrush, m_pSelectedBrush, strokeStyleToUse);
    }
}

void GraphicsEngine::Cleanup() {
    if (m_pNormalBrush) {
        m_pNormalBrush->Release();
        m_pNormalBrush = nullptr;
    }
    if (m_pSelectedBrush) {
        m_pSelectedBrush->Release();
        m_pSelectedBrush = nullptr;
    }
    if (m_pStrokeStyle) {
        m_pStrokeStyle->Release();
        m_pStrokeStyle = nullptr;
    }
    if (m_pSolidStrokeStyle) {
        m_pSolidStrokeStyle->Release();
        m_pSolidStrokeStyle = nullptr;
    }
    if (m_pDashStrokeStyle) {
        m_pDashStrokeStyle->Release();
        m_pDashStrokeStyle = nullptr;
    }
    if (m_pDotStrokeStyle) {
        m_pDotStrokeStyle->Release();
        m_pDotStrokeStyle = nullptr;
    }
    if (m_pDashDotStrokeStyle) {
        m_pDashDotStrokeStyle->Release();
        m_pDashDotStrokeStyle = nullptr;
    }
    if (m_pDashDotDotStrokeStyle) {
        m_pDashDotDotStrokeStyle->Release();
        m_pDashDotDotStrokeStyle = nullptr;
    }
    if (m_pRenderTarget) {
        m_pRenderTarget->Release();
        m_pRenderTarget = nullptr;
    }
    if (m_pD2DFactory) {
        m_pD2DFactory->Release();
        m_pD2DFactory = nullptr;
    }
    if (m_pDWriteFactory) {
        m_pDWriteFactory->Release();
        m_pDWriteFactory = nullptr;
    }
}

void GraphicsEngine::AddShape(std::shared_ptr<Shape> shape) {
    m_shapes.push_back(shape);
}

void GraphicsEngine::DeleteSelectedShape() {
    if (m_selectedShape) {
        auto it = std::find(m_shapes.begin(), m_shapes.end(), m_selectedShape);
        if (it != m_shapes.end()) {
            m_shapes.erase(it);
            m_selectedShape = nullptr;
        }
    }
}

std::shared_ptr<Shape> GraphicsEngine::SelectShape(D2D1_POINT_2F point) {
    for (auto it = m_shapes.rbegin(); it != m_shapes.rend(); ++it) {
        if ((*it)->HitTest(point)) {
            if (m_selectedShape) {
                m_selectedShape->SetSelected(false);
            }
            m_selectedShape = *it;
            m_selectedShape->SetSelected(true);
            return m_selectedShape;
        }
    }

    // 如果没有选中任何图形，清除选择
    ClearSelection();
    return nullptr;
}

void GraphicsEngine::ClearSelection() {
    if (m_selectedShape) {
        m_selectedShape->SetSelected(false);
        m_selectedShape = nullptr;
    }
}

void GraphicsEngine::MoveSelectedShape(float dx, float dy) {
    if (m_selectedShape) {
        m_selectedShape->Move(dx, dy);
    }
}

void GraphicsEngine::RotateSelectedShape(float angle) {
    if (m_selectedShape) {
        m_selectedShape->Rotate(angle);
    }
}

void GraphicsEngine::ScaleSelectedShape(float scale) {
    if (m_selectedShape) {
        m_selectedShape->Scale(scale);
    }
}

void GraphicsEngine::RotateAroundPoint(float angle, D2D1_POINT_2F center) {
    if (m_selectedShape) {
        m_selectedShape->RotateAroundPoint(angle, center);
    }
}

std::shared_ptr<Line> GraphicsEngine::CreatePerpendicularLine(std::shared_ptr<Line> line, D2D1_POINT_2F point) {
    if (!line) return nullptr;

    // 获取直线的起点和终点
    D2D1_POINT_2F start = line->GetStart();
    D2D1_POINT_2F end = line->GetEnd();

    // 计算直线方向向量
    float dx = end.x - start.x;
    float dy = end.y - start.y;

    // 如果直线是水平的或垂直的，直接处理
    if (fabs(dx) < 0.001f) { // 垂直线
        // 垂直线 -> 水平垂直线
        float length = 50.0f; // 固定长度
        return std::make_shared<Line>(
            D2D1::Point2F(point.x - length, point.y),
            D2D1::Point2F(point.x + length, point.y));
    } else if (fabs(dy) < 0.001f) { // 水平线
        // 水平线 -> 垂直垂直线
        float length = 50.0f; // 固定长度
        return std::make_shared<Line>(
            D2D1::Point2F(point.x, point.y - length),
            D2D1::Point2F(point.x, point.y + length));
    }

    float slope = dy / dx;
    float perpendicularSlope = -dx / dy;

    // 使用点斜式方程计算垂直线
    float length = 50.0f; // 固定长度

    // 计算垂直线的两个端点
    D2D1_POINT_2F p1, p2;
    p1.x = point.x - length / sqrtf(1 + perpendicularSlope * perpendicularSlope);
    p1.y = point.y + perpendicularSlope * (p1.x - point.x);

    p2.x = point.x + length / sqrtf(1 + perpendicularSlope * perpendicularSlope);
    p2.y = point.y + perpendicularSlope * (p2.x - point.x);

    return std::make_shared<Line>(p1, p2);
}

std::vector<std::shared_ptr<Line>> GraphicsEngine::CreateTangents(D2D1_POINT_2F point, std::shared_ptr<Circle> circle) {
    std::vector<std::shared_ptr<Line>> tangents;

    if (!circle) return tangents;

    D2D1_POINT_2F center = circle->GetCenter();
    float radius = circle->GetRadius();

    // 计算点到圆心的距离
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    float distance = sqrtf(dx * dx + dy * dy);

    // 如果点在圆内，没有切线
    if (distance < radius) {
        return tangents;
    }

    // 计算切点
    // 使用相似三角形原理计算切点
    float a = radius * radius / distance;
    float h = sqrtf(radius * radius - a * a);

    // 单位向量从圆心指向点
    float ux = dx / distance;
    float uy = dy / distance;

    // 垂直向量
    float vx = -uy;
    float vy = ux;

    // 计算两个切点
    D2D1_POINT_2F tangent1, tangent2;

    // 第一个切点
    tangent1.x = center.x + a * ux + h * vx;
    tangent1.y = center.y + a * uy + h * vy;

    // 第二个切点
    tangent2.x = center.x + a * ux - h * vx;
    tangent2.y = center.y + a * uy - h * vy;

    // 创建切线
    tangents.push_back(std::make_shared<Line>(point, tangent1));
    tangents.push_back(std::make_shared<Line>(point, tangent2));

    return tangents;
}

bool GraphicsEngine::selectShapeForIntersection(std::shared_ptr<Shape> shape) {
    return IntersectionManager::getInstance().selectShape(shape);
}

void GraphicsEngine::calculateIntersection() {
    IntersectionManager::getInstance().calculateIntersection();
}

void GraphicsEngine::clearIntersection() {
    IntersectionManager::getInstance().clear();
}

const std::vector<D2D1_POINT_2F> &GraphicsEngine::getIntersectionPoints() const {
    return IntersectionManager::getInstance().getIntersectionPoints();
}

bool GraphicsEngine::isIntersectionReady() const {
    return IntersectionManager::getInstance().hasTwoShapes();
}

std::shared_ptr<Shape> GraphicsEngine::getFirstIntersectionShape() const {
    return IntersectionManager::getInstance().getFirstShape();
}

std::shared_ptr<Shape> GraphicsEngine::getSecondIntersectionShape() const {
    return IntersectionManager::getInstance().getSecondShape();
}

ID2D1StrokeStyle* GraphicsEngine::GetStrokeStyle(LineStyle lineStyle) {
    switch (lineStyle) {
        case LineStyle::SOLID:
            return m_pSolidStrokeStyle;
        case LineStyle::DASH:
            return m_pDashStrokeStyle;
        case LineStyle::DOT:
            return m_pDotStrokeStyle;
        case LineStyle::DASH_DOT:
            return m_pDashDotStrokeStyle;
        case LineStyle::DASH_DOT_DOT:
            return m_pDashDotDotStrokeStyle;
        default:
            return m_pSolidStrokeStyle;
    }
}
