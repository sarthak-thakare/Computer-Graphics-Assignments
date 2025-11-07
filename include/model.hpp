// -----------------------------------------------------------------------------
// model.hpp
// Hierarchical model (simple scene graph) composed of HNode objects.
// Each node can carry local TRS transforms, color, texture flag, and children.
// -----------------------------------------------------------------------------
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <memory>
#include "shape.hpp"

// A single node in the hierarchy. Children inherit cumulative transforms.
struct HNode {
    std::unique_ptr<shape_t> shape;
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 rotate = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
    glm::vec4 color = glm::vec4(1.0f);
    // Texture support
    unsigned int texture = 0; // OpenGL texture id (0 means none)
    bool useTexture = false;  // whether to sample texture in shader
    std::vector<std::unique_ptr<HNode>> children;
    HNode() = default;
    HNode(std::unique_ptr<shape_t> s): shape(std::move(s)) {}
};

class model_t {
public:
    std::unique_ptr<HNode> root;
    model_t();
    ~model_t();
    // disallow copying
    model_t(const model_t&) = delete;
    model_t& operator=(const model_t&) = delete;
    // allow moving
    model_t(model_t&&) noexcept = default;
    model_t& operator=(model_t&&) noexcept = default;

    void clear();
    HNode* add_shape(std::unique_ptr<shape_t> s);
    void remove_last();
    glm::vec3 compute_centroid() const;
    bool save(const std::string &fname) const;
    bool load(const std::string &fname);

    // Render using Gouraud: needs MVP and Model matrix
    // Depth-first traversal: builds MVP/model matrices and draws each node
    void draw_recursive(HNode* node, const glm::mat4 &parentVP, const glm::mat4 &parentWorld, GLuint mvpLoc, GLuint modelLoc, GLint useTexLoc) const;
    void draw(GLuint mvpLoc, GLuint modelLoc, const glm::mat4 &viewProj, GLint useTexLoc) const;

    // Utility: compute world transform (frame without scale) of a given node
    bool get_world_frame_of(const HNode* target, glm::mat4 &outWorld) const;
private:
    bool get_world_frame_of_rec(const HNode* node, const HNode* target, const glm::mat4 &parentWorld, glm::mat4 &outWorld) const;
};
