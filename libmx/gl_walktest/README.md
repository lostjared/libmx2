
Custom Shaders: Uses GLSL shaders for floor, walls, pillars, objects, projectiles, explosions, and UI elements
Animated Textures: Fractal, swirl, and rainbow effects applied to surfaces using time-based shader transformations
Particle System: Up to 500,000 particles for explosive effects with physics simulation
3D Models: Loads custom .mxmod.z model format with texture support
Cross-platform: Supports both desktop OpenGL 3.3 and WebGL (via Emscripten)
Game Environment
Room Structure:
Textured floor with tiling pattern
Thick perimeter walls with inner/outer faces and top caps
15 randomly-placed cylindrical pillars with tapered bases
Dynamic Objects:
Saturn and bird models (10 objects total)
Objects rotate at varying speeds
Can be destroyed by projectiles
Player Mechanics
First-Person Camera: Mouse-look controls with pitch/yaw rotation
Movement: WASD movement with sprint (Shift) and crouch (Ctrl)
Physics: Gravity-based jumping with collision detection
Collision Detection: Player cannot pass through walls or pillars
Combat System
Laser Projectiles:
Fast-moving rectangular bolts
Trailing particle effects
Collides with floor, walls, pillars, and objects
Explosions:
1500+ particle explosions on impact
Different colors (red for environment, orange for objects)
Particles bounce off walls/pillars and floor
Gravity-affected particle physics
UI Elements
Red Crosshair: Center-screen targeting reticle
FPS Counter: Toggle with 'F' key
Active Bullet Count: Real-time projectile tracking
Instructions: On-screen controls display
Controls
Key/Action	Function
WASD	Move forward/backward/strafe
Mouse	Look around
Left Mouse Button	Fire projectile
Spacebar	Jump
Left Shift	Sprint
Left Ctrl	Crouch
F	Toggle FPS display
Escape	Release/capture mouse
Technical Details
Classes
Wall - Generates thick perimeter walls with proper texture tiling
Pillar - Creates cylindrical pillars with tapered bases and collision
Floor - Tessellated ground plane with animated texture effects
Objects - Manages 3D model objects (Saturn/Bird) with destruction
Projectile - Laser bolt system with trailing effects
Explosion - Particle-based explosion system with pooling
Crosshair - Simple 2D UI overlay
Game - Main game logic coordinator
Rendering Pipeline
Clear color and depth buffers
Draw floor with animated shader
Draw walls with culling enabled
Draw pillars
Draw rotating objects
Draw projectiles and trails (depth test disabled)
Draw explosion particles (additive blending)
Draw crosshair overlay (2D)
Draw text UI
Particle System
Pooling: Reuses inactive particles instead of allocating new ones
Physics: Gravity, velocity, collision response with walls/pillars/floor
Rendering: Point sprites with circular fade-out
Cleanup: Removes particles when pool exceeds 80% capacity