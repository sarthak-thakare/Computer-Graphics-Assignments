// -----------------------------------------------------------------------------
// main.cpp
// Application entry: sets up OpenGL/GLFW, loads shaders, builds scene, robot,
// handles input (animation capture/playback + camera movement + robot control),
// renders with simple multi-light Gouraud shading + optional texture mapping.
// Cleanup here adds human-readable commentary; no functional changes.
// -----------------------------------------------------------------------------
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
#include "animation.hpp"
#include "line_strip.hpp"
#include <sys/stat.h> // For mkdir


AnimationSystem gAnimationSystem; // global animation keyframe system
glm::vec3 gCameraEye(0.0f, 2.0f, 6.0f);     // Initial camera position
glm::vec3 gCameraLookAt(0.0f, 1.2f, 0.0f); // Initial camera target
glm::vec3 gCameraUp(0.0f, 1.0f, 0.0f);

// ANIMATION STATE GLOBALS 
const float g_FPS = 30.0f; // 30 frames per second
bool  g_isPlaying = false;
bool  g_isRecording = false;
int   g_recordingFrameNum = 0;
float g_animationTime = 0.0f; // Stores the current frame of the animation
float g_keyframeSaveTime = 0.0f; // For auto-incrementing keyframe time
double g_lastFrameTime = 0.0;    // For fixed-step timer

// VISUALIZER GLOBALS
std::unique_ptr<HNode> g_cameraPathSpline;   // The yellow smooth spline
std::unique_ptr<HNode> g_cameraControlPoints; // Dots at each keyframe
std::unique_ptr<HNode> g_cameraControlPolygon; // Straight lines connecting keys


// Forward declarations
static std::string readFile(const char* path);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool saveFramebuffer(GLFWwindow* window, const std::string& filename);
void applyAnimationState(float time); // Helper to set state


// ----------------------------------------------------------------------------
// Build Bezier camera path visualization (control points, polygon, smooth curve)
// ----------------------------------------------------------------------------
void updateCameraPathVisuals() {
    // Clear old visuals
    g_cameraPathSpline = nullptr;
    g_cameraControlPoints = nullptr;
    g_cameraControlPolygon = nullptr;

    if (gAnimationSystem.cameraKeys.size() < 2) {
        return; // Not enough points to draw
    }

    std::vector<glm::vec3> splinePoints;
    std::vector<glm::vec3> polygonPoints;
    auto controlPointsNode = std::make_unique<HNode>(); // Group for all point spheres

    const int TESS_LEVEL = 100; // Total segments for the whole curve

    //Create Control Points and Polygon
    for (size_t i = 0; i < gAnimationSystem.cameraKeys.size(); ++i) {
        const auto& key = gAnimationSystem.cameraKeys[i];
        // Add this key's 'eye' to the polygon path
        polygonPoints.push_back(key.eye);

        // Add a small sphere at the control point
        auto sphereShape = std::make_unique<sphere_t>(1); // Low-poly sphere
        auto sphereNode = std::make_unique<HNode>(std::move(sphereShape));
        sphereNode->translate = glm::translate(glm::mat4(1.0f), key.eye);
        sphereNode->scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f)); // Small dot
        sphereNode->color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Bright red
        if(sphereNode->shape){ for(auto &c: sphereNode->shape->colors) c = sphereNode->color; glBindBuffer(GL_ARRAY_BUFFER, sphereNode->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, sphereNode->shape->colors.size()*sizeof(glm::vec4), sphereNode->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);}
        controlPointsNode->children.push_back(std::move(sphereNode));
    }
    // Create the polygon line strip
    auto polygonShape = std::make_unique<line_strip_t>(polygonPoints, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red
    g_cameraControlPolygon = std::make_unique<HNode>(std::move(polygonShape));
    // Store the group of spheres
    g_cameraControlPoints = std::move(controlPointsNode);


    // Create Smooth Spline
    for (int j = 0; j <= TESS_LEVEL; ++j) {
        float t = (float)j / (float)TESS_LEVEL; 
        splinePoints.push_back(AnimationSystem::bezier(gAnimationSystem.cameraKeys, t));
    }
    
    auto splineShape = std::make_unique<line_strip_t>(splinePoints, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // Bright yellow
    g_cameraPathSpline = std::make_unique<HNode>(std::move(splineShape));

    std::cout << "updateCameraPathVisuals: Updated all 3 visualizers (Bézier).\n";
}

// Recursive helper to accumulate transformed vertex bounds
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
    AppMode appMode = ROBOT;     // Start in robot mode
    EditMode editMode = EDIT_NONE;
    char axis = 'X';
    model_t scene;
    RobotArm robot;      // The robot arm toy
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


// Minimal 24-bit BMP loader (BGR->RGB) for uncompressed textures
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
    for(unsigned i=0;i<imageSize;i+=3) std::swap(data[i], data[i+2]);
    return true;
}
// Create OpenGL texture with mipmaps from raw RGB data
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
// Compile GLSL shader from file, abort on failure
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
// Link basic vertex/fragment program
static GLuint makeProgram(){
    GLuint vs = compileShader("shaders/basic.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("shaders/basic.frag", GL_FRAGMENT_SHADER);
    GLuint p = glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs); glLinkProgram(p);
    GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){ char log[1024]; glGetProgramInfoLog(p,1024,nullptr,log); std::cerr<<log<<std::endl; exit(1); }
    return p;
}
// Debug print of current edit/camera mode (legacy modeller functionality)
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


