// -----------------------------------------------------------------------------
// line_strip.hpp : Utility shape for visualizing paths (Bezier, control polygon).
// Stores vertices and colors; normals are dummy upward vectors.
// -----------------------------------------------------------------------------
#pragma once
#include "shape.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <GL/glew.h>


class line_strip_t : public shape_t {
private:
    std::vector<glm::vec2> uvs; // For compatibility
    GLenum gl_draw_mode;        // To store GL_LINE_STRIP
    
    
    //Creates the VBOs and VAO for this line strip.
    void init_vbo() {
        // Based on the 'vbo' array used in your main.cpp
        glGenBuffers(4, vbo); // vbo[0-3]
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // VBO 0: Vertices
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec4), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // VBO 1: Colors
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // VBO 2: Normals (dummy data)
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    // Use vec3 normals to match vertex shader (location=2 expects vec3)
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        
        // No index buffer (vbo[3]) needed for GL_LINE_STRIP
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

public:
    line_strip_t(const std::vector<glm::vec3>& points, const glm::vec4& color) : shape_t(0) {
        
        gl_draw_mode = GL_LINE_STRIP;
        
        // Populate vertex data 
        vertices.reserve(points.size());
        colors.reserve(points.size());
        normals.reserve(points.size());
        uvs.reserve(points.size());    

        for (const auto& p : points) {
            vertices.push_back(glm::vec4(p, 1.0f));    // position
            colors.push_back(color);                   // uniform color for strip
            normals.push_back(glm::vec3(0, 1, 0));     // dummy upward normal
            uvs.push_back(glm::vec2(0, 0));            // unused placeholder
        }
        
        // Call our own init function
        init_vbo(); 
    }

    virtual void draw() override {
        // vao and vertices are inherited from shape_t
        glBindVertexArray(vao);
        glDrawArrays(gl_draw_mode, 0, (GLsizei)vertices.size());
        glBindVertexArray(0);
    }

    //Return the name of the shape
    virtual std::string name() const override {
        return "line_strip";
    }
};