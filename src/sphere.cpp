#include "sphere.hpp"
#include <glm/gtc/constants.hpp>
// Spherical subdivision by stacks & slices with per-vertex normals and UVs.
sphere_t::sphere_t(unsigned int lev, float r): shape_t(lev), radius(r){
    shapetype = SPHERE_SHAPE;
    vertices.clear(); colors.clear(); normals.clear(); texcoords.clear();
    int stacks = 4 + 4*level;
    int slices = 8 + 8*level;
    for(int i=0;i<stacks;i++){
        float phi1 = glm::pi<float>() * float(i) / float(stacks);
        float phi2 = glm::pi<float>() * float(i+1) / float(stacks);
        for(int j=0;j<slices;j++){
            float theta1 = 2.0f * glm::pi<float>() * float(j) / float(slices);
            float theta2 = 2.0f * glm::pi<float>() * float(j+1) / float(slices);
            glm::vec3 p1 = glm::vec3(radius * sin(phi1)*cos(theta1), radius * cos(phi1), radius * sin(phi1)*sin(theta1));
            glm::vec3 p2 = glm::vec3(radius * sin(phi2)*cos(theta1), radius * cos(phi2), radius * sin(phi2)*sin(theta1));
            glm::vec3 p3 = glm::vec3(radius * sin(phi2)*cos(theta2), radius * cos(phi2), radius * sin(phi2)*sin(theta2));
            glm::vec3 p4 = glm::vec3(radius * sin(phi1)*cos(theta2), radius * cos(phi1), radius * sin(phi1)*sin(theta2));
            // UVs (spherical mapping)
            auto uvOf = [&](const glm::vec3 &p){
                float u = (atan2(p.z, p.x) + glm::pi<float>()) / (2.0f*glm::pi<float>());
                float v = (acos(p.y / radius)) / glm::pi<float>();
                return glm::vec2(u, v);
            };
            // tri1
            vertices.push_back(glm::vec4(p1,1.0f)); vertices.push_back(glm::vec4(p2,1.0f)); vertices.push_back(glm::vec4(p3,1.0f));
            normals.push_back(glm::normalize(p1)); normals.push_back(glm::normalize(p2)); normals.push_back(glm::normalize(p3));
            texcoords.push_back(uvOf(p1)); texcoords.push_back(uvOf(p2)); texcoords.push_back(uvOf(p3));
            // tri2
            vertices.push_back(glm::vec4(p1,1.0f)); vertices.push_back(glm::vec4(p3,1.0f)); vertices.push_back(glm::vec4(p4,1.0f));
            normals.push_back(glm::normalize(p1)); normals.push_back(glm::normalize(p3)); normals.push_back(glm::normalize(p4));
            texcoords.push_back(uvOf(p1)); texcoords.push_back(uvOf(p3)); texcoords.push_back(uvOf(p4));
            for(int k=0;k<6;k++) colors.push_back(glm::vec4(0.6f,0.4f,0.8f,1.0f));
        }
    }
    setup_buffers();
}
void sphere_t::draw(){
    if(vao==0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0,(GLsizei)vertices.size());
    glBindVertexArray(0);
}
