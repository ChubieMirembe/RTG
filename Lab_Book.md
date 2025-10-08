# Vulkan Lab 1 - Simple Shapes
## Week 2 - Lab A


### EXERCISE 1: DRAW TWO TRIANGLES WITHOUT USING VERTEX INDICES
#### Goal: Render two distinct triangles instead of one quad using vkCmdDraw().

```c++
void loadModel() {
	// Unique vertex data for two triangles.
    vertices = {
        // First triangle (shifted left by -0.6 on X)
        {{-0.5f - 0.6f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f - 0.6f, -0.5f, 0.0f},  {0.0f, 1.0f, 0.0f}},
        {{0.0f - 0.6f,  0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}},

        // Second triangle (shifted right by +0.6 on X)
        {{0.5f + 0.6f, -0.5f, 0.0f},  {1.0f, 1.0f, 0.0f}},
        {{0.9f + 0.6f,  0.5f, 0.0f},  {0.0f, 1.0f, 1.0f}},
        {{0.0f + 0.6f,  0.5f, 0.0f},  {1.0f, 0.0f, 1.0f}},
    };


    // indices cleared as the to triangle will have s
    indices.clear();
}
```

```c++
void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();

    loadModel();

    createVertexBuffer();
	//createIndexBuffer();  I don't need index buffer because the triangles are not sharing vertices.
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}
```

```c++
void HelloTriangleApplication::cleanup() {
    cleanupSwapChain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

	/*
    This whole section is commented out because it deals with handling the memory for vertex and index buffers
    which I am no longer using.

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    */

```

```c++
 VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    /*
	* This is the function call to bind the index buffer, which I am no longer using.
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    */
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    /*
	* I'm no longer using indexed drawing.
    vkCmdDrawIndexed(commandBuffer, static_cast<uint16_t>(indices.size()), 1, 0, 0, 0);
    */
    vkCmdDraw(commandBuffer, 6, 1, 0, 0); // 6 for the unique vertex count
    vkCmdEndRendering(commandBuffer);
```

```c++

```

```c++

```
**Reflection:**
After designing two distinct triangles with custom vertex data, I ensured they were clearly visible and did not overlap on the screen.
This initial step helped me understand how to define and manage vertex positions manually.

Next, I systematically removed all references to the index buffer throughout the codebase. 
This included eliminating the createIndexBuffer() function call from initVulkan() and cleaning up any related logic 
that previously relied on indexed drawing.

With the index buffer removed, I updated the drawFrame() function to use vkCmdDraw() instead of vkCmdDrawIndexed(). 
Since indexed drawing was no longer applicable, I specified the vertex count directly in the draw call, aligning it with the number of vertices I had defined.
This exercise was a valuable learning experience. It not only taught me how to create and manage custom vertex data but 
also gave me deeper insight into the structure of Vulkan applications. By removing index buffer usage, I was able to 
explore various parts of the Vulkan pipeline and understand how different components interact when rendering geometry.


