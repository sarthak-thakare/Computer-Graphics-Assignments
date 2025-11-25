# Hierarchical Toy Model - Robot Arm

## Team
- Sarthak Thakare - 23b0933
- Shivam Singh Yadav - 23b1007

## Declaration
We confirm the code is written by us. We consulted:
- OpenGL/GLFW/GLEW docs and tutorials
- https://learnopengl.com/
- Course Coding tutorials
- https://github.com/paragchaudhuri/cs475-tutorials
- Google Gemini — used as an aid for debugging suggestions and generating small boilerplate snippets

## Project Description
This project implements a hierarchical robot arm toy with multiple movable joints for computer graphics modeling and animation.

## Hierarchical Structure

```
Root (Model Base)
│
└── Base (Sphere - Yellow)
    │   - Dimensions: 0.5 x 0.3 x 0.5
    │   - Color: RGB(0.9, 0.8, 0.2)
    │   - Purpose: Base platform of the robot
    │
    └── Lower Arm (Cylinder - Red) **[2 DOF Joint]**
        │   - Dimensions: 0.15 x 0.8 x 0.15
        │   - Color: RGB(0.9, 0.2, 0.2)
        │   - DOF: Rotation around X axis (pitch), Y axis (rotate)
        │   - Translation: (0, 0.8, 0)
        │
        └── Middle Joint (Sphere - Dark Gray)
            │   - Dimensions: 0.2 x 0.2 x 0.2
            │   - Color: RGB(0.3, 0.3, 0.3)
            │   - Purpose: Connection joint
            │   - Translation: (0, 1.5, 0)
            │
            └── Upper Arm (Cylinder - Blue) **[2 DOF Joint]**
                │   - Dimensions: 0.12 x 0.8 x 0.12
                │   - Color: RGB(0.2, 0.4, 0.9)
                │   - DOF: Rotation around X axis (pitch), Y axis (rotate)
                │   - Translation: (0, 0.8, 0)
                │
                └── Wrist Joint (Sphere - Dark Gray)
                    │   - Dimensions: 0.18 x 0.18 x 0.18
                    │   - Color: RGB(0.3, 0.3, 0.3)
                    │   - Purpose: Wrist connection
                    │   - Translation: (0, 1.5, 0)
                    │
                    └── Hand (Box - Green) **[3 DOF Joint]**
                        │   - Dimensions: 0.3 x 0.2 x 0.2
                        │   - Color: RGB(0.2, 0.8, 0.3)
                        │   - DOF: Rotation around X, Y, Z axes (full 3D rotation)
                        │   - Translation: (0, 0.3, 0)
                        │
                        ├── Gripper Left (Box - Orange) **[1 DOF Joint]**
                        │   - Dimensions: 0.08 x 0.3 x 0.08
                        │   - Color: RGB(1.0, 0.6, 0.2)
                        │   - DOF: Translation along X axis (open/close)
                        │   - Base Translation: (-0.15, 0.15, 0)
                        │
                        └── Gripper Right (Box - Orange) **[1 DOF Joint]**
                            - Dimensions: 0.08 x 0.3 x 0.08
                            - Color: RGB(1.0, 0.6, 0.2)
                            - DOF: Translation along X axis (open/close)
                            - Base Translation: (0.15, 0.15, 0)
```

## Movable Body Parts (Requirements Met)

### Total Movable Parts: 5
1. **Lower Arm** (2 DOF) - Pitch and Rotation
2. **Upper Arm** (2 DOF) - Pitch and Rotation  
3. **Hand** (3 DOF) - Full 3D rotation (X, Y, Z)
4. **Left Gripper** (1 DOF) - Open/Close
5. **Right Gripper** (1 DOF) - Open/Close

### Multi-DOF Joints: 3
1. **Lower Arm Joint** - 2 DOF (X rotation pitch, Y rotation)
2. **Upper Arm Joint** - 2 DOF (X rotation pitch, Y rotation)
3. **Hand/Wrist Joint** - 3 DOF (X, Y, Z rotation)

✅ **Requirement Met**: At least 4 movable body parts with at least 2 joints having more than 1 DOF

## Controls

### Robot Mode (Default, Press 'P' to switch)

#### Lower Arm Controls (2 DOF)
- **↑ (Up Arrow)**: Lower arm pitch up (rotate around X+)
- **↓ (Down Arrow)**: Lower arm pitch down (rotate around X-)
- **← (Left Arrow)**: Lower arm rotate left (rotate around Y+)
- **→ (Right Arrow)**: Lower arm rotate right (rotate around Y-)

