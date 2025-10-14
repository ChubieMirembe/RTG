# Vulkan Lab 1 - Complex Shapes
## Week 3 - Lab A


### EXERCISE 1: CREATE A FLAT GRID
####  Generate the vertices and indices for a flat grid of arbitrary width and depth, centred at the origin, and render it in wireframe.

**Solution:**
```c++
void createGrid(int width, int depth, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    for (int z = 0; z <= depth; ++z) {
        for (int x = 0; x <= width; ++x) {
            Vertex v;
            v.pos = glm::vec3(x - width / 2.0f, 0.0f, z - depth / 2.0f);
            v.color = glm::vec3(1.0f); // white
            outVertices.push_back(v);
        }
    }

    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            int start = z * (width + 1) + x;
            outIndices.push_back(start);
            outIndices.push_back(start + 1);
            outIndices.push_back(start + width + 1);

            outIndices.push_back(start + 1);
            outIndices.push_back(start + width + 2);
            outIndices.push_back(start + width + 1);
        }
    }
}

```

```c++
void loadModel() {
    createGrid(20, 20, vertices, indices); 
}
```

```c++
ubo.view = glm::lookAt(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
```
**Output:**
![](images/ex1.png)

**Reflection:**
When I followed the implementation steps, then angle I was looking at the grid was too steep, and didn't match the one showed in the example image.
I fixed this by changing lookat function until the output matched the example image. I also had to modify all the buffer sizes to use uint32_t instead of uint16_t, 
as that's what the example code used for the parameters of the grid

### EXERCISE 2: CREATE A WAVY TERRAI
#### Goal:  Modify the grid generation logic to create a simple, wavy terrain.


**Solution:**

```c++
```
```c++

```

**Reflection:**


### EXERCISE 3: PROCEDURAL CYLINDER
#### Goal: Procedurally generate and render a cylinder mesh.

**Solution:**

```c++
```
```c++

```

![](images/ex3.png)

**Reflection:**


### EXERCISE 4:  WIREFRAME RENDERING
#### Goal: Refactor the procedural generation code into a reusable C++ class or namespace, similar to the GeometryGenerator provided at d3d12book/Chapter 7 Drawing in Direct3D 
#### Part II at master · d3dcoder/d3d12book using the procedural geometric models defined in GeometryGenerator.h, GeometryGenerator.cp

**Solution:**
```c++

```

**Reflection:**


### EXERCISE 5: LOADING EXTERNAL MODELS WITH ASSIMP
#### Goal: Integrate the Assimp library into your project to load and render a 3D model from an .obj file.

**Solution:**

```c++
```
```c++
```


**Reflection:**


