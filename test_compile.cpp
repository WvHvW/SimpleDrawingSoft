#include "Shape.h"
#include "CommonType.h"

// 测试编译
void test_compile() {
    // 测试枚举
    DrawingMode mode1 = DrawingMode::MIDPOINT_LINE;
    DrawingMode mode2 = DrawingMode::BRESENHAM_LINE;
    
    // 测试类实例化
    D2D1_POINT_2F start = {0, 0};
    D2D1_POINT_2F end = {10, 10};
    
    auto midpoint = std::make_shared<MidpointLine>(start, end);
    auto bresenham = std::make_shared<BresenhamLine>(start, end);
}
