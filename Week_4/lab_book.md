# Vulkan Lab  - Transformations and Coordinate Systems
## Week 4 - Lab A


### EXERCISE 1:BASIC SCALING TRANSFORMATION
#### Deform the cube you created from Lab 1 into a long, flat plank using a non-uniform scaling transformation.

**Solution:**
The solution for the horizontal, was already provided in the lecture. Below are the code snippets for all three views:
I just modified the scale parameters in the model matrix on the different axes to achieve the desired deformation.
For the horizontal view, I scaled the x-axis more than the y and z axes to create a long plank. For the vertical view,
I scaled the z-axis more than the x and y axes to create a tall plank. For the flat view, I scaled the z-axis very little 
compared to the x and y axes to create a flat shape.

1. Horitontal View:
```c++
glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 0.5f, 0.5f));
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = rotate * modelMatrix;
    ubo.view = glm::lookAt(glm::vec3(6.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 10.0f));
```

2. Vertical View:
```c++
glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 2.0f));
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = rotate * modelMatrix;
    ubo.view = glm::lookAt(glm::vec3(6.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 10.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
```

3. Flat View:
```c++
UniformBufferObject ubo{};
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.1f));
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = rotate * modelMatrix;
    ubo.view = glm::lookAt(glm::vec3(6.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 10.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
```
**Output:**
1. Horizontal View:
![](Images/ex1_horizontal_view.png))

2. Vertical View:
![](Images/ex1_vertical_view.png)

3. Flat View:
![](Images/ex1_flat_view.png)

**Reflection:**
During this exercise, I learned how to apply non-uniform scaling transformations to deform a cube into different shapes.
I continued to have understanding how to "lookat" the object. But by the end, I understood I could manipulate the way the
model was being rendered by changeing the radians in the rotation matrix. This exercise helped me understand how scaling 
transformations work in 3D graphics and how they can be used to create different shapes from a basic cube.

### EXERCISE 2:HIERARCHICAL TRANSFORMATIONS
#### Goal: Apply scaling, translation, and rotation transformations to achieve the following visual outcomes

**Solution:**
I already had the scaling transformation from Exercise 1, where I stretched the cube into a tall, thin pillar at the origin, so I began 
Exercise 2 by focusing on the rotation and translation needed for the orbiting cube. I reused the scaled pillar from Exercise 1 as the 
central object and created a second model matrix for the smaller cube. Each frame, I updated two uniform buffers—one for the pillar and 
one for the orbiting cube—and in the command buffer, I bound the two descriptor sets and issued separate draw calls so both objects would 
render with their respective transformations. In `updateUniformBuffer()`, I changed the transformation order to rotation * translation, 
as the other way round was just causing the cube to rotate in place. So when I corrected this, the cube rotate around the pillar, creating 
a proper circular orbit rather than spinning in place. 

After confirming that the cube’s movement around the pillar was correct, I added depth testing so the cube could pass behind it realistically. 
I introduced new Vulkan resources for depth: a depth image, its memory, and an image view, along with helper functions like `findDepthFormat()` 
and `createDepthResources()`. I integrated depth creation after `createImageViews()` during initialization and swapchain recreation 
and cleaned it up in `cleanupSwapChain()`. Then, in the graphics pipeline, I enabled depth testing and writing with a 
VkPipelineDepthStencilStateCreateInfo block and included the chosen depth format in the dynamic rendering setup. Finally, I modified the 
command buffer recording to transition the depth image and attach it for rendering so it would clear and update each frame. With those changes, 
my existing scaled pillar from Exercise 1 stayed fixed at the origin, the cube orbited smoothly around it, and the depth testing allowed it 
to disappear behind the pillar as it moved through its orbit.

To complete the exercise, I extended the hierarchical transformation scene by adding two tall, scaled pillars and two 
smaller cubes that orbit around them at different speeds. Each pillar was positioned symmetrically along the X-axis, 
while the cubes were given individual rotation rates, directions, and phases to create independent orbital motion around
their respective pillars. I updated the uniform buffers to store five model matrices per frame—one for the ground plane, 
two for the pillars, and two for the orbiting cubes, and issued separate draw calls for each object. This produced a 
dynamic scene where both cubes revolve smoothly around their pillars, correctly passing behind them due to depth testing, 
demonstrating full control over hierarchical transformations and object animation. 

