# Vulkan Lab 4: Lighting Fundamentals 


### EXERCISE 1: PREPARING THE APPLICATION FOR TEXTURES

**Solution:**
The external image-loading header file stb_image.h was downloaded from the official GitHub repository and 
placed into a Dependencies folder within the project for better organization. The macro 
STB_IMAGE_IMPLEMENTATION was defined before including <stb_image.h> to enable loading of PNG and JPG files
during runtime. A wall texture image was placed into an Assets directory next to the application executable 
so that the file can be accessed with a relative path when rendering. The Vertex structure was extended to 
include a glm::vec2 texCoord attribute, providing texture coordinates for each vertex. The Vulkan vertex 
input description was also updated to include an attribute for this new UV data using the correct memory 
formatting and offset. Finally, the cube vertex data was modified so that all 36 vertices now include 
normalized UV coordinates, ensuring proper texture mapping across all six faces of the cube. These
completed changes prepare the application to apply textures correctly in later exercises.

- Include STB Image Implementation:
```c++
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```
- Vertex Structure Update:
```c++
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
	glm::vec3 normal;        // <- - Normal Vector
	glm::vec2 texCoord;     // <- - Texture Coordinate

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) };
        attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) };
		attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, texCoord) };    // <- - Texture Coordinate
        return attributeDescriptions;
    }
};
```

**Reflection:**
This exercise introduced the foundational requirements for supporting textures within a Vulkan application. 
I learned that the graphics pipeline must be informed not only of vertex position and colour data, but also 
of UV coordinates that define how a 2D image is mapped onto 3D geometry. Updating the Vertex structure and
vertex attribute descriptions demonstrated the importance of maintaining consistent memory layouts between 
CPU-side data and shader inputs. Adding stb_image.h reinforced the need for image-loading utilities because
Vulkan itself does not provide built-in support for reading texture files. Generating full UV coordinates 
for each cube vertex highlighted how texture mapping relies on precise indexing to avoid distortion. Overall,
this exercise improved my understanding of how both data structure design and pipeline configuration 
contribute to correct texture usage in Vulkan.

---

### EXERCISE 2: LOADING AND CREATING VULKAN IMAGE RESOURCES

**Solution:**



```c++
```

```c++
```

```c++

```

**Output:**


**Reflection:**


---
### EXERCISE 3: PER-FRAGMENTDIFFUSE LIGHTING
#### Goal: Improve visual quality by moving the lighting logic to the fragment shader, allowing for smoother, more accurate calculations.

**Solution:**


**Output:**


**Reflection:**

--- 
### EXERCISE 4:ADDING PER-VERTEX SPECULAR LIGHTING
#### Goal: : Complete the reflection model implemented in Exercise 2 by adding specular highlights to the vertex shader.


**Solution:**



**Output:**


**Reflection:**

### EXERCISE 5:ADDING PER-FRAGMENT SPECULAR LIGHTING
#### Goal: Complete the reflection model implemented in Exercise 3 by adding specular highlights to the fragment shader.

**Solution:**

**Output:**



**Reflection:**