// FRAMEBUFFER SAVE FUNCTION (writes simple TGA for recording)
bool saveFramebuffer(GLFWwindow* window, const std::string& filename) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
        std::cerr << "Failed to save frame: Invalid window size.\n";
        return false;
    }
    unsigned char tgaHeader[18] = { 0 };
    tgaHeader[2] = 2;  // Uncompressed, true-color image
    tgaHeader[12] = (width & 0xFF);
    tgaHeader[13] = (width >> 8);
    tgaHeader[14] = (height & 0xFF);
    tgaHeader[15] = (height >> 8);
    tgaHeader[16] = 24; // 24 bits per pixel (RGB)
    tgaHeader[17] = 0x00; // Bottom-to-top, left-to-right

    std::vector<unsigned char> pixels(width * height * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels.data());

    FILE* file = fopen(filename.c_str(), "wb");
    if (!file) {
        std::cerr << "Failed to open file for writing: " << filename << "\n";
        return false;
    }
    fwrite(tgaHeader, 1, sizeof(tgaHeader), file);
    fwrite(pixels.data(), 1, pixels.size(), file);
    fclose(file);
    return true;
}


// Central key handler: animation capture/playback, camera navigation, robot control
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(action != GLFW_PRESS) return; // Only handle press
    if(key==GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window,1); return; }


    // 'P' = Play/Pause Animation
    if (key == GLFW_KEY_P) {
        if (!g_isPlaying) {
            if (gAnimationSystem.sceneKeys.empty() && gAnimationSystem.cameraKeys.empty()) {
                std::cout << "No keyframes loaded. Press 'L' to load.\n";
                return;
            }
            g_isPlaying = true;
            
            g_lastFrameTime = glfwGetTime(); 
            
            std::cout << "Playback STARTED from frame " << g_animationTime << "\n";
        } else {
            g_isPlaying = false;
            std::cout << "Playback PAUSED at frame " << g_animationTime << "\n";
        }
        return;
    }

    // R = Record Animation
    if (key == GLFW_KEY_R) {
        if (g_isRecording) { std::cout << "Already recording.\n"; return; }
        if (gAnimationSystem.sceneKeys.empty() && gAnimationSystem.cameraKeys.empty()) {
            std::cout << "No keyframes loaded. Press 'L' to load.\n";
            return;
        }
        
        // Create snapshots directory
        mkdir("snapshots", 0755); 

        g_isRecording = true;
        g_isPlaying = true;
        g_animationTime = 0.0f; // Always record from the start

        g_lastFrameTime = glfwGetTime();

        g_recordingFrameNum = 0;
        std::cout << "RECORDING STARTED...\n";
        return;
    }
    
    // 'T' = Set Time for next keyframe
    if (key == GLFW_KEY_T) {
        if (g_isPlaying) { std::cout << "Cannot set time while playing.\n"; return; }
        std::cout << "Current next-frame time is: " << g_keyframeSaveTime << "\n";
        std::cout << "Enter new next-frame time: ";
        float newTime;
        std::cin >> newTime;
        if (newTime >= g_keyframeSaveTime) {
            g_keyframeSaveTime = newTime;
            std::cout << "Next keyframe time set to: " << g_keyframeSaveTime << "\n";
        } else {
            std::cout << "Error: New time must be >= " << g_keyframeSaveTime << "\n";
        }
        return;
    }
    
    // SCRUBBING CONTROLS
    if (key == GLFW_KEY_MINUS) { // '-' key
        if (g_isPlaying) return;
        g_animationTime = std::max(0.0f, g_animationTime - 1.0f);
        applyAnimationState(g_animationTime);
        std::cout << "Frame: " << g_animationTime << "\n";
        return;
    }
    if (key == GLFW_KEY_EQUAL) { // '=' key
        if (g_isPlaying) return;
        // Find max time
        float maxSceneTime = gAnimationSystem.sceneKeys.empty() ? 0 : gAnimationSystem.sceneKeys.back().t;
        float maxCamTime = gAnimationSystem.cameraKeys.empty() ? 0 : gAnimationSystem.cameraKeys.back().t;
        float maxTime = std::max(maxSceneTime, maxCamTime);
        
        g_animationTime = std::min(maxTime, g_animationTime + 1.0f);
        applyAnimationState(g_animationTime);
        std::cout << "Frame: " << g_animationTime << "\n";
        return;
    }

    // 'C' = Capture (Camera ONLY)
    if (key == GLFW_KEY_C && !(mods & GLFW_MOD_CONTROL) && !(mods & GLFW_MOD_SHIFT)) {
        CameraKey ck{ g_keyframeSaveTime, gCameraEye, gCameraLookAt, gCameraUp };
        gAnimationSystem.cameraKeys.push_back(ck);
        std::cout << "Captured Camera-ONLY key at frame=" << g_keyframeSaveTime << "\n";
        g_keyframeSaveTime += 10.0f; // Default increment
        updateCameraPathVisuals();
        return;
    }

    // 'Ctrl+C' = Capture (Camera + Scene)
    if ((mods & GLFW_MOD_CONTROL) && key == GLFW_KEY_C) {
        // Get robot pose
        SceneKey sk = state.robot.getPose();
        sk.t = g_keyframeSaveTime;
        // Add scene data
        sk.light0On   = lights.l0On ? 1.0f : 0.0f;
        sk.light1On   = lights.l1On ? 1.0f : 0.0f;
        sk.toyLightOn = lights.toyOn ? 1.0f : 0.0f;
        sk.carPos = glm::vec3(state.carWorld[3]); 
        sk.carYaw = 0.0f;
        gAnimationSystem.sceneKeys.push_back(sk);

        // Get CURRENT camera state
        CameraKey ck{ g_keyframeSaveTime, gCameraEye, gCameraLookAt, gCameraUp };
        gAnimationSystem.cameraKeys.push_back(ck);
        
        std::cout << "Captured Scene+Camera key at frame=" << g_keyframeSaveTime << "\n";
        g_keyframeSaveTime += 10.0f; // Default increment
        updateCameraPathVisuals();
        return;
    }

    // 'Shift+C' = Save (Camera Trajectory ONLY)
    if ((mods & GLFW_MOD_SHIFT) && key == GLFW_KEY_C) {
        gAnimationSystem.saveCameraKeys("camera.key");
        std::cout << "Saved Camera Trajectory to camera.key.\n";
        return;
    }

    // 'S' = Save All (Scene + Camera)
    if (key == GLFW_KEY_S) {
        gAnimationSystem.saveCameraKeys("camera.key");
        gAnimationSystem.saveSceneKeys("scene.key");
        std::cout << "Saved all keyframes to file.\n";
        return;
    }

    // 'L' = Load All
    if (key == GLFW_KEY_L) {
        gAnimationSystem.loadCameraKeys("camera.key");
        gAnimationSystem.loadSceneKeys("scene.key");
        std::cout << "Loaded keyframes.\n";
        
        float lastSceneTime = gAnimationSystem.sceneKeys.empty() ? 0 : gAnimationSystem.sceneKeys.back().t;
        float lastCamTime = gAnimationSystem.cameraKeys.empty() ? 0 : gAnimationSystem.cameraKeys.back().t;
        g_keyframeSaveTime = std::max(lastSceneTime, lastCamTime) + 10.0f;
        std::cout << "Next keyframe will be saved at frame " << g_keyframeSaveTime << "\n";
        
        updateCameraPathVisuals();
        applyAnimationState(0.0f); // Reset view to first frame
        return;
    }


    if (state.camMode == CAM_SCENE) {
        float camSpeed = 0.1f;
        float rotSpeed = 0.05f;
        glm::vec3 forward = glm::normalize(gCameraLookAt - gCameraEye);
        glm::vec3 right = glm::normalize(glm::cross(forward, gCameraUp));
        
        // I/K: Move forward/backward
        if (key == GLFW_KEY_I) { gCameraEye += camSpeed * forward; gCameraLookAt += camSpeed * forward; return; }
        if (key == GLFW_KEY_K) { gCameraEye -= camSpeed * forward; gCameraLookAt -= camSpeed * forward; return; }
        // COMMA/PERIOD: Strafe left/right
        if (key == GLFW_KEY_COMMA) { gCameraEye -= camSpeed * right; gCameraLookAt -= camSpeed * right; return; }
        if (key == GLFW_KEY_PERIOD) { gCameraEye += camSpeed * right; gCameraLookAt += camSpeed * right; return; }
        // [/]: Move up/down
        if (key == GLFW_KEY_LEFT_BRACKET) { gCameraEye -= camSpeed * gCameraUp; gCameraLookAt -= camSpeed * gCameraUp; return; }
        if (key == GLFW_KEY_RIGHT_BRACKET) { gCameraEye += camSpeed * gCameraUp; gCameraLookAt += camSpeed * gCameraUp; return; }
        // PageUp/PageDown: Pan (Yaw)
        if (key == GLFW_KEY_PAGE_UP) { glm::mat4 rot = glm::rotate(glm::mat4(1.0f), rotSpeed, gCameraUp); gCameraLookAt = gCameraEye + glm::vec3(rot * glm::vec4(forward, 0.0f)); return; }
        if (key == GLFW_KEY_PAGE_DOWN) { glm::mat4 rot = glm::rotate(glm::mat4(1.0f), -rotSpeed, gCameraUp); gCameraLookAt = gCameraEye + glm::vec3(rot * glm::vec4(forward, 0.0f)); return; }
        // Home/End: Tilt (Pitch)
        if (key == GLFW_KEY_HOME) { glm::mat4 rot = glm::rotate(glm::mat4(1.0f), rotSpeed, right); gCameraLookAt = gCameraEye + glm::vec3(rot * glm::vec4(forward, 0.0f)); return; }
        if (key == GLFW_KEY_END) { glm::mat4 rot = glm::rotate(glm::mat4(1.0f), -rotSpeed, right); gCameraLookAt = gCameraEye + glm::vec3(rot * glm::vec4(forward, 0.0f)); return; }
    }

    // --- ORIGINAL ROBOT/SCENE CONTROLS ---
    if(key==GLFW_KEY_V){ state.camMode = (state.camMode==CAM_SCENE? CAM_FOLLOW: CAM_SCENE); std::cout<<"Camera: "<<(state.camMode==CAM_SCENE?"Scene":"Follow")<<"\n"; return; }
    if(key==GLFW_KEY_8){ lights.l0On = !lights.l0On; std::cout<<"Light 0: "<<(lights.l0On?"On":"Off")<<"\n"; return; }
    if(key==GLFW_KEY_9){ lights.l1On = !lights.l1On; std::cout<<"Light 1: "<<(lights.l1On?"On":"Off")<<"\n"; return; }
    if(key==GLFW_KEY_0){ lights.toyOn = !lights.toyOn; std::cout<<"Toy Light: "<<(lights.toyOn?"On":"Off")<<"\n"; return; }
    float angleStep = 0.1f, gripStep = 0.02f;
    if(key==GLFW_KEY_UP)    { state.robot.lowerArmRotX += angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_DOWN)  { state.robot.lowerArmRotX -= angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_LEFT)  { state.robot.lowerArmRotY += angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_RIGHT) { state.robot.lowerArmRotY -= angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_W) { state.robot.upperArmRotX += angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_F) { state.robot.upperArmRotX -= angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_A) { state.robot.upperArmRotY += angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_D) { state.robot.upperArmRotY -= angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_Q) { state.robot.handRotX += angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_E) { state.robot.handRotX -= angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_Z) { state.robot.handRotY += angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_Y) { state.robot.handRotY -= angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_1) { state.robot.handRotZ += angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_2) { state.robot.handRotZ -= angleStep; state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_O) { state.robot.gripperOpen = std::min(1.0f, state.robot.gripperOpen + gripStep); state.robot.updateJoints(); return; }
    if(key==GLFW_KEY_B) { state.robot.gripperOpen = std::max(0.0f, state.robot.gripperOpen - gripStep); state.robot.updateJoints(); return; }

    // Help
    if(key==GLFW_KEY_H) {
        std::cout << "\n=== ANIMATION CONTROLS ===\n";
        std::cout << "P: Play/Pause animation\n";
        std::cout << "R: Record animation to TGA files\n";
        std::cout << "T: Set time for the next keyframe\n";
        std::cout << "-/=: Scrub animation backward/forward 1 frame\n";
        std::cout << "L: Load 'camera.key' and 'scene.key'\n";
        std::cout << "S: Save ALL keyframes to file\n";
        std::cout << "C: Save Camera-ONLY keyframe\n";
        std::cout << "Ctrl+C: Save Scene + Camera keyframe\n";
        std::cout << "Shift+C: Save Camera Trajectory ONLY (to camera.key)\n";
        std::cout << "\n=== CAMERA CONTROLS (Scene Mode) ===\n";
        std::cout << "I/K: Move Forward/Backward\n";
        std::cout << ",/. (Comma/Period): Strafe Left/Right\n";
        std::cout << "[/]: Move Up/Down\n";
        std::cout << "Home/End: Tilt Up/Down\n";
        std::cout << "PageUp/PageDown: Pan Left/Right\n";
        std::cout << "\n=== ROBOT ARM CONTROLS ===\n";
        std::cout << "Arrow Keys: Lower arm (pitch/rotate)\n";
        std::cout << "W/F: Upper arm (pitch), A/D (rotate)\n";
        std::cout << "Q/E: Hand pitch, Z/Y: Hand yaw, 1/2: Hand roll\n";
        std::cout << "O/B: Open/Close gripper\n";
        std::cout << "8/9/0: Toggle Lights\n";
        std::cout << "V: Toggle Camera (Scene/Follow)\n\n";
        return;
    }
}

