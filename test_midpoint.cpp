#include <iostream>
#include <vector>
#include <cmath>

// 简化的点结构用于测试
struct Point2F {
    float x, y;
    Point2F(float x = 0, float y = 0) : x(x), y(y) {}
};

// 中点画线法算法实现
std::vector<Point2F> MidpointLineAlgorithm(Point2F start, Point2F end) {
    std::vector<Point2F> pixels;
    
    int x0 = static_cast<int>(start.x);
    int y0 = static_cast<int>(start.y);
    int x1 = static_cast<int>(end.x);
    int y1 = static_cast<int>(end.y);
    
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    
    if (dx > dy) {
        // 斜率 <= 1 的情况
        int d = 2 * dy - dx;
        int x = x0, y = y0;
        
        pixels.push_back(Point2F(static_cast<float>(x), static_cast<float>(y)));
        
        while (x != x1) {
            x += sx;
            if (d > 0) {
                y += sy;
                d += 2 * (dy - dx);
            } else {
                d += 2 * dy;
            }
            pixels.push_back(Point2F(static_cast<float>(x), static_cast<float>(y)));
        }
    } else {
        // 斜率 > 1 的情况
        int d = 2 * dx - dy;
        int x = x0, y = y0;
        
        pixels.push_back(Point2F(static_cast<float>(x), static_cast<float>(y)));
        
        while (y != y1) {
            y += sy;
            if (d > 0) {
                x += sx;
                d += 2 * (dx - dy);
            } else {
                d += 2 * dx;
            }
            pixels.push_back(Point2F(static_cast<float>(x), static_cast<float>(y)));
        }
    }
    
    return pixels;
}

int main() {
    // 测试几个不同的直线
    std::vector<std::pair<Point2F, Point2F>> testLines = {
        {Point2F(0, 0), Point2F(10, 5)},    // 斜率 < 1
        {Point2F(0, 0), Point2F(5, 10)},    // 斜率 > 1
        {Point2F(0, 0), Point2F(10, 0)},    // 水平线
        {Point2F(0, 0), Point2F(0, 10)},    // 垂直线
        {Point2F(0, 0), Point2F(10, 10)},   // 斜率 = 1
        {Point2F(10, 5), Point2F(0, 0)},    // 反向
    };
    
    for (size_t i = 0; i < testLines.size(); ++i) {
        auto& line = testLines[i];
        std::cout << "测试直线 " << (i + 1) << ": (" 
                  << line.first.x << "," << line.first.y << ") -> ("
                  << line.second.x << "," << line.second.y << ")" << std::endl;
        
        auto pixels = MidpointLineAlgorithm(line.first, line.second);
        
        std::cout << "生成的像素点: ";
        for (const auto& pixel : pixels) {
            std::cout << "(" << pixel.x << "," << pixel.y << ") ";
        }
        std::cout << std::endl << "总共 " << pixels.size() << " 个像素点" << std::endl << std::endl;
    }
    
    return 0;
}
