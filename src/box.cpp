#include "box.hpp"
// Build a triangulated box (two triangles per face) with simple UVs and colors.
box_t::box_t(unsigned int lev, glm::vec3 half_extents): shape_t(lev), half(half_extents){
    shapetype = BOX_SHAPE;
    vertices.clear(); colors.clear(); normals.clear(); texcoords.clear();
    glm::vec3 h = half;
    glm::vec4 v[8] = {
        glm::vec4(-h.x,-h.y,-h.z,1), glm::vec4(h.x,-h.y,-h.z,1), glm::vec4(h.x,h.y,-h.z,1), glm::vec4(-h.x,h.y,-h.z,1),
        glm::vec4(-h.x,-h.y,h.z,1),  glm::vec4(h.x,-h.y,h.z,1),  glm::vec4(h.x,h.y,h.z,1),  glm::vec4(-h.x,h.y,h.z,1)
    };
    int idx[] = {0,1,2, 0,2,3, 1,5,6, 1,6,2, 5,4,7, 5,7,6, 4,0,3, 4,3,7, 3,2,6, 3,6,7, 4,5,1, 4,1,0};
    glm::vec3 nrm[6] = {
        glm::vec3(0,0,-1), glm::vec3(1,0,0), glm::vec3(0,0,1), glm::vec3(-1,0,0), glm::vec3(0,1,0), glm::vec3(0,-1,0)
    };
    // Simple per-face UVs (0..1 box)
    auto pushTri = [&](int a, int b, int c, const glm::vec3 &n){
        vertices.push_back(v[a]); vertices.push_back(v[b]); vertices.push_back(v[c]);
        normals.push_back(n); normals.push_back(n); normals.push_back(n);
        colors.push_back(glm::vec4(0.9f,0.6f,0.3f,1.0f));
        colors.push_back(glm::vec4(0.9f,0.6f,0.3f,1.0f));
        colors.push_back(glm::vec4(0.9f,0.6f,0.3f,1.0f));
        texcoords.push_back(glm::vec2(0,0)); texcoords.push_back(glm::vec2(1,0)); texcoords.push_back(glm::vec2(1,1));
    };
    auto pushTri2 = [&](int a, int b, int c, const glm::vec3 &n){
        vertices.push_back(v[a]); vertices.push_back(v[b]); vertices.push_back(v[c]);
        normals.push_back(n); normals.push_back(n); normals.push_back(n);
        colors.push_back(glm::vec4(0.9f,0.6f,0.3f,1.0f));
        colors.push_back(glm::vec4(0.9f,0.6f,0.3f,1.0f));
        colors.push_back(glm::vec4(0.9f,0.6f,0.3f,1.0f));
        texcoords.push_back(glm::vec2(0,0)); texcoords.push_back(glm::vec2(1,1)); texcoords.push_back(glm::vec2(0,1));
    };
    // Expand manually to use push helpers per face
    // -Z face
    pushTri(0,1,2,nrm[0]); pushTri2(0,2,3,nrm[0]);
    // +X face
    pushTri(1,5,6,nrm[1]); pushTri2(1,6,2,nrm[1]);
    // +Z face
    pushTri(5,4,7,nrm[2]); pushTri2(5,7,6,nrm[2]);
    // -X face
    pushTri(4,0,3,nrm[3]); pushTri2(4,3,7,nrm[3]);
    // +Y face
    pushTri(3,2,6,nrm[4]); pushTri2(3,6,7,nrm[4]);
    // -Y face
    pushTri(4,5,1,nrm[5]); pushTri2(4,1,0,nrm[5]);
    setup_buffers();
}
void box_t::draw(){
    if(vao==0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0, (GLsizei) vertices.size());
    glBindVertexArray(0);
}