// Build static environment (room, table) and apply textures
static void build_scene(){
    state.scene.clear();
    state.texFloor  = makeTexture("images/wood.bmp");
    state.texWall   = makeTexture("images/bricks.bmp");
    state.texPlatform = makeTexture("images/metal.bmp");
    state.texMetal10  = makeTexture("images/metal10.bmp");
    state.texWooden   = makeTexture("images/wooden.bmp");
    auto setTextureWhite = [](HNode* n, GLuint tex){
        n->texture = tex; n->useTexture = (tex!=0);
        if(n->shape){
            auto &cols = n->shape->colors; for(auto &c : cols) c = glm::vec4(1.0f);
            glBindBuffer(GL_ARRAY_BUFFER, n->shape->vbo[1]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    };
    { auto b = std::make_unique<box_t>(0, glm::vec3(12,0.1f,12)); HNode* n = state.scene.add_shape(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f)); setTextureWhite(n, state.texFloor); }
    { auto b = std::make_unique<box_t>(0, glm::vec3(12,5,0.05f)); HNode* n = state.scene.add_shape(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, -12.0f)); setTextureWhite(n, state.texWall); }
    { auto b = std::make_unique<box_t>(0, glm::vec3(12,5,0.05f)); HNode* n = state.scene.add_shape(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, 12.0f)); setTextureWhite(n, state.texWall); }
    { auto b = std::make_unique<box_t>(0, glm::vec3(0.05f,5,12)); HNode* n = state.scene.add_shape(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(12.0f, 2.5f, 0.0f)); setTextureWhite(n, state.texWall); }
    { auto b = std::make_unique<box_t>(0, glm::vec3(0.05f,5,12)); HNode* n = state.scene.add_shape(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(-12.0f, 2.5f, 0.0f)); setTextureWhite(n, state.texWall); }
    { auto b = std::make_unique<box_t>(0, glm::vec3(12,0.05f,12)); HNode* n = state.scene.add_shape(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f)); setTextureWhite(n, state.texWall); }
    { auto b = std::make_unique<box_t>(0, glm::vec3(1.2f, 0.1f, 1.2f)); HNode* n = state.scene.add_shape(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.1f, 0.0f)); setTextureWhite(n, state.texPlatform); }
    {
        auto tableGroup = std::make_unique<HNode>();
        tableGroup->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -6.0f));
        HNode* tbl = tableGroup.get();
        const float topHalfX = 0.9f, topHalfY = 0.04f, topHalfZ = 0.6f;
        const float legHalfX = 0.05f, legHalfY = 0.36f, legHalfZ = 0.05f;
        const float legY = legHalfY; const float topY = 2*legHalfY + topHalfY;
        const float margin = 0.05f; const float offX = topHalfX - legHalfX - margin;
        const float offZ = topHalfZ - legHalfZ - margin;
        { auto b = std::make_unique<box_t>(0, glm::vec3(topHalfX, topHalfY, topHalfZ)); auto n = std::make_unique<HNode>(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, topY, 0.0f)); setTextureWhite(n.get(), state.texWooden); tbl->children.push_back(std::move(n)); }
        auto addLeg = [&](float x, float z){ auto b = std::make_unique<box_t>(0, glm::vec3(legHalfX, legHalfY, legHalfZ)); auto n = std::make_unique<HNode>(std::move(b)); n->translate = glm::translate(glm::mat4(1.0f), glm::vec3(x, legY, z)); setTextureWhite(n.get(), state.texWooden); tbl->children.push_back(std::move(n)); };
        addLeg( offX,  offZ); addLeg(-offX,  offZ); addLeg( offX, -offZ); addLeg(-offX, -offZ);
        state.scene.root->children.push_back(std::move(tableGroup));
    }
}


