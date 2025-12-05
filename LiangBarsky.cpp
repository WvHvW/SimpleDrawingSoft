#include "Main.cpp"

// Liang-Barsky裁剪算法实现
bool MainWindow::LiangBarskyClip(float &x1, float &y1, float &x2, float &y2,
                                  float xmin, float ymin, float xmax, float ymax) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {x1 - xmin, xmax - x1, y1 - ymin, ymax - y1};
    
    float u1 = 0.0f, u2 = 1.0f;
    
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0) {
            // 线段平行于边界
            if (q[i] < 0) {
                return false; // 完全在外部
            }
        } else {
            float t = q[i] / p[i];
            if (p[i] < 0) {
                // 从外部进入
                if (t > u1) u1 = t;
            } else {
                // 从内部离开
                if (t < u2) u2 = t;
            }
        }
    }
    
    if (u1 > u2) {
        return false; // 线段完全在外部
    }
    
    // 计算裁剪后的端点
    float newX1 = x1 + u1 * dx;
    float newY1 = y1 + u1 * dy;
    float newX2 = x1 + u2 * dx;
    float newY2 = y1 + u2 * dy;
    
    x1 = newX1;
    y1 = newY1;
    x2 = newX2;
    y2 = newY2;
    
    return true;
}

// 应用裁剪
void MainWindow::ApplyClipping() {
    float xmin = min(m_clipRectStart.x, m_clipRectEnd.x);
    float ymin = min(m_clipRectStart.y, m_clipRectEnd.y);
    float xmax = max(m_clipRectStart.x, m_clipRectEnd.x);
    float ymax = max(m_clipRectStart.y, m_clipRectEnd.y);
    
    auto &shapes = m_graphicsEngine->GetShapes();
    std::vector<std::shared_ptr<Shape>> newShapes;
    
    for (const auto &shape : shapes) {
        ShapeType type = shape->GetType();
        
        // 只对直线类型进行裁剪
        if (type == ShapeType::LINE) {
            // 尝试转换为Line类型
            if (auto line = std::dynamic_pointer_cast<Line>(shape)) {
                D2D1_POINT_2F start = line->GetStart();
                D2D1_POINT_2F end = line->GetEnd();
                
                float x1 = start.x, y1 = start.y;
                float x2 = end.x, y2 = end.y;
                
                // 应用Liang-Barsky裁剪
                if (LiangBarskyClip(x1, y1, x2, y2, xmin, ymin, xmax, ymax)) {
                    // 创建裁剪后的新直线
                    auto clippedLine = std::make_shared<Line>(
                        D2D1::Point2F(x1, y1),
                        D2D1::Point2F(x2, y2)
                    );
                    clippedLine->SetLineWidth(line->GetLineWidth());
                    clippedLine->SetLineStyle(line->GetLineStyle());
                    newShapes.push_back(clippedLine);
                }
                // 如果裁剪失败，线段被完全裁剪掉，不添加到newShapes
            } else {
                // 保留非Line类型的直线（MidpointLine, BresenhamLine等）
                newShapes.push_back(shape);
            }
        } else {
            // 保留其他类型的图元
            newShapes.push_back(shape);
        }
    }
    
    // 清空原有图元并添加裁剪后的图元
    m_graphicsEngine->ClearAllShapes();
    for (const auto &shape : newShapes) {
        m_graphicsEngine->AddShape(shape);
    }
    
    OutputDebugStringA("Liang-Barsky裁剪完成\n");
}
