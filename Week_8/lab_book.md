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


**Output:**



**Reflection:**
I made the mistake of rendering the textures from the previous lab alongside the skybox. The caused my window to just open and close immediately. 
After fixing this mistake, I was able to see the skybox rendered correctly. This exercise helped me understand the importance of management 
of resources to ensure that only the intended elements are rendered in a scene.


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


