#pragma once
#include "model.hpp"
#include <memory>

// Robot arm toy with hierarchical structure
class RobotArm {
public:
    model_t model;
    
    // Joint nodes for animation control
    HNode* base;
    HNode* lowerArm;
    HNode* middleJoint;
    HNode* upperArm;
    HNode* wristJoint;
    HNode* hand;
    HNode* gripperLeft;
    HNode* gripperRight;
    // Geometry nodes for texturing arms and hand
    HNode* lowerArmGeom = nullptr;
    HNode* upperArmGeom = nullptr;
    HNode* handGeom     = nullptr;
    
    // Joint angles for animation
    float lowerArmRotX = 0.0f;  // Lower arm rotation around X
    float lowerArmRotY = 0.0f;  // Lower arm rotation around Y (2 DOF)
    
    float upperArmRotX = 0.0f;  // Upper arm rotation around X
    float upperArmRotY = 0.0f;  // Upper arm rotation around Y (2 DOF)
    
    float handRotX = 0.0f;      // Hand rotation around X
    float handRotY = 0.0f;      // Hand rotation around Y
    float handRotZ = 0.0f;      // Hand rotation around Z (3 DOF)
    
    float gripperOpen = 0.0f;   // Gripper open/close state

    // Dimensions used for gripper logic (kept in sync with geometry scales)
    float handWidth = 0.3f;         // X width of the green hand box (scaled)
    float handHeight = 0.3f;        // Y height of the green hand box (scaled)
    float gripperWidth = 0.08f;     // X width of each gripper box (scaled)
    float gripperHeight = 0.3f;     // Y height of each gripper box (scaled)
    float gripperOverlapEps = 0.002f; // small overlap to avoid visible gap when closed
    
    RobotArm();
    void init();  // Initialize after OpenGL context is ready
    void updateJoints();
    void draw(GLuint mvpLoc, GLuint modelLoc, const glm::mat4 &viewProj, GLint useTexLoc);
};
