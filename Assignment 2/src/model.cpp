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

        // indentation for hierarchy
        for(int i=0;i<depth;i++) of << "  ";

        // shape type and tessellation
        if(n->shape) of << n->shape->name() << " " << int(n->shape->level) << " ";
        else of << "none 0 ";

        // color - use the actual color from the node, or from shape's first vertex if available
        glm::vec4 colorToSave = n->color;
        if(n->shape && !n->shape->colors.empty()) {
            // Use the shape's actual color (they're all the same per shape)
            colorToSave = n->shape->colors[0];
        }
        of << colorToSave.r << "," << colorToSave.g << "," << colorToSave.b << "," << colorToSave.a << " ";

        // translation (vec3)
        glm::vec3 t = glm::vec3(n->translate[3]);
        of << t.x << "," << t.y << "," << t.z << " ";

        // scale (vec3)
        glm::vec3 s = glm::vec3(n->scale[0][0], n->scale[1][1], n->scale[2][2]);
        of << s.x << "," << s.y << "," << s.z << " ";

        // rotation (4x4 matrix flattened row-major)
        for(int i=0;i<4;i++) {
            for(int j=0;j<4;j++) {
                of << n->rotate[i][j];
                if(!(i==3 && j==3)) of << ",";
            }
        }
        of << "\n";

        // recurse children
        for(auto &c: n->children) ser(c.get(), depth+1);
    };

    ser(root.get(), 0);
    return true;
}

bool model_t::load(const std::string &fname) {
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

    while(std::getline(inf, line)) {
        if(line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        // count indentation (2 spaces per depth)
        int depth = 0;
        while(depth < (int)line.size() && (line[depth] == ' ' || line[depth] == '\t'))
            depth += 2;

        std::istringstream ss(line.substr(depth));
        std::string type; int lev;
        std::string colorstr, trans, sc, rots;

        // ss >> type >> lev >> colorstr >> trans >> sc >> rots;
        ss >> type >> lev >> colorstr >> trans >> sc;
        std::getline(ss, rots);
        rots.erase(0, rots.find_first_not_of(" \t"));

        // create shape
        std::unique_ptr<shape_t> s;
        if(type=="sphere")   s = std::make_unique<sphere_t>(lev);
        else if(type=="box") s = std::make_unique<box_t>(lev);
        else if(type=="cylinder") s = std::make_unique<cylinder_t>(lev);
        else if(type=="cone")     s = std::make_unique<cone_t>(lev);

        auto node = std::make_unique<HNode>(std::move(s));

        // parse color
        float r=1,g=1,b=1,a=1;
        sscanf(colorstr.c_str(), "%f,%f,%f,%f", &r,&g,&b,&a);
        node->color = glm::vec4(r,g,b,a);

        if(node->shape){
            auto &cols = node->shape->colors;
            for(auto &c : cols){
                c = node->color;
            }
            glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]);
            glBufferSubData(GL_ARRAY_BUFFER, 0,
                            cols.size() * sizeof(glm::vec4), cols.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }


        // parse translate and scale
        float tx=0,ty=0,tz=0;
        float sx=1,sy=1,sz=1;
        sscanf(trans.c_str(), "%f,%f,%f", &tx,&ty,&tz);
        sscanf(sc.c_str(), "%f,%f,%f", &sx,&sy,&sz);
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(tx,ty,tz));
        node->scale     = glm::scale(glm::mat4(1.0f), glm::vec3(sx,sy,sz));

        // parse rotation (16 floats from comma-separated string)
        glm::mat4 R(1.0f);
        {
            std::replace(rots.begin(), rots.end(), ',', ' ');
            std::istringstream rs(rots);
            for(int i=0;i<4;i++)
                for(int j=0;j<4;j++)
                    rs >> R[i][j];
        }
        node->rotate = R;

        // attach to tree
        if(depth/2+1 > (int)stack.size()) stack.push_back(node.get());
        while((int)stack.size() > depth/2 + 1) stack.pop_back();

        stack.back()->children.push_back(std::move(node));
        stack.push_back(stack.back()->children.back().get());
    }

    return true;
}

void model_t::draw_recursive(HNode* node, const glm::mat4 &parentVP, const glm::mat4 &parentWorld, GLuint mvpLoc, GLuint modelLoc, GLint useTexLoc) const {
    if(!node) return;
    glm::mat4 worldFrame = parentWorld * node->translate * node->rotate; // no scale for frame
    glm::mat4 M = worldFrame * node->scale; // apply scale to geometry
    // Fix: MVP must be computed with the full Model matrix so parent transforms apply
    glm::mat4 MVP = parentVP * M;

    glUniformMatrix4fv(mvpLoc,1,GL_FALSE,&MVP[0][0]);
    glUniformMatrix4fv(modelLoc,1,GL_FALSE,&M[0][0]);

    // set texturing flag
    if(useTexLoc >= 0){ glUniform1i(useTexLoc, node->useTexture ? 1 : 0); }
    if(node->useTexture && node->texture!=0){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, node->texture);
    }

    if(node->shape){ node->shape->draw(); }
    for(auto &c: node->children) draw_recursive(c.get(), parentVP, worldFrame, mvpLoc, modelLoc, useTexLoc);
}

void model_t::draw(GLuint mvpLoc, GLuint modelLoc, const glm::mat4 &viewProj, GLint useTexLoc) const {
    if(!root) return;
    draw_recursive(root.get(), viewProj, glm::mat4(1.0f), mvpLoc, modelLoc, useTexLoc);
}

bool model_t::get_world_frame_of_rec(const HNode* node, const HNode* target, const glm::mat4 &parentWorld, glm::mat4 &outWorld) const{
    if(!node) return false;
    glm::mat4 world = parentWorld * node->translate * node->rotate;
    if(node==target){ outWorld = world; return true; }
    for(auto &c: node->children){ if(get_world_frame_of_rec(c.get(), target, world, outWorld)) return true; }
    return false;
}

bool model_t::get_world_frame_of(const HNode* target, glm::mat4 &outWorld) const{
    return get_world_frame_of_rec(root.get(), target, glm::mat4(1.0f), outWorld);
}
