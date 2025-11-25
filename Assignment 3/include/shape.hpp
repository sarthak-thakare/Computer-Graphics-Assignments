// -----------------------------------------------------------------------------
// shape.hpp
// Minimal base for drawable shapes (positions, colors, normals, uvs + GL buffers)
// Owns VAO/VBOs and provides a small helper to upload attribute arrays.
// NOTE: This file intentionally avoids any behavior changes—comments only.
// -----------------------------------------------------------------------------
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <string>


enum ShapeType { SPHERE_SHAPE, CYLINDER_SHAPE, BOX_SHAPE, CONE_SHAPE };

class shape_t {
public:
    ShapeType shapetype;
    unsigned int level;
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;   // per-vertex normals
    std::vector<glm::vec2> texcoords; // optional UVs

    // GL buffers: 0-pos,1-color,2-normal,3-uv
    GLuint vao=0;
    GLuint vbo[4]={0,0,0,0};

    // centroid for pivoting (computed from vertices)
    glm::vec3 centroid = glm::vec3(0.0f);

    // Constrain tessellation level to a small bound (defensive cap only)
    shape_t(unsigned int lev): level(lev) { if(level>4) level=4; }
    virtual ~shape_t(){
        if(vbo[0]) glDeleteBuffers(4,vbo);
        if(vao) glDeleteVertexArrays(1,&vao);
    }
    // Contract for subclasses:
    // - Fill vertices/colors/normals/texcoords in ctor, then call setup_buffers().
    // - draw() should bind VAO and issue a GL draw with correct primitive mode.
    virtual void draw() = 0;
    virtual std::string name() const = 0;

protected:
    // Compute simple arithmetic mean of points as a local pivot
    void compute_centroid(){
        if(vertices.empty()){ centroid = glm::vec3(0.0f); return; }
        glm::vec3 s(0.0f);
        for(auto &v: vertices) s += glm::vec3(v);
        centroid = s / float(vertices.size());
    }
    // Uploads attribute arrays (positions, colors, normals, uvs) and sets VAO state
    void setup_buffers(){
        compute_centroid();
        if(vertices.empty()) return;
        // ensure arrays sizes match
        if(colors.size() != vertices.size()) colors.assign(vertices.size(), glm::vec4(1.0f));
        if(normals.size() != vertices.size()) normals.assign(vertices.size(), glm::vec3(0,1,0));
        if(texcoords.size() != vertices.size()) texcoords.assign(vertices.size(), glm::vec2(0.0f));

        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);
        glGenBuffers(4,vbo);
        // positions
        glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec4), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        // colors
        glBindBuffer(GL_ARRAY_BUFFER,vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, colors.size()*sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        // normals
        glBindBuffer(GL_ARRAY_BUFFER,vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        // texcoords
        glBindBuffer(GL_ARRAY_BUFFER,vbo[3]);
        glBufferData(GL_ARRAY_BUFFER, texcoords.size()*sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindVertexArray(0);
    }
};
