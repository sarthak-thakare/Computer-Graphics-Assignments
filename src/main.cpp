#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <cstdio>
#include "model.hpp"
#include "sphere.hpp"
#include "box.hpp"
#include "cylinder.hpp"
#include "cone.hpp"
#include "robot_arm.hpp"
#include <limits>
#include <cmath>

// Forward declarations
static std::string readFile(const char* path);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Helpers to compute world-space AABB for a model
static void compute_aabb_node(const HNode* node, const glm::mat4 &parentWorld,
                              glm::vec3 &minv, glm::vec3 &maxv){
    if(!node) return;
    glm::mat4 world = parentWorld * node->translate * node->rotate;
    glm::mat4 M = world * node->scale;
    if(node->shape){
        for(const auto &v4 : node->shape->vertices){
            glm::vec3 p = glm::vec3(M * v4);
            minv = glm::min(minv, p);
            maxv = glm::max(maxv, p);
        }
    }
    for(const auto &c : node->children) compute_aabb_node(c.get(), world, minv, maxv);
}
static bool compute_aabb(const model_t &m, const glm::mat4 &world, glm::vec3 &minv, glm::vec3 &maxv){
    if(!m.root) return false;
    minv = glm::vec3(std::numeric_limits<float>::infinity());
    maxv = glm::vec3(-std::numeric_limits<float>::infinity());
    compute_aabb_node(m.root.get(), world, minv, maxv);
    auto finite3 = [](const glm::vec3 &v){ return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); };
    if(!finite3(minv) || !finite3(maxv)) return false;
    return true;
}

enum AppMode { MODELLER, INSPECT, ROBOT };
enum EditMode { EDIT_NONE, EDIT_ROTATE, EDIT_TRANSLATE, EDIT_SCALE };
enum CameraMode { CAM_SCENE = 1, CAM_FOLLOW = 2 };

struct Lights {
    bool l0On = true;
    bool l1On = true;
    bool toyOn = true;
    glm::vec3 l0Pos = glm::vec3(-3, 3, 3);
    glm::vec3 l1Pos = glm::vec3( 3, 3,-3);
    glm::vec3 l0Col = glm::vec3(1.0, 0.95, 0.9);
    glm::vec3 l1Col = glm::vec3(0.9, 0.95, 1.0);
    glm::vec3 toyCol= glm::vec3(1.0, 0.9, 0.7);
} lights;

struct AppState {
    AppMode appMode = ROBOT;  // Start in robot mode
    EditMode editMode = EDIT_NONE;
    char axis = 'X';
    model_t scene;
    RobotArm robot;  // The robot arm toy
    CameraMode camMode = CAM_SCENE;
    // textures
    GLuint texFloor=0, texWall=0, texPlatform=0, texMetal10=0, texWooden=0;
    GLint useTexLoc=-1, samplerLoc=-1;
    // Additional models placed around the robot
    model_t humanModel;
    model_t carModel;
    glm::mat4 humanWorld = glm::mat4(1.0f);
    glm::mat4 carWorld   = glm::mat4(1.0f);
} state;

// simple BMP loader (24-bit uncompressed)
static bool loadBMP(const char* path, int &w, int &h, std::vector<unsigned char> &data){
    FILE* f=fopen(path,"rb"); if(!f) return false;
    unsigned char header[54]; fread(header,1,54,f);
    if(header[0]!='B'||header[1]!='M'){ fclose(f); return false; }
    unsigned int dataPos = *(int*)&(header[0x0A]);
    unsigned int imageSize = *(int*)&(header[0x22]);
    w = *(int*)&(header[0x12]); h = *(int*)&(header[0x16]);
    if(imageSize==0) imageSize = w*h*3; if(dataPos==0) dataPos = 54;
    data.resize(imageSize);
    fseek(f,dataPos,SEEK_SET); fread(data.data(),1,imageSize,f); fclose(f);
    // BMP is BGR; flip to RGB
    for(unsigned i=0;i<imageSize;i+=3) std::swap(data[i], data[i+2]);
    return true;
}

