#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include "model.hpp"
#include "sphere.hpp"
#include "box.hpp"
#include "cylinder.hpp"
#include "cone.hpp"

enum AppMode { MODELLER, INSPECT };
enum EditMode { EDIT_NONE, EDIT_ROTATE, EDIT_TRANSLATE, EDIT_SCALE };

struct AppState {
    AppMode appMode = MODELLER;
    EditMode editMode = EDIT_NONE;
    char axis = 'X';
    model_t scene;
    HNode* current = nullptr; // last added shape or selected node
    float step = 0.1f; // translation/rotation/scale step (rotation in radians)
} state;

std::string readFile(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    rewind(f);
    std::string s(sz, '\0'); 
    fread(&s[0], 1, sz, f);
    fclose(f);
    return s;
}


static GLuint compileShader(const char* path, GLenum type){
    std::string src = readFile(path);
    const char* c = src.c_str();
    GLuint sh = glCreateShader(type);
    glShaderSource(sh,1,&c,nullptr);
    glCompileShader(sh);
    GLint ok; glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(!ok){ char log[1024]; glGetShaderInfoLog(sh,1024,nullptr,log); std::cerr<<log<<std::endl; exit(1);}
    return sh;
}

static GLuint makeProgram(){
    GLuint vs = compileShader("shaders/basic.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("shaders/basic.frag", GL_FRAGMENT_SHADER);
    GLuint p = glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs); glLinkProgram(p);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ char log[1024]; glGetProgramInfoLog(p,1024,nullptr,log); std::cerr<<log<<std::endl; exit(1); }
    return p;
}

