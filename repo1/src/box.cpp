#include "box.hpp"
box_t::box_t(unsigned int lev, glm::vec3 half_extents): shape_t(lev), half(half_extents){
    shapetype = BOX_SHAPE;
    vertices.clear(); colors.clear();
    glm::vec3 h = half;
    glm::vec4 v[8] = {
        glm::vec4(-h.x,-h.y,-h.z,1), glm::vec4(h.x,-h.y,-h.z,1), glm::vec4(h.x,h.y,-h.z,1), glm::vec4(-h.x,h.y,-h.z,1),
        glm::vec4(-h.x,-h.y,h.z,1), glm::vec4(h.x,-h.y,h.z,1), glm::vec4(h.x,h.y,h.z,1), glm::vec4(-h.x,h.y,h.z,1)
    };
    int idx[] = {0,1,2, 0,2,3, 1,5,6, 1,6,2, 5,4,7, 5,7,6, 4,0,3, 4,3,7, 3,2,6, 3,6,7, 4,5,1, 4,1,0};
    for(int i=0;i<36;i+=3){
        vertices.push_back(v[idx[i+0]]);
        vertices.push_back(v[idx[i+1]]);
        vertices.push_back(v[idx[i+2]]);
        for(int k=0;k<3;k++) colors.push_back(glm::vec4(0.9f,0.6f,0.3f,1.0f));
    }
    setup_buffers();
}
void box_t::draw(){
    if(vao==0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0, (GLsizei) vertices.size());
    glBindVertexArray(0);
}
