# Assignment 1 - Modelling (Submission Ready)

## Team
- Sarthak Thakare - 23b0933
- Shivam Singh Yadav - 23b1007

## Declaration
We confirm the code is written by us. We consulted:
- OpenGL/GLFW/GLEW docs and tutorials
- https://learnopengl.com/

- Course Coding tutorials
- https://github.com/paragchaudhuri/cs475-tutorials

- OpenGl video tutorials
- https://www.youtube.com/watch?v=45MIykWJ-C4

- Google Gemini — used as an aid for debugging suggestions and generating small boilerplate snippets

## Build
Requires: g++, GLFW3, GLEW, glm, pkg-config
Ubuntu:
```
sudo apt install build-essential libglfw3-dev libglew-dev libglm-dev pkg-config
```
```
make
```
```
./model_genrator
```

## Controls (keyboard)
- M — Modeller mode
- I — Inspect mode
- 1 — Add Sphere (prompts tess level)
- 2 — Add Box
- 3 — Add Cylinder
- 4 — Add Cone
- 5 — Remove last shape
- R — Enter rotate mode (apply to current shape in modeller; model in inspect)
- T — Enter translate mode
- G — Enter scale mode
- X/Y/Z — Choose axis
- Use '+' to Increase and '-' to Decrease
- C — Change color for current shape (prompts R G B)
- S — Save model (.mod) (prompts filename)
- L — Load model (.mod) (only in Inspect mode)
- Esc — Exit

## .mod format
Indented text representing hierarchy, each line:
```
<shape_name> <tess_level> r,g,b,a tx,ty,tz sx,sy,sz,(4x4 rotation matrix)
```
Example included in `models/`.

## Submission contents
- source files (include/, src/)
- shaders/
- models/ (example .mod files)
- snapshots/ (PNG screenshots of two models)
