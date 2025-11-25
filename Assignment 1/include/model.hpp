#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory>
#include "shape.hpp"

struct HNode {
    std::unique_ptr<shape_t> shape;
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 rotate = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
    glm::vec4 color = glm::vec4(1.0f);
    std::vector<std::unique_ptr<HNode>> children;
    HNode() = default;
    HNode(std::unique_ptr<shape_t> s): shape(std::move(s)) {}
};

class model_t {
public:
    std::unique_ptr<HNode> root;
    model_t();
    ~model_t();
    void clear();
    HNode* add_shape(std::unique_ptr<shape_t> s);
    void remove_last();
    glm::vec3 compute_centroid() const;
    bool save(const std::string &fname) const;
    bool load(const std::string &fname);
    void draw_recursive(HNode* node, const glm::mat4 &parent, GLuint mvpLoc) const;
    void draw(GLuint mvpLoc, const glm::mat4 &viewProj) const;
};
