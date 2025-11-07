# Animation - Robot Arm (Assignment 3)

## Team
- Sarthak Thakare — 23b0933
- Shivam Singh Yadav — 23b1007

## Declaration
We declare that we have written the assignment code ourselves. We took help from the following sources for reference, debugging tips, and small boilerplate snippets:
- OpenGL/GLFW/GLEW/GLM documentation and tutorials
  - GLFW: https://www.glfw.org/docs/latest/
  - GLEW: http://glew.sourceforge.net/
  - GLM: https://glm.g-truc.net/0.9.9/index.html
- LearnOpenGL: https://learnopengl.com/
- Course coding tutorials: https://github.com/paragchaudhuri/cs475-tutorials
- Video tutorial: https://www.youtube.com/watch?v=45MIykWJ-C4
- Google Gemini — used for debugging suggestions and small boilerplate only

## Build and Run
Requirements: g++, GLFW3, GLEW, GLM, pkg-config (Linux)

Ubuntu setup:
```
sudo apt install -y build-essential libglfw3-dev libglew-dev libglm-dev pkg-config
```
Build:
```
make
```
Run:
```
./model
```
Assets: images/ (BMP textures), models/ (car.mod, human.mod), shaders/ already included. The app expects to be run from the repo root so it can find these relative paths.

## Demo Video
Watch the assignment demo here:

https://www.youtube.com/watch?v=j1uYJONGKC0

## Keymap

### Animation Controls
- P: Play/Pause animation
- R: Record frames to snapshots/frame-XXXXX.tga (starts from time 0)
- T: Set time value for the next keyframe
- - / =: Scrub animation backward / forward by 1 frame
- L: Load all keyframes from camera.key and scene.key
- S: Save all keyframes (Scene + Camera) to files
- C: Save Camera-ONLY key at current time
- Ctrl + C: Save Scene + Camera key at current time
- Shift + C: Save Camera Trajectory ONLY (camera.key)

### Camera Controls (Scene mode)
- I / K: Move forward / backward
- , / .: Strafe left / right
- [ / ]: Move down / up
- PageUp / PageDown: Pan left / right (yaw)
- Home / End: Tilt up / down (pitch)
- V: Toggle camera mode (Scene / Follow-Hand)

### Robot Arm Controls
- Lower Arm (Red cylinder) — 2 DOF
  - ↑/↓: Pitch ±X
  - ←/→: Yaw/Rotate ±Y
- Upper Arm (Blue cylinder) — 2 DOF
  - W/F: Pitch ±X
  - A/D: Yaw/Rotate ±Y
- Hand (Green box) — 3 DOF
  - Q/E: Pitch ±X
  - Z/Y: Yaw ±Y
  - 1/2: Roll ±Z
- Gripper (Orange boxes)
  - O/B: Open / Close

### Lights
- 8/9: Toggle scene lights L0/L1
- 0: Toggle toy light attached to hand tip

### Help & Exit
- H: Show controls in console
- ESC: Exit application


## Hierarchical Tree (with labeled parameters)
```
Root (Model)
└─ Base (Box, Yellow)
   └─ LowerArm Joint [pitch X, yaw Y]
      ├─ Joint Sphere (visual)
      ├─ Lower Arm (Cylinder, Red)  <-- techno.bmp
      └─ UpperArm Joint [pitch X, yaw Y]
         ├─ Joint Sphere (visual)
         ├─ Upper Arm (Cylinder, Blue)  <-- techno.bmp
         └─ Wrist Joint / Hand Joint
            ├─ Wrist Sphere (visual)
            └─ Hand (Box, Green) [pitch X, yaw Y, roll Z]  <-- techno01.bmp
               ├─ Gripper Left (Box, Orange)  [translate X]  <-- metal.bmp
               └─ Gripper Right (Box, Orange) [translate X]  <-- metal.bmp
```
Parameters exposed (animation-ready):
- lowerArmRotX, lowerArmRotY
- upperArmRotX, upperArmRotY
- handRotX, handRotY, handRotZ
- gripperOpen (maps to ±X translation of grippers)

## Snapshots (Two Cameras)
Scene Camera view:

![Environment view](snapshots_models/env.png)

Follow-Hand Camera view:

![Robot arm close-up](snapshots_models/robo_arm.png)

## Short Scene Story (for next assignment)
In a small workshop, a compact robot arm practices pick-and-place tasks. It picks items from a wooden table behind it, inspects them under the hand-mounted light, and then places them near a parked toy car. The scene plays like a one-minute assembly micro-routine: pick from table → inspect → rotate → place by the car.

## Project Overview
This project implements a hierarchical robot arm toy with 5 movable parts and 9 DOF total. It features a textured room, a wooden table, and two example models (human and car) arranged around the arm.

## Additions Beyond Requirements
- Textured environment:
  - Floor (wood), walls/ceiling (brick), platform (metal)
- Robot texturing details:
  - Lower and Upper Arms: techno.bmp
  - Hand base box (where grippers attach): techno01.bmp (fallback to techno.bmp)
  - Grippers: metal.bmp
- Additional models placed in scene:
  - human.mod and car.mod loaded, uniformly scaled by 0.5 at the root to preserve joint integrity
  - Automatic floor placement using world AABB to avoid sinking into the floor
- Furniture:
  - Wooden table (wooden.bmp) placed behind the robot
- Cameras:
  - Scene camera and Follow-Hand camera
- Lighting:
  - Two scene lights plus a hand-attached toy light
- Minor improvements:
  - Color saving fix in model IO
  - Per-node texture toggle with UVs in all shapes

Additional implementation notes:
- Camera path visualization includes control points (red spheres), the control polygon (red line), and a smooth Bezier spline (yellow line).
- Animation files: camera.key and scene.key are simple text formats the app reads/writes directly from the repo root.

## File Layout (relevant)
- include/: shape and model headers, robot_arm.hpp
- src/: geometry, model system, main app, robot_arm.cpp
- shaders/: basic.vert, basic.frag (Gouraud + texture modulation)
- models/: human.mod, car.mod
- images/: wood.bmp, wooden.bmp, bricks.bmp, metal.bmp, metal10.bmp, techno.bmp, techno01.bmp
- snapshots/: env.png, robo_arm.png

## Notes
- The modeller/inspect modes from Assignment 1 remain available as legacy features.
- All joint parameters are floats and ready to be keyframed in the animation assignment.
