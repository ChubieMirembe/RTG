# Vulkan Lab 8: Post-Processing Effects

### EXERCISE 1: RENDER-TO-TEXTURE ON A CUBE

**Solution:** 
To complete this exerrcise the main resource I used was the lecture slides and the lab sheet. Combining the two resources I was 
able to implement the render-to-texture technique in Vulkan. The main steps involved were: creating offscreen resources, creating
a post-processing descriptor set layout, and recording the command buffer with two passes. I then created shaders for both passes
to render the scene to a texture and then map that texture onto a cube. 


- Create offscreen resources

```cpp
void createOffscreenResources() {
    VkFormat fmt = swapChainImageFormat;

    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        fmt,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        offscreenImage,
        offscreenImageMemory
    );

    offscreenImageView = createImageView(
        offscreenImage,
        fmt,
        VK_IMAGE_ASPECT_COLOR_BIT
    );

    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    vkCreateSampler(device, &info, nullptr, &offscreenSampler);
}
```

- Create post-processing descriptor set layout

```c++
void createPostDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo{};
    ubo.binding = 0;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.descriptorCount = 1;
    ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding tex{};
    tex.binding = 1;
    tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tex.descriptorCount = 1;
    tex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings{ ubo, tex };

    VkDescriptorSetLayoutCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ci.bindingCount = 2;
    ci.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(device, &ci, nullptr, &postDescriptorSetLayout);
}
```

- Record command buffer with two passes

```c++
// Pass 1: Offscreen → COLOR_ATTACHMENT
VkImageMemoryBarrier2 offToColor{};
offToColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
offToColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
offToColor.srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
offToColor.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
offToColor.srcAccessMask = 0;
offToColor.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
offToColor.image = offscreenImage;
offToColor.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
vkCmdPipelineBarrier2(cb, &depOffToColor);

// --- PASS 1 BEGIN ---
vkCmdBeginRendering(cb, &sceneRenderingInfo);
vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
vkCmdEndRendering(cb);

// Pass 1 → Pass 2: COLOR_ATTACHMENT → SHADER_READ_ONLY
VkImageMemoryBarrier2 offToSample{};
offToSample.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
offToSample.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
offToSample.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
offToSample.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
offToSample.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
offToSample.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
offToSample.image = offscreenImage;
offToSample.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
vkCmdPipelineBarrier2(cb, &depOffToSample);

// --- PASS 2 BEGIN ---
vkCmdBeginRendering(cb, &finalRenderingInfo);
vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, postPipeline);
vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        postPipelineLayout, 0, 1, &postDescriptorSets[i], 0, nullptr);
vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
vkCmdEndRendering(cb);
```

**Output:**

- Pass 1: Render scene to a texture

![](images/ex1_1.png)

- Pass 2: Map the texture onto a cube

![](images/ex1_2.png)

**Reflection:**
This exerrcise continued my understanding of Vulkan's rendering pipeline, particularly in the context of offscreen rendering
and texture mapping. The challenge was in managing the image layouts and ensuring that the transitions between different stages were handled correctly.

---

### EXERCISE 2: IMPLEMENTING THE SKYBOX

**Solution:** 

```cpp
```
```cpp
```
```c++
```
```c++
```
**Output:**

![](images/ex2.png)

**Reflection:**

---

### EXERCISE 3: SIMPLE GLOW EFFECT

**Solution:** 


```cpp
```
```cpp
```
```c++
```
```c++
```
**Output:**

![](images/ex3.png)

**Reflection:**

---

### EXERCISE 4:  OBJECT ON FIRE ANIMATION

**Solution:** 

```cpp
```
```cpp
```
```c++
```
```c++
```
**Output:**

![](images/ex4.png)

**Reflection:**


---


### FURTHER EXPLORATION

**Solution:** 

```cpp
```
```cpp
```
```c++
```
```c++
```
**Output:**

![](images/ex5.png)

**Reflection:**

