// -----------------------------------------------------------------------------
// animation.hpp
// Simple keyframe I/O and interpolation helpers for camera and scene state.
// Provides Bezier path for camera eye/lookAt and linear interpolation for
// other scene parameters. No functional changes in this cleanup.
// -----------------------------------------------------------------------------
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>     

// Represents one camera state
struct CameraKey {
    float t;                     // timestamp 
    glm::vec3 eye, lookAt, up;    
};

// Represents one full scene state
struct SceneKey {
    float t; // timestamp 
    // robot arm 
    float lowerArmPitch, lowerArmYaw;
    float upperArmPitch, upperArmYaw;
    float handPitch, handYaw, handRoll;
    float gripperOpen;

    // Light states (0.0f for off, 1.0f for on)
    float light0On;
    float light1On;
    float toyLightOn;

    // Other object transforms (example for car)
    glm::vec3 carPos;
    float     carYaw; // Y-axis rotation
};

// Full animation state manager
struct AnimationSystem {
    std::vector<CameraKey> cameraKeys;
    std::vector<SceneKey> sceneKeys;

    // Camera Keyframes
    bool saveCameraKeys(const std::string &filename) const {
        std::ofstream fout(filename);
        if (!fout) return false;
        for (auto &k : cameraKeys) {
            fout << k.t << " "<< k.eye.x << " " << k.eye.y << " " << k.eye.z << " "<< k.lookAt.x << " " << k.lookAt.y << " " << k.lookAt.z << " "<< k.up.x << " " << k.up.y << " " << k.up.z << "\n";
        }
        return true;
    }

    bool loadCameraKeys(const std::string &filename) {
        std::ifstream fin(filename);
        if (!fin) return false;
        cameraKeys.clear();
        CameraKey k;
        while (fin >> k.t>> k.eye.x >> k.eye.y >> k.eye.z>> k.lookAt.x >> k.lookAt.y >> k.lookAt.z >> k.up.x >> k.up.y >> k.up.z) {
            cameraKeys.push_back(k);
        }
        std::cout << "Loaded " << cameraKeys.size() << " camera keys from " << filename << "\n";
        return true;
    }

    // Scene Keyframes
    bool saveSceneKeys(const std::string &filename) const {
        std::ofstream fout(filename);
        if (!fout) return false;
        for (auto &k : sceneKeys) {
            fout << k.t << " "
                 << k.lowerArmPitch << " " << k.lowerArmYaw << " "
                 << k.upperArmPitch << " " << k.upperArmYaw << " "
                 << k.handPitch << " " << k.handYaw << " "
                 << k.handRoll << " "
                 << k.gripperOpen << " "
                 << k.light0On << " " << k.light1On << " " << k.toyLightOn << " "
                 << k.carPos.x << " " << k.carPos.y << " " << k.carPos.z << " "
                 << k.carYaw << "\n";
        }
        return true;
    }

    bool loadSceneKeys(const std::string &filename) {
        std::ifstream fin(filename);
        if (!fin) return false;
        sceneKeys.clear();
        SceneKey k;
        while (fin >> k.t
                 >> k.lowerArmPitch >> k.lowerArmYaw
                 >> k.upperArmPitch >> k.upperArmYaw
                 >> k.handPitch >> k.handYaw
                 >> k.handRoll
                 >> k.gripperOpen
                 >> k.light0On >> k.light1On >> k.toyLightOn
                 >> k.carPos.x >> k.carPos.y >> k.carPos.z
                 >> k.carYaw) {
            sceneKeys.push_back(k);
        }
        std::cout << "Loaded " << sceneKeys.size() << " scene keys from " << filename << "\n";
        return true;
    }

    // Interpolation
    struct AnimationState {
        CameraKey camera;
        SceneKey scene;
    };

    static long double nCr(int n, int r) {
        if(r < 0 || r > n) return 0;
        if(r == 0 || r == n) return 1.0;
        if(r > n / 2) r = n - r; 
        long double res = 1.0;
        for(int i = 1; i <= r; ++i) {
            res = res * (n - i + 1) / i;
        }
        return res;
    }


    // Evaluate Bezier curve over cameraKeys' eye positions
    static glm::vec3 bezier(const std::vector<CameraKey>& keys, float t) {
        if (keys.empty()) return glm::vec3(0.0f);
        
        int n = keys.size() - 1; // Degree of the curve
        if (n < 0) return glm::vec3(0.0f);

        glm::vec3 point(0.0f);
        float t_clamped = std::clamp(t, 0.0f, 1.0f); // t must be in [0, 1]

        for (int i = 0; i <= n; ++i) {
            long double bernstein = nCr(n, i) * pow(1.0 - t_clamped, n - i) * pow(t_clamped, i);
            point += keys[i].eye * (float)bernstein;
        }
        return point;
    }