static GLuint makeTexture(const char* path){
    int w=0,h=0; std::vector<unsigned char> d; if(!loadBMP(path,w,h,d)) { std::cerr<<"Failed to load tex: "<<path<<"\n"; return 0; }
    GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,d.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0);
    return t;
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
    std::cout << "Mode: ";
    if(state.appMode==MODELLER) std::cout << "MODELLER";
    else if(state.appMode==INSPECT) std::cout << "INSPECT";
    else if(state.appMode==ROBOT) std::cout << "ROBOT";
    std::cout << ", Edit: ";
    if(state.editMode==EDIT_ROTATE) std::cout<<"Rotate ";
    else if(state.editMode==EDIT_TRANSLATE) std::cout<<"Translate ";
    else if(state.editMode==EDIT_SCALE) std::cout<<"Scale ";
    else std::cout<<"None ";
    std::cout << "Axis: " << state.axis << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(action!=GLFW_PRESS) return;
    if(key==GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window,1); return; }

    // Camera toggle
    if(key==GLFW_KEY_V){ state.camMode = (state.camMode==CAM_SCENE? CAM_FOLLOW: CAM_SCENE); std::cout<<"Camera: "<<(state.camMode==CAM_SCENE?"Scene":"Follow")<<"\n"; return; }

    // Light toggles
    if(key==GLFW_KEY_8){ lights.l0On = !lights.l0On; std::cout<<"Light 0: "<<(lights.l0On?"On":"Off")<<"\n"; return; }
    if(key==GLFW_KEY_9){ lights.l1On = !lights.l1On; std::cout<<"Light 1: "<<(lights.l1On?"On":"Off")<<"\n"; return; }
    if(key==GLFW_KEY_0){ lights.toyOn = !lights.toyOn; std::cout<<"Toy Light: "<<(lights.toyOn?"On":"Off")<<"\n"; return; }

    // ROBOT MODE - Control robot arm joints
    float angleStep = 0.1f;
    float gripStep = 0.02f;

    // Lower Arm Controls (2 DOF) - Arrow Keys
    if(key==GLFW_KEY_UP)    { state.robot.lowerArmRotX += angleStep; state.robot.updateJoints(); std::cout<<"Lower arm pitch up\n"; return; }
    if(key==GLFW_KEY_DOWN)  { state.robot.lowerArmRotX -= angleStep; state.robot.updateJoints(); std::cout<<"Lower arm pitch down\n"; return; }
    if(key==GLFW_KEY_LEFT)  { state.robot.lowerArmRotY += angleStep; state.robot.updateJoints(); std::cout<<"Lower arm rotate left\n"; return; }
    if(key==GLFW_KEY_RIGHT) { state.robot.lowerArmRotY -= angleStep; state.robot.updateJoints(); std::cout<<"Lower arm rotate right\n"; return; }

    // Upper Arm Controls (2 DOF) - WASD
    if(key==GLFW_KEY_W) { state.robot.upperArmRotX += angleStep; state.robot.updateJoints(); std::cout<<"Upper arm pitch up\n"; return; }
    if(key==GLFW_KEY_S) { state.robot.upperArmRotX -= angleStep; state.robot.updateJoints(); std::cout<<"Upper arm pitch down\n"; return; }
    if(key==GLFW_KEY_A) { state.robot.upperArmRotY += angleStep; state.robot.updateJoints(); std::cout<<"Upper arm rotate left\n"; return; }
    if(key==GLFW_KEY_D) { state.robot.upperArmRotY -= angleStep; state.robot.updateJoints(); std::cout<<"Upper arm rotate right\n"; return; }

    // Hand Controls (3 DOF) - Q/E, Z/C, 1/2
    if(key==GLFW_KEY_Q) { state.robot.handRotX += angleStep; state.robot.updateJoints(); std::cout<<"Hand pitch up\n"; return; }
    if(key==GLFW_KEY_E) { state.robot.handRotX -= angleStep; state.robot.updateJoints(); std::cout<<"Hand pitch down\n"; return; }
    if(key==GLFW_KEY_Z) { state.robot.handRotY += angleStep; state.robot.updateJoints(); std::cout<<"Hand yaw left\n"; return; }
    if(key==GLFW_KEY_C) { state.robot.handRotY -= angleStep; state.robot.updateJoints(); std::cout<<"Hand yaw right\n"; return; }
    if(key==GLFW_KEY_1) { state.robot.handRotZ += angleStep; state.robot.updateJoints(); std::cout<<"Hand roll CCW\n"; return; }
    if(key==GLFW_KEY_2) { state.robot.handRotZ -= angleStep; state.robot.updateJoints(); std::cout<<"Hand roll CW\n"; return; }

    // Gripper Controls (Open/Close) - O/L
    if(key==GLFW_KEY_O) { state.robot.gripperOpen += gripStep; if(state.robot.gripperOpen > 1.0f) state.robot.gripperOpen = 1.0f; state.robot.updateJoints(); std::cout<<"Gripper open\n"; return; }
    if(key==GLFW_KEY_L) { state.robot.gripperOpen -= gripStep; if(state.robot.gripperOpen < 0.0f) state.robot.gripperOpen = 0.0f; state.robot.updateJoints(); std::cout<<"Gripper close\n"; return; }

    // Help
    if(key==GLFW_KEY_H) {
        std::cout << "\n=== ROBOT ARM CONTROLS ===\n";
        std::cout << "Arrow Keys: Lower arm (2 DOF) - Up/Down (pitch), Left/Right (rotate)\n";
        std::cout << "WASD: Upper arm (2 DOF) - W/S (pitch), A/D (rotate)\n";
        std::cout << "Q/E: Hand pitch (around X)\n";
        std::cout << "Z/C: Hand yaw (around Y)\n";
        std::cout << "1/2: Hand roll (around Z)\n";
        std::cout << "O/L: Open/Close gripper\n";
        std::cout << "8/9: Toggle Scene Lights, 0: Toggle Toy Light\n";
        std::cout << "V: Toggle Camera (Scene/Follow)\n\n";
        return;
    }
}