// Utility: read entire file into string
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


// Apply interpolated animation state (camera & robot + lights + car position)
void applyAnimationState(float time) {
    
    if (gAnimationSystem.cameraKeys.empty() && gAnimationSystem.sceneKeys.empty()) {
        return; 
    }

    //Get the interpolated state
    AnimationSystem::AnimationState currentState = gAnimationSystem.update(time);

    //Apply Camera State
    if (!gAnimationSystem.cameraKeys.empty()) {
        gCameraEye    = currentState.camera.eye;
        gCameraLookAt = currentState.camera.lookAt;
        gCameraUp     = currentState.camera.up;
    }
    
    // Apply Scene State
    if (!gAnimationSystem.sceneKeys.empty() && time >= gAnimationSystem.sceneKeys.front().t) 
    {
        state.robot.setPose(currentState.scene); 
        lights.l0On = (currentState.scene.light0On >= 0.5f);
        lights.l1On = (currentState.scene.light1On >= 0.5f);
        lights.toyOn = (currentState.scene.toyLightOn >= 0.5f);

        // Re-build the car's world matrix
        glm::mat4 originalCarTransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.8f, 0.0f, -1.0f)) 
                                       * glm::rotate(glm::mat4(1.0f), glm::radians(-35.0f), glm::vec3(0,1,0)) 
                                       * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    
        state.carWorld = originalCarTransform;
        state.carWorld[3] = glm::vec4(currentState.scene.carPos, 1.0f);
    }
}

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
    glEnable(GL_LINE_SMOOTH); // Make lines look nicer

    // scene
    build_scene();
    std::cout << "Scene built with " << state.scene.root->children.size() << " objects.\n";
    
    // robot
    state.robot.init();
    state.robot.model.root->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.35f, 0.0f));
    //robot texturing
    if(state.texMetal10!=0 && state.robot.base){ state.robot.base->texture = state.texMetal10; state.robot.base->useTexture = true; if(state.robot.base->shape){ auto &cols = state.robot.base->shape->colors; for(auto &c: cols) c = glm::vec4(1.0f); glBindBuffer(GL_ARRAY_BUFFER, state.robot.base->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data()); glBindBuffer(GL_ARRAY_BUFFER, 0); } }
    GLuint texTechno = makeTexture("images/techno.bmp");
    if(texTechno!=0){ if(state.robot.lowerArmGeom){ state.robot.lowerArmGeom->texture = texTechno; state.robot.lowerArmGeom->useTexture = true; if(state.robot.lowerArmGeom->shape){ auto &cols = state.robot.lowerArmGeom->shape->colors; for(auto &c: cols) c = glm::vec4(1.0f); glBindBuffer(GL_ARRAY_BUFFER, state.robot.lowerArmGeom->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data()); glBindBuffer(GL_ARRAY_BUFFER, 0); } } if(state.robot.upperArmGeom){ state.robot.upperArmGeom->texture = texTechno; state.robot.upperArmGeom->useTexture = true; if(state.robot.upperArmGeom->shape){ auto &cols = state.robot.upperArmGeom->shape->colors; for(auto &c: cols) c = glm::vec4(1.0f); glBindBuffer(GL_ARRAY_BUFFER, state.robot.upperArmGeom->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data()); glBindBuffer(GL_ARRAY_BUFFER, 0); } } }
    GLuint texTechno01 = makeTexture("images/techno01.bmp");
    if(texTechno01==0){ texTechno01 = makeTexture("images/techno.bmp"); }
    if(texTechno01!=0 && state.robot.handGeom){ state.robot.handGeom->texture = texTechno01; state.robot.handGeom->useTexture = true; if(state.robot.handGeom->shape){ auto &cols = state.robot.handGeom->shape->colors; for(auto &c: cols) c = glm::vec4(1.0f); glBindBuffer(GL_ARRAY_BUFFER, state.robot.handGeom->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data()); glBindBuffer(GL_ARRAY_BUFFER, 0); } }
    if(state.texPlatform!=0){ if(state.robot.gripperLeft){ state.robot.gripperLeft->texture = state.texPlatform; state.robot.gripperLeft->useTexture = true; if(state.robot.gripperLeft->shape){ auto &cols = state.robot.gripperLeft->shape->colors; for(auto &c: cols) c = glm::vec4(1.0f); glBindBuffer(GL_ARRAY_BUFFER, state.robot.gripperLeft->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data()); glBindBuffer(GL_ARRAY_BUFFER, 0); } } if(state.robot.gripperRight){ state.robot.gripperRight->texture = state.texPlatform; state.robot.gripperRight->useTexture = true; if(state.robot.gripperRight->shape){ auto &cols = state.robot.gripperRight->shape->colors; for(auto &c: cols) c = glm::vec4(1.0f); glBindBuffer(GL_ARRAY_BUFFER, state.robot.gripperRight->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, cols.size()*sizeof(glm::vec4), cols.data()); glBindBuffer(GL_ARRAY_BUFFER, 0); } } }
    std::cout << "Robot positioned on platform.\n";

    //Loading human and car models
    {
        bool okH = state.humanModel.load("human.mod");
        bool okC = state.carModel.load("car.mod");
        if(!okH) std::cerr << "Warning: could not load human.mod\n";
        if(!okC) std::cerr << "Warning: could not load car.mod\n";
        const float s = 0.5f;
        state.humanWorld = glm::translate(glm::mat4(1.0f), glm::vec3(-1.6f, 0.0f, 1.2f)) * glm::rotate(glm::mat4(1.0f), glm::radians(20.0f), glm::vec3(0,1,0)) * glm::scale(glm::mat4(1.0f), glm::vec3(s));
        if(okH){ glm::vec3 mn, mx; if(compute_aabb(state.humanModel, state.humanWorld, mn, mx)){ if(mn.y != 0.0f){ state.humanWorld = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -mn.y, 0.0f)) * state.humanWorld; } } }
        state.carWorld   = glm::translate(glm::mat4(1.0f), glm::vec3(1.8f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0,1,0)) * glm::scale(glm::mat4(1.0f), glm::vec3(s));
        if(okC){ glm::vec3 mn, mx; if(compute_aabb(state.carModel, state.carWorld, mn, mx)){ if(mn.y != 0.0f){ state.carWorld = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -mn.y, 0.0f)) * state.carWorld; } } }
    }

    float aspect = 1024.0f/768.0f;
    glm::mat4 projScene = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 200.0f);
    glm::mat4 projFollow= glm::perspective(glm::radians(55.0f), aspect, 0.05f, 100.0f);

    glfwSetKeyCallback(win, key_callback);
    std::cout << "--- Press 'H' for controls --- \n";

    g_lastFrameTime = glfwGetTime(); // Initialize frame timer

    while(!glfwWindowShouldClose(win)){
        
        double currentTime = glfwGetTime();
        double frameDuration = 1.0 / g_FPS;
        double deltaTime = currentTime - g_lastFrameTime;

        bool shouldRender = false; // Flag to see if we need to draw a new frame

        if (g_isPlaying) {
            // Check if enough time has passed to advance one animation frame
            if (deltaTime >= frameDuration) {
                g_lastFrameTime += frameDuration; 

                // Advance animation time by exactly 1.0 frame
                g_animationTime += 1.0f;

                // Check if animation finished
                float maxSceneTime = gAnimationSystem.sceneKeys.empty() ? 0 : gAnimationSystem.sceneKeys.back().t;
                float maxCamTime = gAnimationSystem.cameraKeys.empty() ? 0 : gAnimationSystem.cameraKeys.back().t;
                float maxTime = std::max(maxSceneTime, maxCamTime);
                
                if (g_animationTime > maxTime) {
                    g_animationTime = maxTime; // Clamp time
                    g_isPlaying = false;
                    if (g_isRecording) {
                        g_isRecording = false;
                        std::cout << "RECORDING FINISHED\n";
                    } else {
                        std::cout << "Playback FINISHED\n";
                    }
                }

                //Apply the new state
                applyAnimationState(g_animationTime);
                shouldRender = true; // We have a new frame to render

                //Save frame if recording
                if (g_isRecording) {
                    char filename[100];
                    sprintf(filename, "snapshots/frame-%05d.tga", g_recordingFrameNum);
                    saveFramebuffer(win, filename); 
                    g_recordingFrameNum++; 
                }
            }
            // If deltaTime < frameDuration, we do nothing and just wait.

        } else {
            shouldRender = true;
        }


        // RENDER BLOCK 
        if (shouldRender) {
            glClearColor(0.2f,0.25f,0.3f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 proj, view;
            
            if(state.camMode == CAM_SCENE || g_isPlaying){
                proj = projScene;
                view = glm::lookAt(gCameraEye, gCameraLookAt, gCameraUp);
            } else { 
                // Manual Follow Camera 
                proj = projFollow;
                glm::mat4 handWorld;
                state.robot.model.get_world_frame_of(state.robot.hand, handWorld);
                glm::vec3 target = glm::vec3(handWorld[3]);
                glm::vec3 eye = target + glm::vec3(1.8f, 0.9f, 2.0f);
                view = glm::lookAt(eye, target, glm::vec3(0,1,0));
                
                // Update globals so saving 'C' in this mode works
                gCameraEye = eye; gCameraLookAt = target; gCameraUp = glm::vec3(0,1,0);
            }
            
            glm::mat4 VP = proj * view;

            glUniform1i(glGetUniformLocation(prog,"numLights"), 2);
            glUniform3fv(glGetUniformLocation(prog,"lightPos[0]"), 1, &lights.l0Pos[0]);
            glUniform3fv(glGetUniformLocation(prog,"lightColor[0]"), 1, &lights.l0Col[0]);
            glUniform1i(glGetUniformLocation(prog,"lightOn[0]"), lights.l0On?1:0);
            glUniform3fv(glGetUniformLocation(prog,"lightPos[1]"), 1, &lights.l1Pos[0]);
            glUniform3fv(glGetUniformLocation(prog,"lightColor[1]"), 1, &lights.l1Col[0]);
            glUniform1i(glGetUniformLocation(prog,"lightOn[1]"), lights.l1On?1:0);
            glm::mat4 handWorld;
            state.robot.model.get_world_frame_of(state.robot.hand, handWorld);
            glm::vec3 toyPos = glm::vec3(handWorld * glm::vec4(0, state.robot.handHeight, 0, 1));
            glUniform3fv(glGetUniformLocation(prog,"toyLightPos"), 1, &toyPos[0]);
            glUniform3fv(glGetUniformLocation(prog,"toyLightColor"), 1, &lights.toyCol[0]);
            glUniform1i(glGetUniformLocation(prog,"toyLightOn"), lights.toyOn?1:0);

            // draw room
            state.scene.draw(mvpLoc, modelLoc, VP, state.useTexLoc);

            // Draw additional models
            if(state.humanModel.root) state.humanModel.draw_recursive(state.humanModel.root.get(), VP, state.humanWorld, mvpLoc, modelLoc, state.useTexLoc);
            if(state.carModel.root) state.carModel.draw_recursive(state.carModel.root.get(), VP, state.carWorld, mvpLoc, modelLoc, state.useTexLoc);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // Draw robot
            state.robot.draw(mvpLoc, modelLoc, VP, state.useTexLoc);

            // Draw Camera Visualizers
            if (state.camMode == CAM_SCENE && !g_isPlaying) {
                glm::mat4 identity = glm::mat4(1.0f);
                if (g_cameraPathSpline) {
                    state.scene.draw_recursive(g_cameraPathSpline.get(), VP, identity, mvpLoc, modelLoc, state.useTexLoc);
                }
                if (g_cameraControlPolygon) {
                    state.scene.draw_recursive(g_cameraControlPolygon.get(), VP, identity, mvpLoc, modelLoc, state.useTexLoc);
                }
                if (g_cameraControlPoints) {
                    state.scene.draw_recursive(g_cameraControlPoints.get(), VP, identity, mvpLoc, modelLoc, state.useTexLoc);
                }
            }
            
            glfwSwapBuffers(win);
        } 

        glfwPollEvents();
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}