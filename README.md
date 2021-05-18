# AGP
## Advanced graphics programming Project2 - Deferred rendering
    by 
    Alejandro-Aurelio Gamarra Niño 
    José Antonio Prieto Garcia
    
Implementation of deferred rendering with light volumes.
- Using screen sized quad for directional lighting pass
- Using spheres volumes for point lighting pass

## Controls
### Camera
- keyboard keys W,A,S,D: navigation
- keyboard key C: 2x camera speed navigation
- mouse click + drag: camera pan
### General
- keyboard key i: show gl related information
- Info window:   
    - Choose between different textures from the Gbuffer using the combobox:
        - position
        - normals
        - albedo
        - depth
        - depth grayscale (used for visualization purposes only)
        - final pass (final scene texture after light pass applied)
## Changelog:
### v0.1 - may 18 2021
First version with deferred shading with lighting volumes
- Decoupled geomery pass and lighting pass.
- Lighting pass uses lighting volumes for directional(screen sized quad), and spheres for point lights.
- Shader programs: 
    - Geometry pass: renders all scene objects on a gBuffer for later process
    - Lighting pass: render screen sized quads or spheres for every light on scene in additive form(fragment shader only process fragments of light volumes)
    - Textured geometry: used for final on screen rendering of the user selected texture from the Info window combobox.
### v0.2 - 
