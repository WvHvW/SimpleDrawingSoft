#pragma once
#include <d2d1.h>
#include <vector>
#include <memory>
#include "Shape.h"

// 填充算法命名空间
namespace FillAlgorithms {
    // 栅栏填充法（扫描线填充）
    std::vector<D2D1_POINT_2F> ScanlineFill(Shape* shape, D2D1_POINT_2F seedPoint);
    
    // 种子填充法
    std::vector<D2D1_POINT_2F> SeedFill(Shape* shape, D2D1_POINT_2F seedPoint);
}
