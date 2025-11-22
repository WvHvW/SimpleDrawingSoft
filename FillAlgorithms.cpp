#include "FillAlgorithms.h"
#include <algorithm>
#include <stack>
#include <set>
#include <map>
#include <cmath>

namespace FillAlgorithms {

// 辅助函数：判断点是否在形状内部
static bool IsPointInsideShape(Shape* shape, D2D1_POINT_2F point) {
    if (!shape) return false;
    
    ShapeType type = shape->GetType();
    D2D1_RECT_F bounds = shape->GetBounds();
    
    // 首先检查边界框
    if (point.x < bounds.left || point.x > bounds.right ||
        point.y < bounds.top || point.y > bounds.bottom) {
        return false;
    }
    
    switch (type) {
    case ShapeType::CIRCLE: {
        // 使用GetCircleGeometry支持所有圆形类（Circle、MidpointCircle、BresenhamCircle）
        D2D1_POINT_2F center;
        float radius;
        if (shape->GetCircleGeometry(center, radius)) {
            float dx = point.x - center.x;
            float dy = point.y - center.y;
            return (dx * dx + dy * dy) < (radius * radius);
        }
        break;
    }
    case ShapeType::RECTANGLE: {
        auto rect = dynamic_cast<Rect*>(shape);
        if (rect) {
            D2D1_RECT_F bounds = rect->GetBounds();
            return (point.x >= bounds.left && point.x <= bounds.right &&
                    point.y >= bounds.top && point.y <= bounds.bottom);
        }
        break;
    }
    case ShapeType::TRIANGLE: {
        auto triangle = dynamic_cast<Triangle*>(shape);
        if (triangle) {
            // 使用重心坐标法判断点是否在三角形内
            D2D1_POINT_2F v0 = triangle->GetVertex1();
            D2D1_POINT_2F v1 = triangle->GetVertex2();
            D2D1_POINT_2F v2 = triangle->GetVertex3();
            
            float denominator = ((v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y));
            if (fabs(denominator) < 0.0001f) return false;
            
            float a = ((v1.y - v2.y) * (point.x - v2.x) + (v2.x - v1.x) * (point.y - v2.y)) / denominator;
            float b = ((v2.y - v0.y) * (point.x - v2.x) + (v0.x - v2.x) * (point.y - v2.y)) / denominator;
            float c = 1.0f - a - b;
            
            return (a >= 0 && b >= 0 && c >= 0);
        }
        break;
    }
    case ShapeType::POLYLINE: {
        // 使用射线法判断点是否在多边形内
        auto poly = dynamic_cast<Poly*>(shape);
        if (poly) {
            const std::vector<D2D1_POINT_2F>& points = poly->GetPoints();
            if (points.size() < 3) return false; // 至少需要3个点形成多边形
            
            int intersections = 0;
            size_t n = points.size();
            
            // 从点发出水平向右的射线，计算与多边形边的交点数
            for (size_t i = 0; i < n; ++i) {
                D2D1_POINT_2F p1 = points[i];
                D2D1_POINT_2F p2 = points[(i + 1) % n]; // 闭合多边形
                
                // 检查射线是否与边相交
                if ((p1.y <= point.y && p2.y > point.y) || (p2.y <= point.y && p1.y > point.y)) {
                    float xIntersection = p1.x + (point.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                    if (point.x < xIntersection) {
                        intersections++;
                    }
                }
            }
            
            // 奇数个交点表示在多边形内
            return (intersections % 2) == 1;
        }
        break;
    }
    case ShapeType::DIAMOND:
    case ShapeType::PARALLELOGRAM: {
        // 使用射线法判断点是否在四边形内
        // 通过GetIntersectionSegments获取4条边，从中提取4个顶点
        auto segments = shape->GetIntersectionSegments();
        if (segments.size() != 4) return false;
        
        // 提取4个顶点（按边的起点）
        std::vector<D2D1_POINT_2F> vertices;
        vertices.push_back(segments[0].first);  // 第一条边的起点
        vertices.push_back(segments[1].first);  // 第二条边的起点
        vertices.push_back(segments[2].first);  // 第三条边的起点
        vertices.push_back(segments[3].first);  // 第四条边的起点
        
        // 使用射线法判断点是否在四边形内
        int intersections = 0;
        for (size_t i = 0; i < 4; ++i) {
            D2D1_POINT_2F p1 = vertices[i];
            D2D1_POINT_2F p2 = vertices[(i + 1) % 4];
            
            // 检查射线是否与边相交
            if ((p1.y <= point.y && p2.y > point.y) || (p2.y <= point.y && p1.y > point.y)) {
                float xIntersection = p1.x + (point.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                if (point.x < xIntersection) {
                    intersections++;
                }
            }
        }
        
        // 奇数个交点表示在四边形内
        return (intersections % 2) == 1;
    }
    default:
        return false;
    }
    
    return false;
}

// 栅栏填充法（Fence Fill）
// 过一个顶点做垂直线作为栅栏，对栅栏与各边区域内的像素进行取补标记
// 全部边都被取过后，仍有标记的像素记为要填充的像素
std::vector<D2D1_POINT_2F> ScanlineFill(Shape* shape, D2D1_POINT_2F seedPoint) {
    std::vector<D2D1_POINT_2F> fillPixels;
    
    if (!shape) return fillPixels;
    
    // 检查种子点是否在图形内部
    if (!IsPointInsideShape(shape, seedPoint)) {
        return fillPixels;
    }
    
    D2D1_RECT_F bounds = shape->GetBounds();
    int minY = static_cast<int>(bounds.top);
    int maxY = static_cast<int>(bounds.bottom);
    
    // 1. 选择一个顶点，过该顶点做垂直线作为栅栏
    auto segments = shape->GetIntersectionSegments();
    if (segments.empty()) return fillPixels;
    
    // 选择第一个顶点作为栅栏位置
    int fenceX = static_cast<int>(segments[0].first.x);
    
    // 2. 将栅栏上的点直接加入填充结果
    // 因为栅栏点会在异或过程中被处理偶数次，导致没有标记
    for (int y = minY; y <= maxY; ++y) {
        D2D1_POINT_2F fencePoint = D2D1::Point2F(static_cast<float>(fenceX), static_cast<float>(y));
        if (IsPointInsideShape(shape, fencePoint)) {
            fillPixels.push_back(fencePoint);
        }
    }
    
    // 3. 使用标记数组，对每个像素进行取补标记
    // 使用map存储标记状态（true表示被标记，false表示未标记）
    std::map<std::pair<int, int>, bool> marks;
    
    // 4. 对每条与多边形相交的扫描线，将位于栅栏与边之间的像素取补
    for (int y = minY; y <= maxY; ++y) {
        // 对当前扫描线，找到所有与之相交的边
        for (const auto& segment : segments) {
            int xa = static_cast<int>(segment.first.x);
            int ya = static_cast<int>(segment.first.y);
            int xb = static_cast<int>(segment.second.x);
            int yb = static_cast<int>(segment.second.y);
            
            // 跳过水平边（与扫描线平行，不相交）
            if (ya == yb) {
                continue;
            }
            
            // 判断该边是否与当前扫描线相交（包含边的所有端点）
            int yMin = (std::min)(ya, yb);
            int yMax = (std::max)(ya, yb);
            if (y <= yMin || y > yMax) {
                continue; // 不相交
            }
            
            // 计算该边在当前扫描线上的交点X坐标
            int edgeX;
            if (xa == xb) {
                // 竖直边
                edgeX = xa;
            } else {
                // 斜边：使用线性插值计算交点
                edgeX = static_cast<int>(std::round(xa + double(y - ya) * (xb - xa) / (yb - ya)));
            }
            
            // 将栅栏与该边交点之间的扫描线段取补（异或操作），包含栅栏点和边界点
            int x1 = (std::min)(fenceX, edgeX);
            int x2 = (std::max)(fenceX, edgeX);
            
            for (int x = x1; x <= x2; ++x) {
                marks[{x, y}] = !marks[{x, y}];
            }
            
        }
    }
    
    // 5. 收集仍有标记的像素作为填充像素
    for (const auto& mark : marks) {
        if (mark.second) { // 仍有标记
            int x = mark.first.first;
            int y = mark.first.second;
            D2D1_POINT_2F fillPoint = D2D1::Point2F(static_cast<float>(x), static_cast<float>(y));
            
            // 验证该点在图形内部
            if (IsPointInsideShape(shape, fillPoint)) {
                fillPixels.push_back(fillPoint);
            }
        }
    }
    
    return fillPixels;
}

// 种子填充法（使用栈实现非递归）
std::vector<D2D1_POINT_2F> SeedFill(Shape* shape, D2D1_POINT_2F seedPoint) {
    std::vector<D2D1_POINT_2F> fillPixels;
    
    if (!shape) return fillPixels;
    
    // 检查种子点是否在图形内部
    if (!IsPointInsideShape(shape, seedPoint)) {
        return fillPixels;
    }
    
    D2D1_RECT_F bounds = shape->GetBounds();
    int minX = static_cast<int>(bounds.left);
    int maxX = static_cast<int>(bounds.right);
    int minY = static_cast<int>(bounds.top);
    int maxY = static_cast<int>(bounds.bottom);
    
    // 使用set来跟踪已填充的像素
    std::set<std::pair<int, int>> filled;
    std::stack<std::pair<int, int>> stack;
    
    int seedX = static_cast<int>(seedPoint.x);
    int seedY = static_cast<int>(seedPoint.y);
    
    stack.push({seedX, seedY});
    
    // 四连通填充
    int dx[] = {0, 0, 1, -1};
    int dy[] = {1, -1, 0, 0};
    
    while (!stack.empty() && fillPixels.size() < 100000) { // 限制填充数量防止无限循环
        std::pair<int, int> current = stack.top();
        stack.pop();
        int x = current.first;
        int y = current.second;
        
        // 检查是否已填充或越界
        if (x < minX || x > maxX || y < minY || y > maxY) continue;
        if (filled.count({x, y})) continue;
        
        D2D1_POINT_2F testPoint = D2D1::Point2F(static_cast<float>(x), static_cast<float>(y));
        if (!IsPointInsideShape(shape, testPoint)) continue;
        
        // 填充当前像素
        filled.insert({x, y});
        fillPixels.push_back(testPoint);
        
        // 将四个邻居加入栈
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            if (!filled.count({nx, ny})) {
                stack.push({nx, ny});
            }
        }
    }
    
    return fillPixels;
}

} // namespace FillAlgorithms