static void build_scene(){
    state.scene.clear();
    // Load textures
    state.texFloor    = makeTexture("images/wood.bmp");
    state.texWall     = makeTexture("images/bricks.bmp");
    state.texPlatform = makeTexture("images/metal.bmp");
    state.texMetal10  = makeTexture("images/metal10.bmp");
    state.texWooden   = makeTexture("images/wooden.bmp");

    // Floor: top at y=0 (room reference)
    {
        auto b = std::make_unique<box_t>(0, glm::vec3(12,0.1f,12));
        HNode* n = state.scene.add_shape(std::move(b));
        n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));
        n->color = glm::vec4(1.0f);
        if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        n->texture = state.texFloor; n->useTexture = (n->texture!=0);
    }
    // Back wall (-Z)
    {
        auto b = std::make_unique<box_t>(0, glm::vec3(12,5,0.05f));
        HNode* n = state.scene.add_shape(std::move(b));
        n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, -12.0f));
        n->color = glm::vec4(1.0f);
        if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        n->texture = state.texWall; n->useTexture = (n->texture!=0);
    }
    // Front wall (+Z)
    {
        auto b = std::make_unique<box_t>(0, glm::vec3(12,5,0.05f));
        HNode* n = state.scene.add_shape(std::move(b));
        n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, 12.0f));
        n->color = glm::vec4(1.0f);
        if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        n->texture = state.texWall; n->useTexture = (n->texture!=0);
    }
    // Right wall (+X)
    {
        auto b = std::make_unique<box_t>(0, glm::vec3(0.05f,5,12));
        HNode* n = state.scene.add_shape(std::move(b));
        n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f, 2.5f, 0.0f));
        n->color = glm::vec4(1.0f);
        if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        n->texture = state.texWall; n->useTexture = (n->texture!=0);
    }
    // Left wall (-X)
    {
        auto b = std::make_unique<box_t>(0, glm::vec3(0.05f,5,12));
        HNode* n = state.scene.add_shape(std::move(b));
        n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(-12.0f, 2.5f, 0.0f));
        n->color = glm::vec4(1.0f);
        if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        n->texture = state.texWall; n->useTexture = (n->texture!=0);
    }
    // Ceiling (brick)
    {
        auto b = std::make_unique<box_t>(0, glm::vec3(12,0.05f,12));
        HNode* n = state.scene.add_shape(std::move(b));
        n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f));
        n->color = glm::vec4(1.0f);
        if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        n->texture = state.texWall; n->useTexture = (n->texture!=0);
    }
    // Center platform for robot (metal) top at y=0.2
    {
        auto b = std::make_unique<box_t>(0, glm::vec3(1.2f, 0.1f, 1.2f));
        HNode* n = state.scene.add_shape(std::move(b));
        n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.1f, 0.0f));
        n->color = glm::vec4(1.0f);
        if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        n->texture = state.texPlatform; n->useTexture = (n->texture!=0);
    }

    // Simple wooden table behind the robot (at z = -6)
    {
        auto tableGroup = std::make_unique<HNode>();
        tableGroup->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -6.0f));
        HNode* tbl = tableGroup.get();

        // Table dimensions
        const float topHalfX = 0.9f, topHalfY = 0.04f, topHalfZ = 0.6f; // top thickness 0.08
        const float legHalfX = 0.05f, legHalfY = 0.36f, legHalfZ = 0.05f; // leg height 0.72, total table height ~0.8
        const float legY = legHalfY; // centers legs so bottoms at y=0
        const float topY = 2*legHalfY + topHalfY; // sit on legs
        const float margin = 0.05f;
        const float offX = topHalfX - legHalfX - margin;
        const float offZ = topHalfZ - legHalfZ - margin;

        auto setTextureWhite = [](HNode* n, GLuint tex){
            n->texture = tex; n->useTexture = (tex!=0);
            if(n->shape){
                auto &cols = n->shape->colors; for(auto &c : cols) c = glm::vec4(1.0f);
                glBindBuffer(GL_ARRAY_BUFFER, n->shape->vbo[1]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        };

        // Top
        {
            auto b = std::make_unique<box_t>(0, glm::vec3(topHalfX, topHalfY, topHalfZ));
            auto n = std::make_unique<HNode>(std::move(b));
            n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, topY, 0.0f));
            n->color = glm::vec4(1.0f);
            if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
            setTextureWhite(n.get(), state.texWooden);
            tbl->children.push_back(std::move(n));
        }
        // Helper to add a leg
        auto addLeg = [&](float x, float z){
            auto b = std::make_unique<box_t>(0, glm::vec3(legHalfX, legHalfY, legHalfZ));
            auto n = std::make_unique<HNode>(std::move(b));
            n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(x, legY, z));
            n->color = glm::vec4(1.0f);
            if(n->shape){ auto &cols=n->shape->colors; for(auto &c:cols) c=n->color; glBindBuffer(GL_ARRAY_BUFFER,n->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER,0,cols.size()*sizeof(glm::vec4),cols.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
            setTextureWhite(n.get(), state.texWooden);
            tbl->children.push_back(std::move(n));
        };
        addLeg( offX,  offZ);
        addLeg(-offX,  offZ);
        addLeg( offX, -offZ);
        addLeg(-offX, -offZ);

        state.scene.root->children.push_back(std::move(tableGroup));
    }
}