```c++
glm::mat4 pillarModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 3.0f, 0.3f));
```
```c++
glm::mat4 rotation = glm::rotate(glm::mat4(1.0f),
                     time * glm::radians(90.0f),
                     glm::vec3(0.0f, 1.0f, 0.0f));

glm::mat4 translation = glm::translate(glm::mat4(1.0f),
                        glm::vec3(2.0f, 0.0f, 0.0f));

glm::mat4 orbit = rotation * translation;
glm::mat4 cubeModel = orbit * glm::scale(glm::mat4(1.0f), glm::vec3(0.4f));
```
-- Multiple draw calls per frame:
```c++
for (uint32_t i = 0; i < objectCount; ++i) {
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                            0, 1, &descriptorSets[base + i], 0, nullptr);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}
```
-- Writing multiple UBOs per frame:
```c++
const uint32_t base = currentImage * objectCount; // objectCount = e.g. 5

auto writeUBO = [&](uint32_t idx, const glm::mat4& model) {
    UniformBufferObject ubo{};
    ubo.model = model;
    ubo.view  = view;
    ubo.proj  = proj;
    std::memcpy(uniformBuffersMapped[base + idx], &ubo, sizeof(ubo));
};
```
- Transformation Logic
```c++
// Time variable for animation
static auto start = std::chrono::high_resolution_clock::now();
float t = std::chrono::duration<float>(
    std::chrono::high_resolution_clock::now() - start).count();

// Generic orbit function
auto orbitMatrix = [&](float speedDeg, float direction, float phaseDeg) {
    float angle = glm::radians(phaseDeg + direction * speedDeg * t);
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(orbitRadius, 0.0f, 0.0f));
    return rotation * translation; // GLM post-multiply: translate then rotate about origin
};

```

**Output:**
1. Single Towwer and Cube:
![](Images/ex2_single.gif)

2. Double Tower and Cube:
![](Images/ex2_double.png)

**Reflection:**
In this lab, I built upon the scaling transformation from Exercise 1 to implement hierarchical transformations using 
translation, rotation, and scaling matrices in combination. I created two tall, thin pillars positioned symmetrically 
along the X-axis and added two smaller cubes that orbit around them at different speeds. By carefully ordering the 
transformation matrices (rotation * translation), I achieved correct circular motion, demonstrating how matrix
multiplication order directly affects spatial behavior. Implementing separate uniform buffers for each object allowed me 
to manage multiple model matrices independently within the same frame, which was essential for rendering several moving 
objects simultaneously.

A key learning outcome was understanding the hierarchical relationship between objects, the cubes’ motion depended on 
their respective pillar positions, reinforcing the concept of parent-child transformation. Adding depth testing completed 
the visual realism by allowing objects to correctly pass behind one another, showing how the GPU uses depth comparisons 
to handle occlusion. This exercise helped me solidify my understanding of the model-view-projection (MVP) matrix pipeline, 
frame-level uniform buffer management, and real-time animation in Vulkan. It also highlighted the importance of organizing 
transformation logic and resource management when scaling up from a single object to a dynamic scene.

### EXERCISE 3: PROCEDURAL CYLINDER
#### Goal: Procedurally generate and render a cylinder mesh.

**Solution:**

```c++
```
```c++
```
**Solution:**

```c++
```
```c++
```




**Output:**


**Question:**



### EXERCISE 4:  WIREFRAME RENDERING
#### Goal: Refactor the procedural generation code into a reusable C++ class or namespace, similar to the GeometryGenerator provided at d3d12book/Chapter 7 Drawing in Direct3D 
#### Part II at master ? d3dcoder/d3d12book using the procedural geometric models defined in GeometryGenerator.h, GeometryGenerator.cp

**Solution:**
```c++

```
```c++

```
**Output:**


**Reflection:**



### EXERCISE 5: LOADING EXTERNAL MODELS WITH ASSIMP
#### Goal: Integrate the Assimp library into your project to load and render a 3D model from an .obj file.

**Solution:**

```c++
```

**Output:**


**Reflection:**


### FURTHER EXPLORATION 
```c++
```

```c++

```

***Reflection:***
