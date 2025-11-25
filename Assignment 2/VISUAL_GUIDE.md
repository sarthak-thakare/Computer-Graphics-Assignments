## Robot Arm Visual Representation

### Side View (Initial Position)

```
                    ┌───┐ ┌───┐
                    │ O │ │ O │    Grippers (Orange) [1 DOF each]
                    └─┬─┘ └─┬─┘
                      └──┬──┘
                    ┌────┴────┐
                    │  HAND   │    Hand (Green Box) [3 DOF]
                    └────┬────┘
                         ●         Wrist Joint (Gray Sphere)
                         │
                         │
                    ┌────┴────┐
                    │  UPPER  │    Upper Arm (Blue Cylinder) [2 DOF]
                    │   ARM   │
                    └────┬────┘
                         ●         Middle Joint (Gray Sphere)
                         │
                         │
                    ┌────┴────┐
                    │  LOWER  │    Lower Arm (Red Cylinder) [2 DOF]
                    │   ARM   │
                    └────┬────┘
                    
                      ╱───╲
                     │BASE │     Base (Yellow Sphere - squashed)
                      ╲───╱
```

### Top View (Showing Gripper Open/Close)

```
Gripper Closed (gripperOpen = 0.0):

              ┌───┐
              │ H │  Hand
              └─┬─┘
            ┌───┼───┐
            │ L │ R │  Left & Right Grippers (close together)
            └───┴───┘


Gripper Open (gripperOpen = 0.3):

              ┌───┐
              │ H │  Hand
              └─┬─┘
        ┌───┐   │   ┌───┐
        │ L │   │   │ R │  Left & Right Grippers (spread apart)
        └───┘       └───┘
```

### Example Poses

#### 1. Rest Position (Default)
```
All angles = 0, robot stands straight up
Grippers closed

         ┌─┬─┐
         └─┘
          ●
    ┌─────┴─────┐
    │   BLUE    │
    └─────┬─────┘
          ●
    ┌─────┴─────┐
    │    RED    │
    └─────┬─────┘
       ╱───╲
      │YELLOW│
       ╲───╱
```

#### 2. Reaching Forward
```
Lower arm pitched forward, upper arm extends

         ┌─┬─┐
         └─┘     Hand →
          ●
    ┌────┴──────
    │   BLUE   
    └────┬──────
          ●
    ┌────┴──────
    │    RED    
    └────┬──────
       ╱───╲
      │YELLOW│
       ╲───╱
```

#### 3. Waving Position
```
Lower arm rotated, upper arm bent

              ┌─┬─┐
              └─┘
               ●
         ┌─────┴─────┐
         │   BLUE    │
         └─────┬─────┘
               ●
          ╱────┴
         ╱  RED
    ╱───┴─────
     ╱───╲
    │YELLOW│
     ╲───╱
```

### Color Palette

```
 ╔══════════╗
 ║  YELLOW  ║  Base        - RGB(0.9, 0.8, 0.2)
 ╚══════════╝

 ╔══════════╗
 ║   RED    ║  Lower Arm   - RGB(0.9, 0.2, 0.2)
 ╚══════════╝

 ╔══════════╗
 ║   GRAY   ║  Joints      - RGB(0.3, 0.3, 0.3)
 ╚══════════╝

 ╔══════════╗
 ║   BLUE   ║  Upper Arm   - RGB(0.2, 0.4, 0.9)
 ╚══════════╝

 ╔══════════╗
 ║  GREEN   ║  Hand        - RGB(0.2, 0.8, 0.3)
 ╚══════════╝

 ╔══════════╗
 ║  ORANGE  ║  Grippers    - RGB(1.0, 0.6, 0.2)
 ╚══════════╝
```

### Dimensions Summary

```
Part            Type       Width  Height  Depth
──────────────────────────────────────────────────
Base            Sphere     0.50   0.30    0.50
Lower Arm       Cylinder   0.15   0.80    0.15
Middle Joint    Sphere     0.20   0.20    0.20
Upper Arm       Cylinder   0.12   0.80    0.12
Wrist Joint     Sphere     0.18   0.18    0.18
Hand            Box        0.30   0.20    0.20
Gripper Left    Box        0.08   0.30    0.08
Gripper Right   Box        0.08   0.30    0.08
```

### Movement Ranges

```
Joint               DOF  Axis  Range      Control
────────────────────────────────────────────────────
Lower Arm           2    X     ±∞        ↑/↓
                         Y     ±∞        ←/→

Upper Arm           2    X     ±∞        W/S
                         Y     ±∞        A/D

Hand                3    X     ±∞        Q/E
                         Y     ±∞        Z/C
                         Z     ±∞        1/2

Gripper (both)      1    X     0-0.3     O/L
```

### Practical Movement Examples

1. **Pick Up Object**
   - Lower arm: Pitch down (↓)
   - Upper arm: Pitch down (S)
   - Hand: Pitch forward (E)
   - Gripper: Open (O), then Close (L)

2. **Place Object**
   - Move to position using arm controls
   - Gripper: Open (O)

3. **Wave Hello**
   - Lower arm: Rotate left and right (← → repeatedly)
   - Upper arm: Slight pitch up (W)

4. **Inspect Object**
   - Hand: Rotate around all axes (Q/E, Z/C, 1/2)
   - Gripper: Keep closed (L)

This robot arm provides excellent visual feedback with distinct colors and smooth, intuitive control for all joints!