static std::string readFile(const char* path) {
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

// Forward declare key callback
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(){
    if(!glfwInit()){ std::cerr<<"GLFW init failed\n"; return -1; }
    GLFWwindow* win = glfwCreateWindow(1024,768,"Hierarchical Modeller",NULL,NULL);
    if(!win){ std::cerr<<"Window create failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glewExperimental = GL_TRUE; if(glewInit()!=GLEW_OK){ std::cerr<<"GLEW init failed\n"; return -1; }

    glfwSetInputMode(win, GLFW_STICKY_KEYS, GLFW_TRUE);

    GLuint prog = makeProgram();
    glUseProgram(prog);
    GLuint mvpLoc   = glGetUniformLocation(prog,"MVP");
    GLuint modelLoc = glGetUniformLocation(prog,"Model");
    state.useTexLoc = glGetUniformLocation(prog,"useTexture");
    state.samplerLoc= glGetUniformLocation(prog,"tex");
    glUniform1i(state.samplerLoc, 0);
    glEnable(GL_DEPTH_TEST);

    // scene
    build_scene();
    
    std::cout << "Scene built with " << state.scene.root->children.size() << " objects.\n";
    
    // robot centered at origin and raised onto platform
    state.robot.init();
    // Platform top is at y=0.2; robot base sphere scaled to 0.3 height (radius 0.5 * scaleY 0.3 = 0.15)
    // So base bottom at 0.2, top at 0.2+0.3=0.5 but sphere center needs to be at 0.2+0.15=0.35
    state.robot.model.root->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.35f, 0.0f));

    // Apply metal10 texture to robot base box
    if(state.texMetal10!=0 && state.robot.base){
        state.robot.base->texture = state.texMetal10;
        state.robot.base->useTexture = true;
        // Set base color to white to avoid tinting the texture
        state.robot.base->color = glm::vec4(1.0f);
        if(state.robot.base->shape){
            auto &cols = state.robot.base->shape->colors;
            for(auto &c: cols) c = state.robot.base->color;
            glBindBuffer(GL_ARRAY_BUFFER, state.robot.base->shape->vbo[1]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    // Load techno texture and assign to lower and upper arm cylinders only
    GLuint texTechno = makeTexture("images/techno.bmp");
    if(texTechno!=0){
        if(state.robot.lowerArmGeom){
            state.robot.lowerArmGeom->texture = texTechno;
            state.robot.lowerArmGeom->useTexture = true;
            // neutral color to avoid tint
            if(state.robot.lowerArmGeom->shape){
                auto &cols = state.robot.lowerArmGeom->shape->colors;
                for(auto &c: cols) c = glm::vec4(1.0f);
                glBindBuffer(GL_ARRAY_BUFFER, state.robot.lowerArmGeom->shape->vbo[1]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
        if(state.robot.upperArmGeom){
            state.robot.upperArmGeom->texture = texTechno;
            state.robot.upperArmGeom->useTexture = true;
            if(state.robot.upperArmGeom->shape){
                auto &cols = state.robot.upperArmGeom->shape->colors;
                for(auto &c: cols) c = glm::vec4(1.0f);
                glBindBuffer(GL_ARRAY_BUFFER, state.robot.upperArmGeom->shape->vbo[1]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
    }

    // Load techno01 for the hand (yellow box in description) and apply metal to grippers
    GLuint texTechno01 = makeTexture("images/techno01.bmp");
    if(texTechno01==0){ texTechno01 = makeTexture("images/techno.bmp"); }
    if(texTechno01!=0 && state.robot.handGeom){
        state.robot.handGeom->texture = texTechno01;
        state.robot.handGeom->useTexture = true;
        if(state.robot.handGeom->shape){
            auto &cols = state.robot.handGeom->shape->colors;
            for(auto &c: cols) c = glm::vec4(1.0f);
            glBindBuffer(GL_ARRAY_BUFFER, state.robot.handGeom->shape->vbo[1]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
    // Assign metal.bmp (already loaded as texPlatform) to grippers
    if(state.texPlatform!=0){
        if(state.robot.gripperLeft){
            state.robot.gripperLeft->texture = state.texPlatform;
            state.robot.gripperLeft->useTexture = true;
            if(state.robot.gripperLeft->shape){
                auto &cols = state.robot.gripperLeft->shape->colors;
                for(auto &c: cols) c = glm::vec4(1.0f);
                glBindBuffer(GL_ARRAY_BUFFER, state.robot.gripperLeft->shape->vbo[1]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
        if(state.robot.gripperRight){
            state.robot.gripperRight->texture = state.texPlatform;
            state.robot.gripperRight->useTexture = true;
            if(state.robot.gripperRight->shape){
                auto &cols = state.robot.gripperRight->shape->colors;
                for(auto &c: cols) c = glm::vec4(1.0f);
                glBindBuffer(GL_ARRAY_BUFFER, state.robot.gripperRight->shape->vbo[1]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }
    }
    
    std::cout << "Robot positioned on platform at y=0.35.\n";

    // Load and place additional models (scaled uniformly by 0.5 to preserve joints spacing)
    {
        bool okH = state.humanModel.load("human.mod");
        bool okC = state.carModel.load("car.mod");
        if(!okH) std::cerr << "Warning: could not load models/human.mod\n";
        if(!okC) std::cerr << "Warning: could not load models/car.mod\n";
        const float s = 0.5f;
        // Place human closer to robot (front-left) on floor
        state.humanWorld = glm::translate(glm::mat4(1.0f), glm::vec3(-1.6f, 0.0f, 1.2f));
        state.humanWorld = state.humanWorld * glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(0,1,0));
        state.humanWorld = state.humanWorld * glm::scale(glm::mat4(1.0f), glm::vec3(s));
        // Snap human to floor (raise by -minY)
        if(okH){
            glm::vec3 mn, mx; if(compute_aabb(state.humanModel, state.humanWorld, mn, mx)){
                if(mn.y != 0.0f){ state.humanWorld = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -mn.y, 0.0f)) * state.humanWorld; }
            }
        }
        // Place car closer to robot (back-right) on floor
        state.carWorld   = glm::translate(glm::mat4(1.0f), glm::vec3(1.8f, 0.0f, -1.0f));
        state.carWorld   = state.carWorld * glm::rotate(glm::mat4(1.0f), glm::radians(-35.0f), glm::vec3(0,1,0));
        state.carWorld   = state.carWorld * glm::scale(glm::mat4(1.0f), glm::vec3(s));
        if(okC){
            glm::vec3 mn, mx; if(compute_aabb(state.carModel, state.carWorld, mn, mx)){
                if(mn.y != 0.0f){ state.carWorld = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -mn.y, 0.0f)) * state.carWorld; }
            }
        }
    }

    // Projection matrices for the two cameras (wider FOV for scene)
    float aspect = 1024.0f/768.0f;
    glm::mat4 projScene = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 200.0f);
    glm::mat4 projFollow= glm::perspective(glm::radians(55.0f), aspect, 0.05f, 100.0f);

    glfwSetKeyCallback(win, key_callback);

    double last = glfwGetTime();
    while(!glfwWindowShouldClose(win)){
        double now = glfwGetTime(); float dt = float(now-last); last = now;
        // physics_step(dt); // no physics/toys now

        glClearColor(0.2f,0.25f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // camera selection (place camera inside the room)
        glm::mat4 proj, view;
        if(state.camMode == CAM_SCENE){
            proj = projScene;
            // Position camera to see the robot on the platform (robot center ~1.5-2.0 y)
            glm::vec3 eye(0.0f, 2.0f, 6.0f), tgt(0.0f, 1.2f, 0.0f);
            view = glm::lookAt(eye, tgt, glm::vec3(0,1,0));
        } else {
            proj = projFollow;
            // follow the robot hand joint (world position)
            glm::mat4 handWorld;
            state.robot.model.get_world_frame_of(state.robot.hand, handWorld);
            glm::vec3 target = glm::vec3(handWorld[3]);
            glm::vec3 eye = target + glm::vec3(1.8f, 0.9f, 2.0f);
            view = glm::lookAt(eye, target, glm::vec3(0,1,0));
        }
        glm::mat4 VP = proj * view;

        // set light uniforms (use explicit array indices)
        glUniform1i(glGetUniformLocation(prog,"numLights"), 2);
        glUniform3fv(glGetUniformLocation(prog,"lightPos[0]"), 1, &lights.l0Pos[0]);
        glUniform3fv(glGetUniformLocation(prog,"lightColor[0]"), 1, &lights.l0Col[0]);
        glUniform1i(glGetUniformLocation(prog,"lightOn[0]"), lights.l0On?1:0);
        glUniform3fv(glGetUniformLocation(prog,"lightPos[1]"), 1, &lights.l1Pos[0]);
        glUniform3fv(glGetUniformLocation(prog,"lightColor[1]"), 1, &lights.l1Col[0]);
        glUniform1i(glGetUniformLocation(prog,"lightOn[1]"), lights.l1On?1:0);

        // toy light (attached to hand tip)
        glm::mat4 handWorld;
        state.robot.model.get_world_frame_of(state.robot.hand, handWorld);
        glm::vec3 toyPos = glm::vec3(handWorld * glm::vec4(0, state.robot.handHeight, 0, 1));
        glUniform3fv(glGetUniformLocation(prog,"toyLightPos"), 1, &toyPos[0]);
        glUniform3fv(glGetUniformLocation(prog,"toyLightColor"), 1, &lights.toyCol[0]);
        glUniform1i(glGetUniformLocation(prog,"toyLightOn"), lights.toyOn?1:0);

        // draw room
        state.scene.draw(mvpLoc, modelLoc, VP, state.useTexLoc);

        // Draw additional models with a root transform that includes uniform scaling
        // This preserves joint closure by scaling translations as well
        if(state.humanModel.root){
            state.humanModel.draw_recursive(state.humanModel.root.get(), VP, state.humanWorld, mvpLoc, modelLoc, state.useTexLoc);
        }
        if(state.carModel.root){
            state.carModel.draw_recursive(state.carModel.root.get(), VP, state.carWorld, mvpLoc, modelLoc, state.useTexLoc);
        }

        // Ensure no texturing for the robot: disable and unbind
        // Remove the uniform reset so robot can use textures on its arms and base
        // if(state.useTexLoc >= 0){ glUniform1i(state.useTexLoc, 0); }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        state.robot.draw(mvpLoc, modelLoc, VP, state.useTexLoc);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
