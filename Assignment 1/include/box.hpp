#pragma once
#include "shape.hpp"
class box_t : public shape_t {
public:
    glm::vec3 half;
    box_t(unsigned int lev=0, glm::vec3 half_extents = glm::vec3(0.5f));
    virtual void draw() override;
    virtual std::string name() const override { return "box"; }
};