    // Evaluate Bezier curve over cameraKeys' lookAt positions
    static glm::vec3 bezierLookAt(const std::vector<CameraKey>& keys, float t) {
        if (keys.empty()) return glm::vec3(0.0f, 0.0f, -1.0f);
        
        int n = keys.size() - 1; // Degree of the curve
        if (n < 0) return glm::vec3(0.0f); 

        glm::vec3 point(0.0f);
        float t_clamped = std::clamp(t, 0.0f, 1.0f); // t must be in [0, 1]

        for (int i = 0; i <= n; ++i) {
            long double bernstein = nCr(n, i) * pow(1.0 - t_clamped, n - i) * pow(t_clamped, i);
            point += keys[i].lookAt * (float)bernstein;
        }
        return point;
    }


private:
    // Linear interpolation helpers
    static float lerp(float a, float b, float alpha) { return a + alpha * (b - a); }
    static glm::vec3 lerp(const glm::vec3 &a, const glm::vec3 &b, float alpha) { return a + alpha * (b - a); }

public:
    AnimationState update(float t) const {
        AnimationState state;

        // Camera Interpolation
        if (cameraKeys.size() >= 2) { 
            float camStart = cameraKeys.front().t;
            float camEnd = cameraKeys.back().t;
            float clamped_t = std::clamp(t, camStart, camEnd);

            float alpha = 0.0f;
            float dt = camEnd - camStart;
            if (dt != 0) {
                alpha = (clamped_t - camStart) / dt;
            }

            // Interpolate 'eye' and 'lookAt' using the Bezier function
            state.camera.eye = bezier(cameraKeys, alpha);
            state.camera.lookAt = bezierLookAt(cameraKeys, alpha);
            
        
            size_t i = 0;
            // Find key 'i' such that t is between key[i].t and key[i+1].t
            for (size_t j = 0; j < cameraKeys.size() - 1; ++j) {
                if (clamped_t >= cameraKeys[j].t && clamped_t <= cameraKeys[j+1].t) {
                    i = j;
                    break;
                }
            }
            const CameraKey& p1 = cameraKeys[i];
            const CameraKey& p2 = cameraKeys[i+1];
            
            float segment_dt = p2.t - p1.t;
            float segment_alpha = (segment_dt == 0) ? 0.f : (clamped_t - p1.t) / segment_dt;

            state.camera.up = glm::normalize(lerp(p1.up, p2.up, segment_alpha)); 
            state.camera.t = clamped_t;

        } else if (!cameraKeys.empty()) {
            // Fallback for only 1 keyframe
            state.camera = cameraKeys.front();
        }

        // Scene Interpolation
        if (!sceneKeys.empty()) {
            float animStart = sceneKeys.front().t;
            float animEnd = sceneKeys.back().t;
            float clamped_t = std::clamp(t, animStart, animEnd);

           // Find the correct segment
            size_t i = 0;
            for (i = 0; i < sceneKeys.size() - 1; ++i) {
                if (clamped_t < sceneKeys[i+1].t) {
                    break;
                }
            }
            
            // s0 is the key at index 'i', s1 is the next key
            SceneKey s0 = sceneKeys[i];
            // If 'i' is the last key, just use it for s1 as well
            SceneKey s1 = (i + 1 < sceneKeys.size()) ? sceneKeys[i+1] : s0;

            // Calculate alpha for this segment
            float dt = s1.t - s0.t;
            float alpha = (dt == 0) ? 0.f : (clamped_t - s0.t) / dt;
            
            state.scene.t = clamped_t;
            state.scene.lowerArmPitch = lerp(s0.lowerArmPitch, s1.lowerArmPitch, alpha);
            state.scene.lowerArmYaw   = lerp(s0.lowerArmYaw,   s1.lowerArmYaw,   alpha);
            state.scene.upperArmPitch = lerp(s0.upperArmPitch, s1.upperArmPitch, alpha);
            state.scene.upperArmYaw   = lerp(s0.upperArmYaw,   s1.upperArmYaw,   alpha);
            state.scene.handPitch     = lerp(s0.handPitch,     s1.handPitch,     alpha);
            state.scene.handYaw       = lerp(s0.handYaw,       s1.handYaw,       alpha);
            state.scene.handRoll      = lerp(s0.handRoll,      s1.handRoll,      alpha);
            state.scene.gripperOpen   = lerp(s0.gripperOpen,   s1.gripperOpen,   alpha);
        
            // Added new
            state.scene.carPos = lerp(s0.carPos, s1.carPos, alpha);
            state.scene.carYaw = lerp(s0.carYaw, s1.carYaw, alpha);

            state.scene.light0On   = s0.light0On;
            state.scene.light1On   = s0.light1On;
            state.scene.toyLightOn = s0.toyLightOn;
        }

        return state;
    }
};