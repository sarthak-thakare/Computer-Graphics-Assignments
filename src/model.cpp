#include "model.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include "sphere.hpp"
#include "box.hpp"
#include "cylinder.hpp"
#include "cone.hpp"
#include <GL/glew.h>
#include <functional>

model_t::model_t(){ root = std::make_unique<HNode>(); }
model_t::~model_t(){ clear(); }
void model_t::clear(){ root = std::make_unique<HNode>(); }

HNode* model_t::add_shape(std::unique_ptr<shape_t> s){
    auto node = std::make_unique<HNode>(std::move(s));
    HNode* ptr = node.get();
    root->children.push_back(std::move(node));
    return ptr;
}
void model_t::remove_last(){ if(!root->children.empty()) root->children.pop_back(); }

glm::vec3 model_t::compute_centroid() const {
    std::vector<glm::vec3> pts;
    std::function<void(const HNode*)> collect = [&](const HNode* n){
        if(!n) return;
        if(n->shape){
            for(auto &v: n->shape->vertices) pts.emplace_back(glm::vec3(v));
        }
        for(auto &c: n->children) collect(c.get());
    };
    collect(root.get());
    if(pts.empty()) return glm::vec3(0.0f);
    glm::vec3 s(0.0f);
    for(auto &p: pts) s += p;
    return s / float(pts.size());
}

bool model_t::save(const std::string &fname) const {
    std::string filepath = "models/" + fname;
    std::cout << "Attempting to save to: " << filepath << std::endl;
    std::ofstream of(filepath);
    if(!of) {
        std::cout << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    std::function<void(const HNode*, int)> ser = [&](const HNode* n, int depth){
        if(!n) return;
        for(int i=0;i<depth;i++) of << "  ";
        if(n->shape) of << n->shape->name() << " " << int(n->shape->level) << " ";
        else of << "none 0 ";
        // write color
        of << n->color.r << "," << n->color.g << "," << n->color.b << "," << n->color.a << " ";
        // write transforms: translate (vec3), rotate (mat4 flattened), scale (vec3)
        // For simplicity write translate vec3 and scale vec3 and skip full rotate storage (could be extended)
        glm::vec3 t = glm::vec3(n->translate[3]);
        glm::vec3 s = glm::vec3(n->scale[0][0], n->scale[1][1], n->scale[2][2]);
        of << t.x << "," << t.y << "," << t.z << " ";
        of << s.x << "," << s.y << "," << s.z << "\n";
        for(auto &c: n->children) ser(c.get(), depth+1);
    };
    ser(root.get(), 0);
    return true;
}

bool model_t::load(const std::string &fname){
    std::string filepath = "models/" + fname;
    std::cout << "Attempting to load from: " << filepath << std::endl;
    std::ifstream inf(filepath);
    if(!inf) {
        std::cout << "Failed to open file for reading: " << filepath << std::endl;
        return false;
    }
    clear();
    std::string line;
    std::vector<HNode*> stack;
    stack.push_back(root.get());
    while(std::getline(inf, line)){
        if(line.find_first_not_of(" \t\r\n")==std::string::npos) continue;
        // count indentation
        int depth = 0; while(depth < (int)line.size() && (line[depth]==' ' || line[depth]=='\t')) depth += 2;
        std::istringstream ss(line.substr(depth));
        std::string type; int lev; std::string colorstr; std::string trans, sc;
        ss >> type >> lev >> colorstr >> trans >> sc;
        std::unique_ptr<shape_t> s;
        if(type=="sphere") s = std::make_unique<sphere_t>(lev);
        else if(type=="box") s = std::make_unique<box_t>(lev);
        else if(type=="cylinder") s = std::make_unique<cylinder_t>(lev);
        else if(type=="cone") s = std::make_unique<cone_t>(lev);
        auto node = std::make_unique<HNode>(std::move(s));
        // parse color
        float r=1,g=1,b=1,a=1;
        sscanf(colorstr.c_str(), "%f,%f,%f,%f", &r,&g,&b,&a);
        node->color = glm::vec4(r,g,b,a);
        // parse translate and scale
        float tx=0,ty=0,tz=0; float sx=1,sy=1,sz=1;
        sscanf(trans.c_str(), "%f,%f,%f", &tx,&ty,&tz);
        sscanf(sc.c_str(), "%f,%f,%f", &sx,&sy,&sz);
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(tx,ty,tz));
        node->scale = glm::scale(glm::mat4(1.0f), glm::vec3(sx,sy,sz));
        // attach to stack at depth
        if(depth/2+1 > (int)stack.size()) stack.push_back(node.get());
        while((int)stack.size() > depth/2 + 1) stack.pop_back();
        stack.back()->children.push_back(std::move(node));
        stack.push_back(stack.back()->children.back().get());
    }
    return true;
}

void model_t::draw_recursive(HNode* node, const glm::mat4 &parent, GLuint mvpLoc) const {
    if(!node) return;
    glm::mat4 M = parent * node->translate * node->rotate * node->scale;
    glUniformMatrix4fv(mvpLoc,1,GL_FALSE,&M[0][0]);
    if(node->shape){
        // temporarily override colors if node->color not default white
        // we won't modify GPU buffers, instead we set a uniform? shaders don't have color uniform -> as workaround regenerate per-vertex colors is heavy
        // Instead, shape constructors set colors; to respect per-node color we'd need shader uniform multiply - but to keep simple we'll rely on per-vertex colors.
        node->shape->draw();
    }
    for(auto &c: node->children) draw_recursive(c.get(), M, mvpLoc);
}

void model_t::draw(GLuint mvpLoc, const glm::mat4 &viewProj) const {
    if(!root) return;
    draw_recursive(root.get(), viewProj, mvpLoc);
}