static void printMode(){
    std::cout << "Mode: " << (state.appMode==MODELLER? "MODELLER":"INSPECT") << ", Edit: ";
    if(state.editMode==EDIT_ROTATE) std::cout<<"Rotate ";
    else if(state.editMode==EDIT_TRANSLATE) std::cout<<"Translate ";
    else if(state.editMode==EDIT_SCALE) std::cout<<"Scale ";
    else std::cout<<"None ";
    std::cout << "Axis: " << state.axis << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(action!=GLFW_PRESS) return;
    if(key==GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window,1);
    if(key==GLFW_KEY_M){ state.appMode = MODELLER; std::cout<<"Switched to MODELLER\n"; return; }
    if(key==GLFW_KEY_I){ state.appMode = INSPECT; std::cout<<"Switched to INSPECT\n"; return; }

    if(state.appMode==MODELLER){
        if(key==GLFW_KEY_1){ unsigned lev=1; std::cout<<"Tess level (0..4): "; std::cin>>lev; if(lev>4) lev=4; auto s = std::make_unique<sphere_t>(lev); state.current = state.scene.add_shape(std::move(s)); std::cout<<"Added sphere\n"; return; }
        if(key==GLFW_KEY_2){ unsigned lev=0; std::cout<<"Tess level (0..4): "; std::cin>>lev; if(lev>4) lev=4; auto s = std::make_unique<box_t>(lev); state.current = state.scene.add_shape(std::move(s)); std::cout<<"Added box\n"; return; }
        if(key==GLFW_KEY_3){ unsigned lev=1; std::cout<<"Tess level (0..4): "; std::cin>>lev; if(lev>4) lev=4; auto s = std::make_unique<cylinder_t>(lev); state.current = state.scene.add_shape(std::move(s)); std::cout<<"Added cylinder\n"; return; }
        if(key==GLFW_KEY_4){ unsigned lev=1; std::cout<<"Tess level (0..4): "; std::cin>>lev; if(lev>4) lev=4; auto s = std::make_unique<cone_t>(lev); state.current = state.scene.add_shape(std::move(s)); std::cout<<"Added cone\n"; return; }
        if(key==GLFW_KEY_5){ state.scene.remove_last(); std::cout<<"Removed last shape\n"; state.current=nullptr; return; }
        if(key==GLFW_KEY_R){ state.editMode = EDIT_ROTATE; std::cout<<"Rotate mode\n"; return; }
        if(key==GLFW_KEY_T){ state.editMode = EDIT_TRANSLATE; std::cout<<"Translate mode\n"; return; }
        if(key==GLFW_KEY_G){ state.editMode = EDIT_SCALE; std::cout<<"Scale mode\n"; std::cout<<"Scale pivot is centroid\n"; return; }
    } else { // INSPECT mode
        if(key==GLFW_KEY_L){
            std::string fname; std::cout<<"Load .mod filename: "; std::cin>>fname;
            if(state.scene.load(fname)) std::cout<<"Loaded "<<fname<<"\n"; else std::cout<<"Load failed\n";
            return;
        }
        if(key==GLFW_KEY_R){ state.editMode = EDIT_ROTATE; std::cout<<"Rotate model\n"; return; }
    }

    // axis selection
    if(key==GLFW_KEY_X) { state.axis='X'; std::cout<<"Axis X\n"; return; }
    if(key==GLFW_KEY_Y) { state.axis='Y'; std::cout<<"Axis Y\n"; return; }
    if(key==GLFW_KEY_Z) { state.axis='Z'; std::cout<<"Axis Z\n"; return; }

    // plus/minus apply transformation
    if(key==GLFW_KEY_KP_ADD || key==GLFW_KEY_EQUAL){
        float amt = state.step;
        if(state.editMode==EDIT_ROTATE){
            float ang = amt; if(state.axis=='X') { if(state.appMode==MODELLER && state.current) state.current->rotate = glm::rotate(state.current->rotate, ang, glm::vec3(1,0,0)); else state.scene.root->rotate = glm::rotate(state.scene.root->rotate, ang, glm::vec3(1,0,0)); }
            if(state.axis=='Y') { if(state.appMode==MODELLER && state.current) state.current->rotate = glm::rotate(state.current->rotate, ang, glm::vec3(0,1,0)); else state.scene.root->rotate = glm::rotate(state.scene.root->rotate, ang, glm::vec3(0,1,0)); }
            if(state.axis=='Z') { if(state.appMode==MODELLER && state.current) state.current->rotate = glm::rotate(state.current->rotate, ang, glm::vec3(0,0,1)); else state.scene.root->rotate = glm::rotate(state.scene.root->rotate, ang, glm::vec3(0,0,1)); }
        } else if(state.editMode==EDIT_TRANSLATE){
            glm::vec3 t(0.0f);
            if(state.axis=='X') t.x += amt;
            if(state.axis=='Y') t.y += amt;
            if(state.axis=='Z') t.z += amt;
            if(state.appMode==MODELLER && state.current) state.current->translate = glm::translate(state.current->translate, t);
            else state.scene.root->translate = glm::translate(state.scene.root->translate, t);
        } else if(state.editMode==EDIT_SCALE){
            glm::vec3 s(1.0f);
            if(state.axis=='X') s.x += amt;
            if(state.axis=='Y') s.y += amt;
            if(state.axis=='Z') s.z += amt;
            if(state.appMode==MODELLER && state.current) state.current->scale = glm::scale(state.current->scale, s);
            else state.scene.root->scale = glm::scale(state.scene.root->scale, s);
        }
        return;
    }
    if(key==GLFW_KEY_KP_SUBTRACT || key==GLFW_KEY_MINUS){
        float amt = state.step;
        if(state.editMode==EDIT_ROTATE){
            float ang = -amt; if(state.axis=='X') { if(state.appMode==MODELLER && state.current) state.current->rotate = glm::rotate(state.current->rotate, ang, glm::vec3(1,0,0)); else state.scene.root->rotate = glm::rotate(state.scene.root->rotate, ang, glm::vec3(1,0,0)); }
            if(state.axis=='Y') { if(state.appMode==MODELLER && state.current) state.current->rotate = glm::rotate(state.current->rotate, ang, glm::vec3(0,1,0)); else state.scene.root->rotate = glm::rotate(state.scene.root->rotate, ang, glm::vec3(0,1,0)); }
            if(state.axis=='Z') { if(state.appMode==MODELLER && state.current) state.current->rotate = glm::rotate(state.current->rotate, ang, glm::vec3(0,0,1)); else state.scene.root->rotate = glm::rotate(state.scene.root->rotate, ang, glm::vec3(0,0,1)); }
        } else if(state.editMode==EDIT_TRANSLATE){
            glm::vec3 t(0.0f);
            if(state.axis=='X') t.x -= amt;
            if(state.axis=='Y') t.y -= amt;
            if(state.axis=='Z') t.z -= amt;
            if(state.appMode==MODELLER && state.current) state.current->translate = glm::translate(state.current->translate, t);
            else state.scene.root->translate = glm::translate(state.scene.root->translate, t);
        } else if(state.editMode==EDIT_SCALE){
            glm::vec3 s(1.0f);
            if(state.axis=='X') s.x -= amt;
            if(state.axis=='Y') s.y -= amt;
            if(state.axis=='Z') s.z -= amt;
            if(state.appMode==MODELLER && state.current) state.current->scale = glm::scale(state.current->scale, s);
            else state.scene.root->scale = glm::scale(state.scene.root->scale, s);
        }
        return;
    }

    if(key==GLFW_KEY_C){
        if(state.appMode==MODELLER && state.current){
            float r,g,b; std::cout<<"Enter R G B (0..1): "; std::cin>>r>>g>>b;
            state.current->color = glm::vec4(r,g,b,1.0f);
        } else std::cout<<"No current shape selected\n";
        return;
    }

    if(key==GLFW_KEY_S){
        std::string fname; std::cout<<"Save filename (.mod): "; std::cin>>fname;
        if(state.scene.save(fname)) std::cout<<"Saved "<<fname<<"\n"; else std::cout<<"Save failed\n";
        return;
    }

    printMode();
}

int main(){
    if(!glfwInit()){ std::cerr<<"GLFW init failed\n"; return -1; }
    GLFWwindow* win = glfwCreateWindow(1024,768,"Hierarchical Modeller",NULL,NULL);
    if(!win){ std::cerr<<"Window create failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glewExperimental = GL_TRUE; if(glewInit()!=GLEW_OK){ std::cerr<<"GLEW init failed\n"; return -1; }

    GLuint prog = makeProgram();
    glUseProgram(prog);
    GLuint mvpLoc = glGetUniformLocation(prog,"MVP");
    glEnable(GL_DEPTH_TEST);

    // create camera parameters
    float aspect = 1024.0f/768.0f;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    // initial example models: add 2 demo models in models/
    // create two sample models saved as .mod in models/
    state.scene.clear();
    // attach callbacks
    glfwSetKeyCallback(win, key_callback);

    // main loop
    while(!glfwWindowShouldClose(win)){
        glClearColor(0.2f,0.25f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // compute view based on centroid
        glm::vec3 cent = state.scene.compute_centroid();
        glm::vec3 campos = cent + glm::vec3(3.0f,2.0f,3.0f);
        glm::mat4 view = glm::lookAt(campos, cent, glm::vec3(0,1,0));
        glm::mat4 vp = proj * view;
        state.scene.draw(mvpLoc, vp);
        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
