#include "Line.h"

// Line.cpp
void Line::Render(ID2D1RenderTarget *rt, ID2D1SolidBrush *brush, bool selected) {
    if (selected) {
        ID2D1StrokeStyle *dashStyle = nullptr;
        D2D1_STROKE_STYLE_PROPERTIES props = D2D1::StrokeStyleProperties(
            D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
            D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_DASH, 0.0f);
        rt->CreateStrokeStyle(props, nullptr, 0, &dashStyle);
        brush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
        rt->DrawLine(m_start, m_end, brush, 2.0f, dashStyle);
        dashStyle->Release();
    } else {
        brush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
        rt->DrawLine(m_start, m_end, brush, 2.0f);
    }
}

bool Line::HitTest(D2D1_POINT_2F pt, float tolerance) {
    // µ„µΩœﬂ∂Œæ‡¿Î
    float A = pt.y - m_start.y;
    float B = m_start.x - pt.x;
    float C = m_end.x - m_start.x;
    float D = m_end.y - m_start.y;
    float dot = A * C + B * D;
    float len_sq = C * C + D * D;
    if (len_sq == 0) return false;
    float param = dot / len_sq;
    if (param < 0 || param > 1) return false;
    D2D1_POINT_2F closest = {m_start.x + param * C, m_start.y + param * D};
    float dx = pt.x - closest.x, dy = pt.y - closest.y;
    return (dx * dx + dy * dy) <= (tolerance * tolerance);
}

void Line::Move(D2D1_POINT_2F delta) {
    m_start.x += delta.x;
    m_start.y += delta.y;
    m_end.x += delta.x;
    m_end.y += delta.y;
}
