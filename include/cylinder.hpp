#pragma once
#include "shape.hpp"
class cylinder_t: public shape_t {
public:
    float radius, height;
    cylinder_t(unsigned int lev=1, float r=0.4f, float h=1.0f);
    virtual void draw() override;
    virtual std::string name() const override { return "cylinder"; }
};
