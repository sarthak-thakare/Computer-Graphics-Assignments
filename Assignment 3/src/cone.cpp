#include "cone.hpp"
#include <glm/gtc/constants.hpp>
// Build cone side + base as triangle fans.
cone_t::cone_t(unsigned int lev, float r, float h): shape_t(lev), radius(r), height(h){
    shapetype = CONE_SHAPE;
    vertices.clear(); colors.clear();
    int slices = 12 + 6*level;
    glm::vec3 apex(0.0f, height/2.0f, 0.0f);
    glm::vec3 center(0.0f, -height/2.0f, 0.0f);
    for(int i=0;i<slices;i++){
        float a1 = 2.0f * glm::pi<float>() * float(i)/slices;
        float a2 = 2.0f * glm::pi<float>() * float(i+1)/slices;
        glm::vec3 p1(radius*cos(a1), -height/2.0f, radius*sin(a1));
        glm::vec3 p2(radius*cos(a2), -height/2.0f, radius*sin(a2));
        vertices.push_back(glm::vec4(apex,1.0f)); vertices.push_back(glm::vec4(p2,1.0f)); vertices.push_back(glm::vec4(p1,1.0f));
        for(int k=0;k<3;k++) colors.push_back(glm::vec4(0.9f,0.2f,0.2f,1.0f));
        vertices.push_back(glm::vec4(center,1.0f)); vertices.push_back(glm::vec4(p1,1.0f)); vertices.push_back(glm::vec4(p2,1.0f));
        for(int k=0;k<3;k++) colors.push_back(glm::vec4(0.3f,0.3f,0.3f,1.0f));
    }
    setup_buffers();
}
void cone_t::draw(){
    if(vao==0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());
    glBindVertexArray(0);
}
