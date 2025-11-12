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

**Output:**


**Reflection:**


---

### EXERCISE 4: 

**Solution:** 


**Output:**



**Reflection:**



### EXERCISE 5:  

**Solution:** 


**Output:**



**Reflection:**


