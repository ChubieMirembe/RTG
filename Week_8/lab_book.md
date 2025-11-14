# Vulkan Lab 7: Cube Mapping and Particle Systems


### EXERCISE 1:  IMPLEMENTING AND CONTROLLING THE DEPTH BUFFER

**Solution:** 
I had already implement the depth buffer in a previous lab. So to complete this exercise, I review my implementation to make sure it aligned with the snippets you
have provided. I created a lambda function to toggle the depth buffer settings so I coould see the difference when rendering. The reason a lambda function is used
is to encapsulate the depth buffer control logic in a concise manner, allowing for easy toggling of depth testing features without cluttering the main rendering code. 
This approach enhances code readability and maintainability.

- Depth Buffer Control Flag:
```cpp
static constexpr bool USE_DEPTH = true;
```

- Default Depth Stencil State Creation Info:

```cpp
VkPipelineDepthStencilStateCreateInfo depth{};
depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
depth.depthTestEnable  = USE_DEPTH ? VK_TRUE : VK_FALSE;
depth.depthWriteEnable = USE_DEPTH ? VK_TRUE : VK_FALSE;
depth.depthCompareOp   = VK_COMPARE_OP_LESS;
depth.depthBoundsTestEnable = VK_FALSE;
depth.stencilTestEnable     = VK_FALSE;

```

**Output:**

- Depth Buffer Enabled

![](images/ex1_1.png)

- Depth Buffer Disabled
- ![](images/ex1_2.png)

**Reflection:**
This exercise reinforced my understanding of depth buffering in Vulkan. By implementing a toggle for depth testing, I could visually observe the impact of depth buffering on rendering.
It also put me on the path of making my rendering engine more flexible and configurable, which is crucial for developing complex graphics applications.

---

### EXERCISE 2: IMPLEMENTING THE SKYBOX

**Solution:** 
In this exercise, I refactored the rendering pipeline from the previous lab to support a single cubemap-based skybox instead of multiple textured objects.
I removed the two extra 2D samplers and updated the descriptor set layout to include only a uniform buffer (binding 0) and a cubemap sampler (binding 1).
The graphics pipeline was simplified to use static depth testing with VK_COMPARE_OP_LESS_OR_EQUAL and depth writes disabled, ensuring the skybox always
renders correctly behind all geometry.

I converted the texture-loading routine for the third sampler into a cubemap loader, which reads six images representing the skybox faces and creates a 
VK_IMAGE_VIEW_TYPE_CUBE image view. The fragment shader was replaced with a single samplerCube lookup, while the vertex shader removes camera translation
to keep the cube centered around the viewer.

To make the scene interactive, I implemented a keyboard-controlled camera. The handleInput() function processes W/A/S/D for horizontal movement, Q/E for
vertical movement, and arrow keys for yaw and pitch rotation. These inputs update the camera position and orientation every frame, and the new view matrix
is written to the uniform buffer so that the skybox view reacts smoothly to user movement.

- Skybox Depth Stencil State:

```c++
VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = USE_DEPTH ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = VK_FALSE; // USE_DEPTH ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth.depthBoundsTestEnable = VK_FALSE;
    depth.stencilTestEnable = VK_FALSE;

```

- Cull Front Faces for Skybox:

```c++ 
rs.cullMode  = VK_CULL_MODE_FRONT_BIT;
```
**Output:**

![](images/ex2_1_.png)
![](images/ex2_2_.png)

**Reflection:**
During this exercise, I encountered an issue where the window opened and closed immediately after launch. After reviewing my code, I discovered that I 
had mistakenly left in the texture-loading and rendering logic from the previous lab while introducing the cubemap skybox. This created conflicts in 
resource allocation because the old descriptors and samplers no longer matched the new descriptor set layout. Once I removed all references to the previous 
textures and ensured that only the cubemap and uniform buffer were active, the application initialized correctly, and the skybox displayed as intended.

This experience taught me how critical careful resource management is in Vulkan. Even a small mismatch between descriptors, pipelines, or shaders can 
prevent a program from running. It also helped me appreciate how important it is to keep each rendering stage consistent and well organized.

By focusing only on the cubemap, I also learned more about how the skybox works in relation to the view matrix. The camera’s translation is removed so 
the skybox remains centered around the viewer, which creates the illusion of an infinite environment. Once I implemented keyboard movement, I could move 
freely within the scene, and this demonstrated how changes in camera position and orientation directly influence what is drawn each frame.

Overall, this exercise strengthened my understanding of scene setup, pipeline configuration, and resource cleanup. It showed how attention to detail is
essential when modifying an existing rendering system and how correct management of resources ensures that only the intended elements appear in the final 
sce

---

### EXERCISE 3:  

**Solution:** 
I kept one `createGraphicsPipeline` function but created two different pipeline state objects for the skybox and reflective meshes.
One was resonsible for the skybox rendering with front-face culling and depth writes disabled, while the other handled the reflective objects.
The shaders were modified to include a uniform flag indicating whether the current draw call was for the skybox or a reflective mesh.
I computed the world space normal and view direction and then calculated the reflection vector using GLSL's built-in reflect function.
As the camera moves around, the reflection vector updates accordingly, allowing the reflective objects to accurately mirror the skybox environment.

- Pipeline States:

```cpp
// skybox: see interior faces and do not write depth
VkPipelineRasterizationStateCreateInfo rsSky = rsCommon;
rsSky.cullMode = VK_CULL_MODE_FRONT_BIT;   // or VK_CULL_MODE_NONE
VkPipelineDepthStencilStateCreateInfo dzSky{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
dzSky.depthTestEnable  = VK_TRUE;
dzSky.depthWriteEnable = VK_FALSE;
dzSky.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

// reflective mesh: solid geometry with depth writes
VkPipelineRasterizationStateCreateInfo rsRefl = rsCommon;
rsRefl.cullMode = VK_CULL_MODE_BACK_BIT;
VkPipelineDepthStencilStateCreateInfo dzRefl{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
dzRefl.depthTestEnable  = VK_TRUE;
dzRefl.depthWriteEnable = VK_TRUE;
dzRefl.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
````

- Fragment Shader:

```cpp
void main() {
    if (pc.unlit != 0u) {
        vec3 dir = normalize(vDir);
        vec3 col = texture(uSky, dir).rgb;
        outColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);
        return;
    }

    // Reflective object pass
    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(ubo.eyePos.xyz - vWorldPos);  
    vec3 I = -V;                                    

    // Perfect mirror reflection
    vec3 R = reflect(I, N);

    // Sample cubemap with reflection vector
    vec3 col = texture(uSky, R).rgb;

    // Optional: keep gamma correction
    outColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);
}
```

- Vertex Shader:

```cpp
void main() {
    mat4 M = (pc.useOverride != 0u) ? pc.modelOverride : ubo.model;

    vec4 world = M * vec4(inPos, 1.0);
    vWorldPos = world.xyz;
    vWorldNormal = normalize(mat3(M) * inNormal);

    mat4 V = ubo.view;
    if (pc.unlit != 0u) {
        // Skybox: no translation, depth at far plane
        mat4 Vsky = V;
        Vsky[3] = vec4(0.0, 0.0, 0.0, 1.0);
        vDir = mat3(inverse(Vsky)) * world.xyz;
        vec4 clip = ubo.proj * Vsky * vec4(world.xyz, 1.0);
        gl_Position = vec4(clip.xy, clip.w, clip.w);
    } else {
        // Meshes: full view matrix, normal depth
        vDir = mat3(inverse(V)) * world.xyz;
        gl_Position = ubo.proj * V * vec4(world.xyz, 1.0);
    }
}
```

**Output:**
![](images/ex3.png)

**Reflection:**
Working through this exercise is reinforcing that I need to make my code more modular. As this would make it easier
to add features like multiple pipelines without having to rewrite large sections of code. I was able to complete
this exercise without having to completely refactor, but for future projects I plan on investing time into building
a more flexible rendering architecture from the start. This will help me avoid issues where different objects 
compete for the same pipeline settings.

---

### EXERCISE 4: 

**Solution:** 
To create the refractive effect, I modified the fragment shader to compute the refraction vector using GLSL's built-in refract function.
The cube shader now calculates the view direction and normal in world space, then applies Snell's law to determine how light bends
as it passes through the refractive material. I set the index of refraction (eta) to 1.33, simulating water. A blend factor based on the Fresnel equations
was also implemented to mix the refracted color with a slight tint, enhancing realism. Finally, I enabled alpha blending in the pipeline to allow for transparency effects.

- Fragment Shader:

```cpp
void main() {
    if (pc.unlit != 0u) {
        vec3 dir = normalize(vDir);
        vec3 col = texture(uSky, dir).rgb;
        outColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);
        return;
    }

    // Refractive cube
    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(ubo.eyePos.xyz - vWorldPos);

    vec3 I = -V;                         
    float eta = 1.0 / 1.33;              
    vec3 T = refract(I, N, eta);         

    vec3 col = texture(uSky, T).rgb;

    float F0 = 0.04;
    float F = F0 + (1.0 - F0) * pow(clamp(1.0 - dot(N, V), 0.0, 1.0), 5.0);
    col = mix(col * 0.8, col, F);

    col = pow(col, vec3(1.0 / 2.2));
    float alpha = 0.4;                   

    outColor = vec4(col, alpha);
}
```
- Blend State:

```cpp
VkPipelineColorBlendAttachmentState cba{};
cba.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;

cba.blendEnable         = VK_TRUE;
cba.srcColorBlendFactor = VK_SRC_ALPHA;
cba.dstColorBlendFactor = VK_ONE_MINUS_SRC_ALPHA;
cba.colorBlendOp        = VK_BLEND_OP_ADD;
cba.srcAlphaBlendFactor = VK_ONE;
cba.dstAlphaBlendFactor = VK_ONE_MINUS_SRC_ALPHA;
cba.alphaBlendOp        = VK_BLEND_OP_ADD;

```
**Output:**
![](images/ex4.png)

**Reflection:**
This exercise was particularly challenging as it required a solid understanding of both the mathematical principles behind refraction and the practical implementation in GLSL.
Implementing Snell's law and the Fresnel equations in the shader code was a great learning experience, as it deepened my understanding of how light interacts with different materials.
Enabling alpha blending also introduced me to the complexities of rendering transparent objects in Vulkan, which is not as straightforward as opaque rendering.

---


### EXERCISE 5:  

**Solution:** 


**Output:**



**Reflection:**


