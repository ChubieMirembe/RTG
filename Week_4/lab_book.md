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
```c++

```
```c++

```

**Output:**
1. Single Cube Rotation:
![](Images/ex2_single_cube.png)

2. Dual Cube Rotation:
![](Images/ex2_dual_cube.png)

1. **Reflection:**


### EXERCISE 3: PROCEDURAL CYLINDER
#### Goal: Procedurally generate and render a cylinder mesh.

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
