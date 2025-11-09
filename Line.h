// Line.h
#pragma once
#include "Shape.h"
#include <d2d1.h>

class Line : public Shape {
public:
    Line(D2D1_POINT_2F start) : m_start(start), m_end(start) {}
    void SetEnd(D2D1_POINT_2F end) { m_end = end; m_complete = true; }
    void Render(ID2D1RenderTarget* rt, ID2D1SolidBrush* brush, bool selected) override;
    bool HitTest(D2D1_POINT_2F pt, float tolerance) override;
    void Move(D2D1_POINT_2F delta) override;
    void Complete() override { m_complete = true; }

private:
    D2D1_POINT_2F m_start, m_end;
};