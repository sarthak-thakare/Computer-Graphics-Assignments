#include "cylinder.hpp"
#include <glm/gtc/constants.hpp>
cylinder_t::cylinder_t(unsigned int lev, float r, float h): shape_t(lev), radius(r), height(h){
    shapetype = CYLINDER_SHAPE;
    vertices.clear(); colors.clear();
    int slices = 12 + 6*level;
    float halfh = height/2.0f;
    for(int i=0;i<slices;i++){
        float a1 = 2.0f * glm::pi<float>() * float(i)/slices;
        float a2 = 2.0f * glm::pi<float>() * float(i+1)/slices;
        glm::vec3 p1(radius*cos(a1), -halfh, radius*sin(a1));
        glm::vec3 p2(radius*cos(a2), -halfh, radius*sin(a2));
        glm::vec3 p3(radius*cos(a2), halfh, radius*sin(a2));
        glm::vec3 p4(radius*cos(a1), halfh, radius*sin(a1));
        vertices.push_back(glm::vec4(p1,1.0f)); vertices.push_back(glm::vec4(p2,1.0f)); vertices.push_back(glm::vec4(p3,1.0f));
        vertices.push_back(glm::vec4(p1,1.0f)); vertices.push_back(glm::vec4(p3,1.0f)); vertices.push_back(glm::vec4(p4,1.0f));
        for(int k=0;k<6;k++) colors.push_back(glm::vec4(0.2f,0.7f,0.3f,1.0f));
    }
    glm::vec3 centerB(0.0f,-halfh,0.0f), centerT(0.0f,halfh,0.0f);
    for(int i=0;i<slices;i++){
        float a1 = 2.0f * glm::pi<float>() * float(i)/slices;
        float a2 = 2.0f * glm::pi<float>() * float(i+1)/slices;
        glm::vec3 p1(radius*cos(a1), -halfh, radius*sin(a1));
        glm::vec3 p2(radius*cos(a2), -halfh, radius*sin(a2));
        vertices.push_back(glm::vec4(centerB,1.0f)); vertices.push_back(glm::vec4(p2,1.0f)); vertices.push_back(glm::vec4(p1,1.0f));
        for(int k=0;k<3;k++) colors.push_back(glm::vec4(0.2f,0.6f,0.9f,1.0f));
        glm::vec3 q1(radius*cos(a1), halfh, radius*sin(a1));
        glm::vec3 q2(radius*cos(a2), halfh, radius*sin(a2));
        vertices.push_back(glm::vec4(centerT,1.0f)); vertices.push_back(glm::vec4(q1,1.0f)); vertices.push_back(glm::vec4(q2,1.0f));
        for(int k=0;k<3;k++) colors.push_back(glm::vec4(0.2f,0.6f,0.9f,1.0f));
    }
    setup_buffers();
}
void cylinder_t::draw(){
    if(vao==0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());
    glBindVertexArray(0);
}
