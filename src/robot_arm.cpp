#include "robot_arm.hpp"
#include "sphere.hpp"
#include "cylinder.hpp"
#include "box.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <iostream>

RobotArm::RobotArm() {
    // Constructor - do nothing here
    // Actual initialization happens in init() after OpenGL context is ready
}

void RobotArm::init() {
    // Clear the model
    model.clear();

    // Ensure no global tilt so base looks horizontal
    model.root->rotate = glm::mat4(1.0f);

    // Base (Box instead of Sphere)
    auto baseBox = std::make_unique<box_t>(0);
    base = model.add_shape(std::move(baseBox));
    // Base scaling parameters
    const float baseScaleX = 0.5f, baseScaleY = 0.3f, baseScaleZ = 0.5f;
    base->scale = glm::scale(glm::mat4(1.0f), glm::vec3(baseScaleX, baseScaleY, baseScaleZ));
    base->color = glm::vec4(0.9f, 0.8f, 0.2f, 1.0f);
    if(base->shape){ for(auto &c: base->shape->colors) c = base->color; glBindBuffer(GL_ARRAY_BUFFER, base->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, base->shape->colors.size()*sizeof(glm::vec4), base->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);}    

    // Dimensions (derive baseTop from base half-height (0.5) and Y scale)
    const float baseRadius = 0.5f; // box_t default half extent
    const float baseTop = baseRadius * baseScaleY; // exact top of base after scaling (0.15)
    const float lowerLen = 0.8f;  // lower arm length (reduced from 1.0)
    const float upperLen = 0.8f;  // upper arm length (reduced from 1.0)
    const float jointR  = 0.15f;  // visual joint scale (reduced from 0.18)
    const float handH   = 0.25f;   // hand height (reduced from 0.3)
    handHeight = handH;           // set member for later computations

    // LOWER ARM JOINT (empty pivot node at top of base)
    auto lowerJoint = std::make_unique<HNode>();
    lowerJoint->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, baseTop, 0.0f));
    lowerArm = lowerJoint.get();
    base->children.push_back(std::move(lowerJoint));

    // Visual base joint sphere at top of base (between base and red cylinder)
    {
        auto sph = std::make_unique<sphere_t>(2);
        auto node = std::make_unique<HNode>(std::move(sph));
        // Place the sphere center exactly at the joint pivot (baseTop)
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        node->scale = glm::scale(glm::mat4(1.0f), glm::vec3(jointR));
        node->color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        lowerArm->children.push_back(std::move(node));
    }

    // Lower arm geometry (cylinder) under lowerArm joint; pivot at its bottom
    {
        auto cyl = std::make_unique<cylinder_t>(2);
        auto node = std::make_unique<HNode>(std::move(cyl));
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, lowerLen*0.5f, 0.0f));
        node->scale     = glm::scale(glm::mat4(1.0f), glm::vec3(0.12f, lowerLen, 0.12f));
        node->color     = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f); // brighter red
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        lowerArmGeom = node.get();
        lowerArm->children.push_back(std::move(node));
    }

    // UPPER ARM JOINT (empty) placed exactly at top of lower arm
    auto upperJoint = std::make_unique<HNode>();
    upperJoint->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, lowerLen, 0.0f));
    upperArm = upperJoint.get();
    lowerArm->children.push_back(std::move(upperJoint));

    // Visual middle joint sphere at the upper joint
    {
        auto sph = std::make_unique<sphere_t>(2);
        auto node = std::make_unique<HNode>(std::move(sph));
        node->scale = glm::scale(glm::mat4(1.0f), glm::vec3(jointR));
        node->color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        upperArm->children.push_back(std::move(node));
    }

    // Upper arm geometry under upperArm joint; pivot at its bottom
    {
        auto cyl = std::make_unique<cylinder_t>(2);
        auto node = std::make_unique<HNode>(std::move(cyl));
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, upperLen*0.5f, 0.0f));
        node->scale     = glm::scale(glm::mat4(1.0f), glm::vec3(0.10f, upperLen, 0.10f));
        node->color     = glm::vec4(0.3f, 0.6f, 1.0f, 1.0f); // brighter blue
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        upperArmGeom = node.get();
        upperArm->children.push_back(std::move(node));
    }

    // WRIST JOINT (empty) placed exactly at top of upper arm
    auto wristJ = std::make_unique<HNode>();
    wristJ->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, upperLen, 0.0f));
    wristJoint = wristJ.get();
    upperArm->children.push_back(std::move(wristJ));

    // Visual wrist sphere
    {
        auto sph = std::make_unique<sphere_t>(2);
        auto node = std::make_unique<HNode>(std::move(sph));
        node->scale = glm::scale(glm::mat4(1.0f), glm::vec3(jointR*0.9f));
        node->color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        wristJoint->children.push_back(std::move(node));
    }

    // Use wrist joint as the hand joint for rotations
    hand = wristJoint;

    // Hand geometry under hand joint; pivot at its bottom
    HNode* handGeomPtr = nullptr;
    {
        // Wider hand box
        handWidth = 0.35f;              // slightly smaller
        const float handDepth = 0.18f;
        auto box = std::make_unique<box_t>(1);
        auto node = std::make_unique<HNode>(std::move(box));
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, handH*0.5f, 0.0f));
        node->scale     = glm::scale(glm::mat4(1.0f), glm::vec3(handWidth, handH, handDepth));
        node->color     = glm::vec4(0.3f, 1.0f, 0.4f, 1.0f); // brighter green
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        handGeomPtr = node.get();
        handGeom = handGeomPtr;
        hand->children.push_back(std::move(node));
    }

    // Grippers as children of hand geometry
    gripperWidth  = 0.07f;      // slightly smaller
    gripperHeight = 0.30f;      // slightly smaller
    const float gripYCenter = 0.5f * (handHeight + gripperHeight); // bottom flush with hand top
    {
        auto box = std::make_unique<box_t>(0);
        auto node = std::make_unique<HNode>(std::move(box));
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(-0.14f, gripYCenter, 0.0f));
        node->scale     = glm::scale(glm::mat4(1.0f), glm::vec3(gripperWidth, gripperHeight, 0.07f));
        node->color     = glm::vec4(1.0f, 0.7f, 0.2f, 1.0f); // brighter orange
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        gripperLeft = node.get();
        handGeomPtr->children.push_back(std::move(node));
    }
    {
        auto box = std::make_unique<box_t>(0);
        auto node = std::make_unique<HNode>(std::move(box));
        node->translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.14f, gripYCenter, 0.0f));
        node->scale     = glm::scale(glm::mat4(1.0f), glm::vec3(gripperWidth, gripperHeight, 0.07f));
        node->color     = glm::vec4(1.0f, 0.7f, 0.2f, 1.0f); // brighter orange
        if(node->shape){ for(auto &c: node->shape->colors) c = node->color; glBindBuffer(GL_ARRAY_BUFFER, node->shape->vbo[1]); glBufferSubData(GL_ARRAY_BUFFER, 0, node->shape->colors.size()*sizeof(glm::vec4), node->shape->colors.data()); glBindBuffer(GL_ARRAY_BUFFER,0);} 
        gripperRight = node.get();
        handGeomPtr->children.push_back(std::move(node));
    }

    // Initial pose for better visibility - arms extended forward and upward
    lowerArmRotX = 0.0f;   // start horizontal
    lowerArmRotY = 0.0f;
    upperArmRotX = 0.0f;   // keep horizontal
    upperArmRotY = 0.0f;
    handRotX = 0.0f;
    handRotY = 0.0f;
    handRotZ = 0.0f;
    gripperOpen = 0.7f;    // more open so grippers are visible
    updateJoints();

    std::cout << "Robot arm initialized successfully!\n";
    std::cout << "  Base children: " << base->children.size() << "\n";
    std::cout << "  LowerArm children: " << lowerArm->children.size() << "\n";
    std::cout << "  UpperArm children: " << upperArm->children.size() << "\n";
    std::cout << "  Hand children: " << hand->children.size() << "\n";
}

