# Hierarchical Modeller — Submission-ready

## Team
- Sarthak Thakare 
- Shivam Singh Yadav

## Declaration
We confirm the code is written by us. We consulted:
- OpenGL/GLFW/GLEW docs and tutorials
- ChatGPT for code structure suggestions

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
- Use '+' / '=' to Increase and '-' to Decrease
- C — Change color for current shape (prompts R G B)
- S — Save model (.mod) (prompts filename)
- L — Load model (.mod) (only in Inspect mode)
- Esc — Exit

## .mod format
Indented text representing hierarchy, each line:
```
<shape_name> <tess_level> r,g,b,a tx,ty,tz sx,sy,sz
```
Example included in `models/`.

## Submission contents
- source files (include/, src/)
- shaders/
- models/ (example .mod files)
- snapshots/ (PNG screenshots of two models)
