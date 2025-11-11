#pragma once
#include "Shape.h"
#include <vector>
#include <memory>

class IntersectionManager {
public:
    static IntersectionManager &getInstance();

    bool selectShape(std::shared_ptr<Shape> shape);
    std::vector<D2D1_POINT_2F> calculateIntersection();
    const std::vector<D2D1_POINT_2F> &getIntersectionPoints() const;
    void clear();
    bool hasTwoShapes() const;
    std::shared_ptr<Shape> getFirstShape() const {
        return shape1;
    }
    std::shared_ptr<Shape> getSecondShape() const {
        return shape2;
    }

private:
    IntersectionManager() = default;
    std::shared_ptr<Shape> shape1;
    std::shared_ptr<Shape> shape2;
    std::vector<D2D1_POINT_2F> intersectionPoints;

    std::vector<D2D1_POINT_2F> calculateIntersectionImpl();
};