#### Upper Arm Controls (2 DOF)
- **W**: Upper arm pitch up (rotate around X+)
- **S**: Upper arm pitch down (rotate around X-)
- **A**: Upper arm rotate left (rotate around Y+)
- **D**: Upper arm rotate right (rotate around Y-)

#### Hand Controls (3 DOF)
- **Q**: Hand pitch up (rotate around X+)
- **E**: Hand pitch down (rotate around X-)
- **Z**: Hand yaw left (rotate around Y+)
- **C**: Hand yaw right (rotate around Y-)
- **1**: Hand roll counter-clockwise (rotate around Z+)
- **2**: Hand roll clockwise (rotate around Z-)

#### Gripper Controls (1 DOF each)
- **O**: Open gripper (spread grippers apart)
- **L**: Close gripper (bring grippers together)

### Mode Switching
- **P**: Switch to ROBOT mode (toy model manipulation)
- **M**: Switch to MODELLER mode (interactive modeling from Assignment 1)
- **I**: Switch to INSPECT mode (view saved models from Assignment 1)
- **H**: Show help/controls
- **ESC**: Exit application

### Modeller Mode (Press 'M')
- **1**: Add Sphere (prompts tess level)
- **2**: Add Box
- **3**: Add Cylinder
- **4**: Add Cone
- **5**: Remove last shape
- **R**: Enter rotate mode
- **T**: Enter translate mode
- **G**: Enter scale mode
- **X/Y/Z**: Choose axis
- **+/-**: Apply transformation
- **C**: Change color for current shape (prompts R G B)
- **S**: Save model (.mod)

### Inspect Mode (Press 'I')
- **L**: Load model (.mod) 
- **R**: Rotate entire model
- **X/Y/Z**: Choose axis
- **+/-**: Apply rotation

## Building and Running

```bash
make
./model_generator
```

Press **H** for in-app help.

## Dependencies
- OpenGL
- GLEW
- GLFW3
- GLM
- pkg-config

Ubuntu installation:
```bash
sudo apt install build-essential libglfw3-dev libglew-dev libglm-dev pkg-config
```

## Shape Classes Used
- `sphere_t`: Base, joints (with varying tessellation levels)
- `cylinder_t`: Arms (lower and upper)
- `box_t`: Hand, grippers
- `cone_t`: Available but not used in robot model

## Design Decisions

1. **Color Coding**: Each part has a distinct color for easy identification
   - Base: Yellow (stable platform)
   - Lower Arm: Red (primary actuator)
   - Upper Arm: Blue (secondary actuator)
   - Joints: Dark Gray (connection points)
   - Hand: Green (end effector)
   - Grippers: Orange (manipulators)

2. **Joint Structure**: Spheres used as visual joint indicators between arm segments for realistic appearance

3. **Gripper Design**: Simple linear motion for open/close functionality, controlled by single parameter

4. **Scale Hierarchy**: Parts decrease in size as we move up the arm for realistic proportions

5. **Coordinate System**: 
   - Y-axis: Up direction (arms extend upward)
   - X-axis: Left-right (gripper open/close)
   - Z-axis: Forward-backward

## Files Structure

```
├── include/
│   ├── box.hpp
│   ├── cone.hpp
│   ├── cylinder.hpp
│   ├── model.hpp
│   ├── robot_arm.hpp      [NEW - Robot arm class]
│   ├── shape.hpp
│   └── sphere.hpp
├── src/
│   ├── box.cpp
│   ├── cone.cpp
│   ├── cylinder.cpp
│   ├── main.cpp           [MODIFIED - Added robot mode]
│   ├── model.cpp          [MODIFIED - Fixed color saving]
│   ├── robot_arm.cpp      [NEW - Robot implementation]
│   └── sphere.cpp
├── shaders/
│   ├── basic.frag
│   └── basic.vert
├── models/
│   ├── car.mod
│   └── human.mod
├── glm/                   [GLM library headers]
├── Makefile
└── README.md
```

## Animation Ready
The hierarchical structure is designed for animation with clear parent-child relationships. All joint parameters can be keyframed for animation in future assignments. The hierarchy will remain unchanged for the animation assignment.

## Assignment Requirements Checklist

- ✅ Hierarchical model created using shape classes from previous assignment
- ✅ At least 4 movable body parts (5 implemented: 2 arms, hand, 2 grippers)
- ✅ At least 2 joints with more than 1 DOF (3 implemented: lower arm 2-DOF, upper arm 2-DOF, hand 3-DOF)
- ✅ Keys assigned to vary each parameter
- ✅ Colors assigned to each part
- ✅ Hierarchy tree documented in README
- ✅ Structural constraints appropriate for animation
- ✅ Code uses shape class for geometry
- ✅ Code uses HNode for hierarchy nodes