void RobotArm::updateJoints() {
    // Update lower arm rotation (2 DOF)
    lowerArm->rotate = glm::mat4(1.0f);
    lowerArm->rotate = glm::rotate(lowerArm->rotate, lowerArmRotY, glm::vec3(0, 1, 0));
    lowerArm->rotate = glm::rotate(lowerArm->rotate, lowerArmRotX, glm::vec3(1, 0, 0));
    
    // Update upper arm rotation (2 DOF)
    upperArm->rotate = glm::mat4(1.0f);
    upperArm->rotate = glm::rotate(upperArm->rotate, upperArmRotY, glm::vec3(0, 1, 0));
    upperArm->rotate = glm::rotate(upperArm->rotate, upperArmRotX, glm::vec3(1, 0, 0));
    
    // Update hand rotation (3 DOF)
    hand->rotate = glm::mat4(1.0f);
    hand->rotate = glm::rotate(hand->rotate, handRotZ, glm::vec3(0, 0, 1));
    hand->rotate = glm::rotate(hand->rotate, handRotY, glm::vec3(0, 1, 0));
    hand->rotate = glm::rotate(hand->rotate, handRotX, glm::vec3(1, 0, 0));
    
    // Update gripper positions (open/close)
    // Clamp open amount and map to [closed..open] range so:
    // - closed: fingers touch at center (slight overlap)
    // - open: outer faces of grippers touch inner walls of the green hand
    float t = gripperOpen;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    const float handHalf = 0.5f * handWidth;          // half of hand width
    const float gripHalf = 0.5f * gripperWidth;       // half of gripper width
    const float eps      = gripperOverlapEps;         // small overlap to avoid seam when closed

    const float offsetClosed = (gripHalf - eps);
    const float offsetOpen   = (handHalf - gripHalf); // centers so outer faces align to hand inner walls
    const float offset       = (1.0f - t) * offsetClosed + t * offsetOpen;

    // keep bottom of gripper flush with hand top (so height grows upward)
    const float gripYCenter2 = 0.5f * (handHeight + gripperHeight);
    gripperLeft->translate  = glm::translate(glm::mat4(1.0f), glm::vec3(-offset, gripYCenter2, 0.0f));
    gripperRight->translate = glm::translate(glm::mat4(1.0f), glm::vec3( offset, gripYCenter2, 0.0f));
}

void RobotArm::draw(GLuint mvpLoc, GLuint modelLoc, const glm::mat4 &viewProj, GLint useTexLoc) {
    // Robot arm does not use textures; pass -1 for useTex uniform location
    model.draw(mvpLoc, modelLoc, viewProj, useTexLoc);
}
