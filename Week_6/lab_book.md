# Vulkan Lab 5: Texture Mapping


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
        attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) };
		attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord) };    // <- - Texture Coordinate
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
In this exercise, I implemented the complete Vulkan texture-loading workflow  from reading an image file on the CPU to creating a fully functional 
GPU texture object ready for sampling in the shader. The process involved understanding how data flows between host and device memory, how Vulkan
handles image layouts and synchronization, and how samplers define how textures are accessed.

I began by declaring four main Vulkan objects (VkImage, VkDeviceMemory, VkImageView, and VkSampler) to represent the texture resource, its memory,
view, and sampling configuration. I used stb_image to load an image file (wall.jpg) and obtain pixel data in RGBA format, then created a staging 
buffer that temporarily stored this data in host-visible memory. The pixel data was mapped into this buffer using vkMapMemory() and later un-mapped
before freeing the CPU copy with stbi_image_free().

Next, I created a GPU-resident VkImage using device-local memory. This step ensured that the texture resides in high-performance memory optimized 
for sampling. Before the texture could be used, it was transitioned through layout states: first from UNDEFINED to TRANSFER_DST_OPTIMAL for data 
transfer, and then from TRANSFER_DST_OPTIMAL to SHADER_READ_ONLY_OPTIMAL for shader access. These transitions were handled using pipeline barriers 
recorded and executed within one-time command buffers. The data transfer itself was performed using vkCmdCopyBufferToImage().

After transferring, I created an image view (VkImageView) to define how the image would be accessed in the shader, and a sampler (VkSampler) that 
controls filtering and addressing behavior. The sampler used linear filtering, repeat wrapping, and anisotropic filtering based on device limits.
Finally, I cleaned up the staging buffer since the data had already been copied to GPU memory.

- Creating the Image:
```c++
void HelloTriangleApplication::createImage(uint32_t width, uint32_t height, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
    VkImage& image, VkDeviceMemory& imageMemory) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(device, &imageInfo, nullptr, &image);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    vkBindImageMemory(device, image, imageMemory, 0);
}
```
- Transitioning Image Layouts:
```c++
void HelloTriangleApplication::transitionImageLayout(
    VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask) {

    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(cmd);
}
```
- Copying Buffer to Image:
```c++
void HelloTriangleApplication::copyBufferToImage(
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(cmd, buffer, image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(cmd);
}
```
- Creating Image View:
```c++
VkImageView HelloTriangleApplication::createImageView(
    VkImage image, VkFormat format, VkImageAspectFlags aspectMask) {

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    vkCreateImageView(device, &viewInfo, nullptr, &imageView);
    return imageView;
}
```
- Creating Texture Sampler:
```c++
void HelloTriangleApplication::createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler);
}
```

**Reflection:**
This exercise deepened my understanding of how Vulkan manages textures at a low level. I learned that loading an image involves several explicit 
steps such as creating a staging buffer, transferring data to a GPU-local image, performing layout transitions, and setting up an image view and 
sampler.

The process highlighted Vulkan’s emphasis on explicit control. Each stage, from CPU data upload to GPU sampling, had to be manually defined, 
including synchronization through layout transitions (UNDEFINED ? TRANSFER_DST ? SHADER_READ_ONLY). Implementing these barriers taught me how 
crucial proper pipeline ordering is for correctness and performance.

Creating the image view and sampler clarified how Vulkan separates texture storage from how it is accessed. The view defines which parts of the 
image are visible, while the sampler defines how textures are filtered and wrapped.

Overall, I learned how textures flow from disk to GPU memory and how Vulkan requires precise management of memory and synchronization. This 
exercise built a solid foundation for understanding image-based resources and will be essential for later topics such as mipmapping, depth buffers,
and material systems.

---
### EXERCISE 4: BINDING AND SHADER UPDATES

**Solution:**
With the texture image, view, and sampler successfully created, I updated the descriptor set layout to include a second binding for the combined 
image sampler. This ensured that the fragment shader could access the texture along with the uniform buffer data. The descriptor pool was expanded 
to support both uniform buffer and sampler descriptors, and each descriptor set was written with two bindings, one for the uniform buffer (binding 0)
and one for the texture sampler (binding 1). The shaders were then updated to include texture coordinates and a `sampler2D` uniform bound to binding 1. 
The fragment shader sampled the texture using the interpolated UV coordinates and output the resulting texture color directly to the framebuffer. 
The vertex structure and pipeline configuration were modified to pass texture coordinates from the vertex shader to the fragment shader. During 
initialization, the texture setup functions `createTextureImage()`, `createTextureImageView()`, and `createTextureSampler()` were called before 
creating vertex and index buffers so that the texture resources were ready when command buffers were recorded. In the cleanup routine, I ensured that 
all texture-related Vulkan objects, including the sampler, image view, image, and its device memory, were properly destroyed to prevent resource leaks. 
This completed the full Vulkan texture-loading workflow, allowing the cube to render with the applied texture. Through this exercise, I gained a clear 
understanding of how Vulkan handles image creation, memory transitions, descriptor bindings, and sampler configuration to enable textured rendering.

```c++
// Add sampler binding alongside the existing UBO
VkDescriptorSetLayoutBinding ubo{};
ubo.binding = 0;
ubo.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
ubo.descriptorCount = 1;
ubo.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

VkDescriptorSetLayoutBinding sampler{};
sampler.binding = 1;                                       // New binding
sampler.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
sampler.descriptorCount = 1;
sampler.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

std::array<VkDescriptorSetLayoutBinding, 2> bindings{ ubo, sampler };

VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
layoutInfo.pBindings    = bindings.data();

vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
```
```c++
std::array<VkDescriptorPoolSize, 2> poolSizes{};
poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         MAX_FRAMES_IN_FLIGHT };
poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT }; // new

VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
poolInfo.pPoolSizes    = poolSizes.data();
poolInfo.maxSets       = MAX_FRAMES_IN_FLIGHT;

vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
```

```c++
VkDescriptorImageInfo imageInfo{};
imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
imageInfo.imageView   = textureImageView;
imageInfo.sampler     = textureSampler;

VkWriteDescriptorSet samplerWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
samplerWrite.dstSet          = descriptorSets[i];
samplerWrite.dstBinding      = 1; // matches layout(binding=1)
samplerWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
samplerWrite.descriptorCount = 1;
samplerWrite.pImageInfo      = &imageInfo;

vkUpdateDescriptorSets(device, 1, &samplerWrite, 0, nullptr);
```

**Output:**
![](Week_6/Images/ex4.png)

**Reflection:**
This exercise helped me understand how to actually get textures working in Vulkan instead of just loading them. I learned that creating the image 
isn’t enough; you have to connect it to the shaders through descriptor layouts, descriptor sets, and samplers. Setting up the combined image sampler 
in the descriptor layout made it clear how Vulkan links texture data to the fragment shader. I also realized how hands-on Vulkan is, since you have
to manage every part of the process yourself, from adding the sampler binding to updating descriptor sets and binding them during draw calls.
Writing the vertex and fragment shaders tied it all together, because I could see how the texture coordinates flowed through the pipeline to 
sample the image. Overall, it was satisfying to finally see how everything fits together and how the CPU-side texture setup, GPU memory, and shaders 
all work in sync to display a textured cube.

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
