// -----------------------------------------------------------------------------
// cone.hpp : Simple cone (apex + base fan) colored per triangle.
// -----------------------------------------------------------------------------
#pragma once
#include "shape.hpp"
class cone_t: public shape_t {
public:
    float radius, height;
    cone_t(unsigned int lev=1, float r=0.4f, float h=1.0f);
    virtual void draw() override;
    virtual std::string name() const override { return "cone"; }
};
