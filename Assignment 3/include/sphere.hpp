// -----------------------------------------------------------------------------
// sphere.hpp : UV-mapped sphere built from latitude/longitude stacks & slices.
// -----------------------------------------------------------------------------
#pragma once
#include "shape.hpp"
class sphere_t: public shape_t {
public:
    float radius;
    sphere_t(unsigned int lev=1, float r=0.5f);
    virtual void draw() override;
    virtual std::string name() const override { return "sphere"; }
};
