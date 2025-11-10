#include "GraphicsEngine.h"
#include "Shape.h"
#include <cmath>

GraphicsEngine::GraphicsEngine() :
    m_hwnd(nullptr), m_pD2DFactory(nullptr), m_pRenderTarget(nullptr), m_pNormalBrush(nullptr), m_pSelectedBrush(nullptr), m_pDWriteFactory(nullptr), m_currentMode(DrawingMode::SELECT) {
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
            // 虚线
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
        shape->Draw(m_pRenderTarget, m_pNormalBrush, m_pSelectedBrush, m_pStrokeStyle);
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
