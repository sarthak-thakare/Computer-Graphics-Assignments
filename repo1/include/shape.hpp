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

    // GL buffers
    GLuint vao=0;
    GLuint vbo[2]={0,0};

    // centroid for pivoting (computed from vertices)
    glm::vec3 centroid = glm::vec3(0.0f);

    shape_t(unsigned int lev): level(lev) { if(level>4) level=4; }
    virtual ~shape_t(){
        if(vbo[0]) glDeleteBuffers(2,vbo);
        if(vao) glDeleteVertexArrays(1,&vao);
    }
    // fill vertices/colors in constructor and then call setup_buffers()
    virtual void draw() = 0;
    virtual std::string name() const = 0;

protected:
    void compute_centroid(){
        if(vertices.empty()){ centroid = glm::vec3(0.0f); return; }
        glm::vec3 s(0.0f);
        for(auto &v: vertices) s += glm::vec3(v);
        centroid = s / float(vertices.size());
    }
    void setup_buffers(){
        compute_centroid();
        if(vertices.empty()) return;
        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);
        glGenBuffers(2,vbo);
        glBindBuffer(GL_ARRAY_BUFFER,vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec4), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glBindBuffer(GL_ARRAY_BUFFER,vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, colors.size()*sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,0,(void*)0);
        glBindVertexArray(0);
    }
};
