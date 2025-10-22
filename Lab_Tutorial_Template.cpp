//==================================================
// Vulkan ver 1.3 — Exercise 4: Sun–Earth–Moon (Depth On)
//==================================================

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>

/////////////////////////////////////////////////////
// Config
/////////////////////////////////////////////////////

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

/////////////////////////////////////////////////////
// Data
/////////////////////////////////////////////////////


// Match GLSL std140 layout exactly
// Replace your old UniformBufferObject with this version:
struct UniformBufferObject {
    alignas(16) glm::mat4 model;   // per-object model matrix
    alignas(16) glm::vec4 eye;     // xyz = camera position
    alignas(16) glm::vec4 center;  // xyz = look-at target
    alignas(16) glm::vec4 up;      // xyz = world up (e.g., 0,1,0)
    alignas(16) glm::vec4 cam;     // x=fovy(rad), y=aspect, z=zNear, w=zFar
};



struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription desc{};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> a{};
        a[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) };
        a[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) };
        return a;
    }
};


// Cube geometry (triangle strip with degenerates)
const std::vector<Vertex> Quad_vertices = {
    {{-1.0f,-1.0f,+1.0f},{1,0,0}}, {{+1.0f,-1.0f,+1.0f},{0,1,0}},
    {{+1.0f,+1.0f,+1.0f},{0,0,1}}, {{-1.0f,+1.0f,+1.0f},{1,1,1}},
    {{-1.0f,-1.0f,-1.0f},{1,0,1}}, {{+1.0f,-1.0f,-1.0f},{1,1,0}},
    {{+1.0f,+1.0f,-1.0f},{0,1,1}}, {{-1.0f,+1.0f,-1.0f},{1,0.5f,0}}
};

const std::vector<uint16_t> Quad_indices = {
    0,1,3,2,  2,1,  1,5,2,6,  6,5,  5,4,6,7,  7,4,  4,0,7,3,  3,2,  3,2,7,6,  6,5,  5,1,4,0
};

std::vector<Vertex>   vertices;
std::vector<uint16_t> indices;

void generateSphere(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices,
    float radius = 1.0f, uint32_t stacks = 32, uint32_t slices = 32) {
    vertices.clear();
    indices.clear();

    for (uint32_t i = 0; i <= stacks; ++i) {
        float v = float(i) / stacks;
        float phi = v * glm::pi<float>();
        for (uint32_t j = 0; j <= slices; ++j) {
            float u = float(j) / slices;
            float theta = u * glm::two_pi<float>();
            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);
            vertices.push_back({ glm::vec3(radius * x, radius * y, radius * z),
                                 glm::vec3((x + 1) * 0.5f, (y + 1) * 0.5f, (z + 1) * 0.5f) });
        }
    }

    for (uint32_t i = 0; i < stacks; ++i) {
        for (uint32_t j = 0; j < slices; ++j) {
            uint32_t first = i * (slices + 1) + j;
            uint32_t second = first + slices + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}


static void loadModel() {
    generateSphere(vertices, indices, 1.0f, 32, 32);
}


/////////////////////////////////////////////////////
// Debug utils
/////////////////////////////////////////////////////

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) func(instance, debugMessenger, pAllocator);
}

/////////////////////////////////////////////////////
// Application
/////////////////////////////////////////////////////

class HelloTriangleApplication {
public:
    void run();

private:
    // Window / app
    GLFWwindow* window = {};
    bool framebufferResized = false;
    uint32_t currentFrame = 0;

    // Vulkan core
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent{ 0,0 };
    std::vector<VkImageView> swapChainImageViews;

    // Pipeline
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // Buffers
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    // UBOs — [EX4 CHANGE] 3 per frame (Sun, Earth, Moon)
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    // Depth resources
    VkImage        depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView    depthImageView = VK_NULL_HANDLE;
    VkFormat       depthFormat = VK_FORMAT_UNDEFINED;

    // Descriptors
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    // Command buffers / sync
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    // Flow
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Init steps
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();     // [EX4 CHANGE] 3 per frame
    void createDescriptorPool();     // [EX4 CHANGE] capacity for 3 per frame
    void createDescriptorSets();     // [EX4 CHANGE] write 3 per frame
    void createDepthResources();     // depth
    void createCommandBuffers();
    void createSyncObjects();

    // Draw
    void drawFrame();
    void recreateSwapChain();
    void cleanupSwapChain();
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void updateUniformBuffer(uint32_t currentImage); // [EX4 CHANGE] Sun→Earth→Moon

    // Helpers
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // Buffer helpers
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Images (depth)
    void createImage(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage,
        VkImage& image, VkDeviceMemory& mem);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);

    // Callbacks
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* userData);
};

/////////////////////////////////////////////////////
// Run
/////////////////////////////////////////////////////

void HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Lab 3 - Exercise 4", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

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
    createIndexBuffer();
    createUniformBuffers();    // [EX4 CHANGE] 3 per frame
    createDescriptorPool();    // [EX4 CHANGE] for 3 per frame
    createDescriptorSets();    // [EX4 CHANGE] write 3 per frame
    createDepthResources();    // depth created after swap extent is known
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}

void HelloTriangleApplication::cleanup() {
    cleanupSwapChain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    for (size_t i = 0; i < uniformBuffers.size(); ++i) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

/////////////////////////////////////////////////////
// Instance / Device / Swapchain
/////////////////////////////////////////////////////

void HelloTriangleApplication::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available!");

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Hello Triangle";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "No Engine";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;

    auto extensions = getRequiredExtensions();
    ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCI{};
    if (enableValidationLayers) {
        ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        ci.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCI);
        ci.pNext = &debugCI;
    }
    else {
        ci.enabledLayerCount = 0;
        ci.pNext = nullptr;
    }

    if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

void HelloTriangleApplication::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT ci{};
    populateDebugMessengerCreateInfo(ci);
    if (CreateDebugUtilsMessengerEXT(instance, &ci, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger!");
}

void HelloTriangleApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void HelloTriangleApplication::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (!count) throw std::runtime_error("No Vulkan-capable GPU found!");
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    for (auto d : devices) {
        if (isDeviceSuitable(d)) { physicalDevice = d; break; }
    }
    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to find a suitable GPU!");
}

void HelloTriangleApplication::createLogicalDevice() {
    QueueFamilyIndices idx = findQueueFamilies(physicalDevice);
    std::set<uint32_t> unique = { idx.graphicsFamily.value(), idx.presentFamily.value() };

    std::vector<VkDeviceQueueCreateInfo> queues;
    float priority = 1.0f;
    for (uint32_t qf : unique) {
        VkDeviceQueueCreateInfo q{};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = qf;
        q.queueCount = 1;
        q.pQueuePriorities = &priority;
        queues.push_back(q);
    }

    VkPhysicalDeviceDynamicRenderingFeatures dyn{};
    dyn.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dyn.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features sync2{};
    sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2.synchronization2 = VK_TRUE;
    dyn.pNext = &sync2;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &dyn;

    VkDeviceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pNext = &features2;
    ci.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    ci.pQueueCreateInfos = queues.data();
    ci.pEnabledFeatures = nullptr;
    ci.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    ci.ppEnabledExtensionNames = deviceExtensions.data();
    if (enableValidationLayers) {
        ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        ci.ppEnabledLayerNames = validationLayers.data();
    }

    if (vkCreateDevice(physicalDevice, &ci, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    vkGetDeviceQueue(device, idx.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, idx.presentFamily.value(), 0, &presentQueue);
}

void HelloTriangleApplication::createSwapChain() {
    auto details = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(details.formats);
    VkPresentModeKHR present = chooseSwapPresentMode(details.presentModes);
    VkExtent2D extent = chooseSwapExtent(details.capabilities);

    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
        imageCount = details.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = format.format;
    ci.imageColorSpace = format.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices idx = findQueueFamilies(physicalDevice);
    uint32_t indicesArray[] = { idx.graphicsFamily.value(), idx.presentFamily.value() };
    if (idx.graphicsFamily != idx.presentFamily) {
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = indicesArray;
    }
    else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    ci.preTransform = details.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = present;
    ci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = format.format;
    swapChainExtent = extent;
}

void HelloTriangleApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        vi.image = swapChainImages[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = swapChainImageFormat;
        vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.baseMipLevel = 0;
        vi.subresourceRange.levelCount = 1;
        vi.subresourceRange.baseArrayLayer = 0;
        vi.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &vi, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view!");
    }
}

/////////////////////////////////////////////////////
// Pipeline
/////////////////////////////////////////////////////

void HelloTriangleApplication::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo{};
    ubo.binding = 0;
    ubo.descriptorCount = 1;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // simple per-object UBOs
    ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.bindingCount = 1;
    info.pBindings = &ubo;

    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
}

void HelloTriangleApplication::createGraphicsPipeline() {
    auto vert = readFile("shaders/vert.spv");
    auto frag = readFile("shaders/frag.spv");
    VkShaderModule vertModule = createShaderModule(vert);
    VkShaderModule fragModule = createShaderModule(frag);

    VkPipelineShaderStageCreateInfo vs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    vs.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vs.module = vertModule;
    vs.pName = "main";

    VkPipelineShaderStageCreateInfo fs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    fs.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fs.module = fragModule;
    fs.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vs, fs };

    auto binding = Vertex::getBindingDescription();
    auto attrs = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &binding;
    vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vi.pVertexAttributeDescriptions = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    ia.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.lineWidth = 1.0f;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth state
    VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState cbAttach{};
    cbAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cbAttach.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cb.attachmentCount = 1;
    cb.pAttachments = &cbAttach;

    std::vector<VkDynamicState> dynStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dyn.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dyn.pDynamicStates = dynStates.data();

    VkPipelineLayoutCreateInfo pl{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &descriptorSetLayout;
    if (vkCreatePipelineLayout(device, &pl, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    // Dynamic rendering: provide formats (color + depth)
    depthFormat = findDepthFormat();

    VkPipelineRenderingCreateInfo rendering{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &swapChainImageFormat;
    rendering.depthAttachmentFormat = depthFormat;

    VkGraphicsPipelineCreateInfo gp{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    gp.pNext = &rendering;
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;          // enable depth
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &dyn;
    gp.layout = pipelineLayout;
    gp.renderPass = VK_NULL_HANDLE;
    gp.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, nullptr, &graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");

    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
}

void HelloTriangleApplication::createCommandPool() {
    QueueFamilyIndices q = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = q.graphicsFamily.value();
    if (vkCreateCommandPool(device, &info, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");
}

/////////////////////////////////////////////////////
// Buffers
/////////////////////////////////////////////////////

void HelloTriangleApplication::createVertexBuffer() {
    VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

    VkBuffer staging;
    VkDeviceMemory stagingMem;
    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    void* data;
    vkMapMemory(device, stagingMem, 0, size, 0, &data);
    std::memcpy(data, vertices.data(), (size_t)size);
    vkUnmapMemory(device, stagingMem);

    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    copyBuffer(staging, vertexBuffer, size);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
}

void HelloTriangleApplication::createIndexBuffer() {
    VkDeviceSize size = sizeof(indices[0]) * indices.size();

    VkBuffer staging;
    VkDeviceMemory stagingMem;
    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    void* data;
    vkMapMemory(device, stagingMem, 0, size, 0, &data);
    std::memcpy(data, indices.data(), (size_t)size);
    vkUnmapMemory(device, stagingMem);

    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    copyBuffer(staging, indexBuffer, size);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
}

void HelloTriangleApplication::createUniformBuffers() {
    const size_t total = MAX_FRAMES_IN_FLIGHT * 3; // [EX4 CHANGE] Sun + Earth + Moon per frame
    VkDeviceSize sz = sizeof(UniformBufferObject);

    uniformBuffers.resize(total);
    uniformBuffersMemory.resize(total);
    uniformBuffersMapped.resize(total);

    for (size_t i = 0; i < total; ++i) {
        createBuffer(sz, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i], uniformBuffersMemory[i]);
        vkMapMemory(device, uniformBuffersMemory[i], 0, sz, 0, &uniformBuffersMapped[i]);
    }
}

void HelloTriangleApplication::createDescriptorPool() {
    const uint32_t totalSets = MAX_FRAMES_IN_FLIGHT * 3; // [EX4 CHANGE]

    VkDescriptorPoolSize pool{};
    pool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool.descriptorCount = totalSets;

    VkDescriptorPoolCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    ci.poolSizeCount = 1;
    ci.pPoolSizes = &pool;
    ci.maxSets = totalSets;

    if (vkCreateDescriptorPool(device, &ci, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void HelloTriangleApplication::createDescriptorSets() {
    const uint32_t totalSets = MAX_FRAMES_IN_FLIGHT * 3; // [EX4 CHANGE]

    std::vector<VkDescriptorSetLayout> layouts(totalSets, descriptorSetLayout);
    VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    ai.descriptorPool = descriptorPool;
    ai.descriptorSetCount = totalSets;
    ai.pSetLayouts = layouts.data();

    descriptorSets.resize(totalSets);
    if (vkAllocateDescriptorSets(device, &ai, descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    for (size_t i = 0; i < totalSets; ++i) {
        VkDescriptorBufferInfo bi{};
        bi.buffer = uniformBuffers[i];
        bi.offset = 0;
        bi.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = descriptorSets[i];
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &bi;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

/////////////////////////////////////////////////////
// Depth
/////////////////////////////////////////////////////

VkFormat HelloTriangleApplication::findDepthFormat() {
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    for (VkFormat f : candidates) {
        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, f, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return f;
    }
    throw std::runtime_error("No suitable depth format found");
}

bool HelloTriangleApplication::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void HelloTriangleApplication::createDepthResources() {
    depthFormat = findDepthFormat();
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

/////////////////////////////////////////////////////
// Command buffers / sync
/////////////////////////////////////////////////////

void HelloTriangleApplication::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.commandPool = commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &ai, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void HelloTriangleApplication::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo si{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fi{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device, &si, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &si, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fi, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sync objects!");
        }
    }
}

/////////////////////////////////////////////////////
// Draw / swapchain lifecycle
/////////////////////////////////////////////////////

void HelloTriangleApplication::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult res = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
        &imageIndex);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapChain(); return; }
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain image!");

    updateUniformBuffer(currentFrame);

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    // Submit (Synchronization2)
    VkCommandBufferSubmitInfo cmdInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
    cmdInfo.commandBuffer = commandBuffers[currentFrame];

    VkSemaphoreSubmitInfo waitInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
    waitInfo.semaphore = imageAvailableSemaphores[currentFrame];
    waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signalInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
    signalInfo.semaphore = renderFinishedSemaphores[currentFrame];
    signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &waitInfo;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &cmdInfo;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signalInfo;

    if (vkQueueSubmit2(graphicsQueue, 1, &submit, inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");

    VkPresentInfoKHR present{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
    present.swapchainCount = 1;
    present.pSwapchains = &swapChain;
    present.pImageIndices = &imageIndex;

    res = vkQueuePresentKHR(presentQueue, &present);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::recreateSwapChain() {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(window, &w, &h);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createDepthResources();     // recreate depth for new extent
}

void HelloTriangleApplication::cleanupSwapChain() {
    for (auto v : swapChainImageViews)
        vkDestroyImageView(device, v, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    if (depthImageView) { vkDestroyImageView(device, depthImageView, nullptr); depthImageView = VK_NULL_HANDLE; }
    if (depthImage) { vkDestroyImage(device, depthImage, nullptr); depthImage = VK_NULL_HANDLE; }
    if (depthImageMemory) { vkFreeMemory(device, depthImageMemory, nullptr); depthImageMemory = VK_NULL_HANDLE; }
}

void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cmd, &bi) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin cmd buffer!");

    // Color barrier: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier2 colorBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    colorBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    colorBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    colorBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorBarrier.image = swapChainImages[imageIndex];
    colorBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    // Depth barrier: UNDEFINED -> DEPTH_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier2 depthBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    depthBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    depthBarrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    depthBarrier.srcAccessMask = 0;
    depthBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthBarrier.image = depthImage;
    depthBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

    std::array<VkImageMemoryBarrier2, 2> barriers{ colorBarrier, depthBarrier };
    VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dep.imageMemoryBarrierCount = (uint32_t)barriers.size();
    dep.pImageMemoryBarriers = barriers.data();
    vkCmdPipelineBarrier2(cmd, &dep);

    // Dynamic rendering attachments
    VkRenderingAttachmentInfo color{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    color.imageView = swapChainImageViews[imageIndex];
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.clearValue.color = { {0.05f, 0.07f, 0.12f, 1.0f} }; // dark bluish

    VkRenderingAttachmentInfo depth{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    depth.imageView = depthImageView;
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo ri{ VK_STRUCTURE_TYPE_RENDERING_INFO };
    ri.renderArea = { {0,0}, swapChainExtent };
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &color;
    ri.pDepthAttachment = &depth;

    vkCmdBeginRendering(cmd, &ri);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport vp{};
    vp.width = static_cast<float>(swapChainExtent.width);
    vp.height = static_cast<float>(swapChainExtent.height);
    vp.minDepth = 0.0f; vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D sc{ {0,0}, swapChainExtent };
    vkCmdSetScissor(cmd, 0, 1, &sc);

    VkBuffer vbs[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vbs, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    const uint32_t base = currentFrame * 3; // [EX4 CHANGE]

    // 0) Sun
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
        0, 1, &descriptorSets[base + 0], 0, nullptr);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    // 1) Earth
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
        0, 1, &descriptorSets[base + 1], 0, nullptr);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    // 2) Moon
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
        0, 1, &descriptorSets[base + 2], 0, nullptr);
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    vkCmdEndRendering(cmd);

    // Transition color for present
    VkImageMemoryBarrier2 toPresent{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.image = swapChainImages[imageIndex];
    toPresent.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkDependencyInfo dep2{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dep2.imageMemoryBarrierCount = 1;
    dep2.pImageMemoryBarriers = &toPresent;
    vkCmdPipelineBarrier2(cmd, &dep2);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

/////////////////////////////////////////////////////
// UBO update — Exercise 4 transforms (Sun→Earth→Moon)
/////////////////////////////////////////////////////

void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    // --- Time
    static auto start = std::chrono::high_resolution_clock::now();
    float t = std::chrono::duration<float>(
        std::chrono::high_resolution_clock::now() - start).count();

    // --- Camera params (now sent directly, not as matrices)
    glm::vec3 eye = { 12.0f, 7.0f, 12.0f };
    glm::vec3 center = { 0.0f, 0.0f,  0.0f };
    glm::vec3 up = { 0.0f, 1.0f,  0.0f };

    float fovy = glm::radians(45.0f);
    float aspect = swapChainExtent.width / (float)swapChainExtent.height;
    float zNear = 0.1f;
    float zFar = 200.0f;

    // --- Speeds
    float sunSpinDeg = 10.0f;
    float earthOrbitDeg = 40.0f;
    float earthSpinDeg = 120.0f;
    float moonOrbitDeg = 200.0f;

    // --- Distances / scales
    float earthRadius = 4.0f;
    float moonRadius = 1.6f;
    float sunScale = 1.6f;
    float earthScale = 0.6f;
    float moonScale = 0.18f;

    // --- Tilts
    float earthTiltDeg = 15.0f;
    float moonTiltDeg = 5.0f;

    // --- Angles
    float sunSpin = glm::radians(sunSpinDeg) * t;
    float earthOrbit = glm::radians(earthOrbitDeg) * t;
    float earthSpin = glm::radians(earthSpinDeg) * t;
    float moonOrbit = glm::radians(moonOrbitDeg) * t;

    // --- Sun (root)
    glm::mat4 Sun =
        glm::rotate(glm::mat4(1.0f), sunSpin, glm::vec3(0, 1, 0)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(sunScale));

    // --- Earth (relative to Sun)
    glm::mat4 Earth =
        Sun *
        glm::rotate(glm::mat4(1.0f), glm::radians(earthTiltDeg), glm::vec3(1, 0, 0)) *
        glm::rotate(glm::mat4(1.0f), earthOrbit, glm::vec3(0, 1, 0)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(earthRadius, 0, 0)) *
        glm::rotate(glm::mat4(1.0f), earthSpin, glm::vec3(0, 1, 0)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(earthScale));

    // --- Moon (relative to Earth) with tidal lock (inverse rotation)
    glm::mat4 Moon =
        Earth *
        glm::rotate(glm::mat4(1.0f), glm::radians(moonTiltDeg), glm::vec3(0, 0, 1)) *
        glm::rotate(glm::mat4(1.0f), moonOrbit, glm::vec3(0, 1, 0)) *
        glm::translate(glm::mat4(1.0f), glm::vec3(moonRadius, 0, 0)) *
        glm::rotate(glm::mat4(1.0f), -moonOrbit, glm::vec3(0, 1, 0)) * // tidal lock
        glm::scale(glm::mat4(1.0f), glm::vec3(moonScale));

    // --- Write 3 UBOs (Sun, Earth, Moon) for this frame
    const uint32_t base = currentImage * 3;

    auto writeUBO = [&](uint32_t idx, const glm::mat4& model) {
        UniformBufferObject u{};
        u.model = model;
        u.eye = glm::vec4(eye, 1.0f);
        u.center = glm::vec4(center, 1.0f);
        u.up = glm::vec4(up, 0.0f);                  // w unused
        u.cam = glm::vec4(fovy, aspect, zNear, zFar);
        std::memcpy(uniformBuffersMapped[base + idx], &u, sizeof(u));
        };

    writeUBO(0, Sun);
    writeUBO(1, Earth);
    writeUBO(2, Moon);
}



/////////////////////////////////////////////////////
// Helpers
/////////////////////////////////////////////////////

void HelloTriangleApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& ci) {
    ci = {};
    ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = debugCallback;
}

bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice d) {
    QueueFamilyIndices idx = findQueueFamilies(d);
    bool extOK = checkDeviceExtensionSupport(d);
    bool scOK = false;
    if (extOK) {
        auto sc = querySwapChainSupport(d);
        scOK = !sc.formats.empty() && !sc.presentModes.empty();
    }

    VkPhysicalDeviceDynamicRenderingFeatures dyn{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
    VkPhysicalDeviceFeatures2 f2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    f2.pNext = &dyn;
    vkGetPhysicalDeviceFeatures2(d, &f2);

    return idx.isComplete() && extOK && scOK && dyn.dynamicRendering;
}

bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice d) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(d, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> avail(count);
    vkEnumerateDeviceExtensionProperties(d, nullptr, &count, avail.data());

    std::set<std::string> req(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& e : avail) req.erase(e.extensionName);
    return req.empty();
}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice d) {
    QueueFamilyIndices idx;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(d, &count, nullptr);
    std::vector<VkQueueFamilyProperties> fam(count);
    vkGetPhysicalDeviceQueueFamilyProperties(d, &count, fam.data());
    int i = 0;
    for (const auto& qf : fam) {
        if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) idx.graphicsFamily = i;
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &presentSupport);
        if (presentSupport) idx.presentFamily = i;
        if (idx.isComplete()) break;
        ++i;
    }
    return idx;
}

SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice d) {
    SwapChainSupportDetails det;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, surface, &det.capabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &count, nullptr);
    if (count) {
        det.formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &count, det.formats.data());
    }

    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &count, nullptr);
    if (count) {
        det.presentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &count, det.presentModes.data());
    }
    return det;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    }
    return formats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return caps.currentExtent;

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    VkExtent2D actual = { (uint32_t)w, (uint32_t)h };
    actual.width = std::clamp(actual.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    actual.height = std::clamp(actual.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return actual;
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() {
    uint32_t count = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&count);
    std::vector<const char*> v(exts, exts + count);
    if (enableValidationLayers) v.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return v;
}

bool HelloTriangleApplication::checkValidationLayerSupport() {
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> props(count);
    vkEnumerateInstanceLayerProperties(&count, props.data());
    for (const char* name : validationLayers) {
        bool found = false;
        for (const auto& p : props) if (strcmp(name, p.layerName) == 0) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

std::vector<char> HelloTriangleApplication::readFile(const std::string& filename) {
    std::ifstream f(filename, std::ios::ate | std::ios::binary);
    if (!f.is_open()) throw std::runtime_error("failed to open file: " + filename);
    size_t size = (size_t)f.tellg();
    std::vector<char> buf(size);
    f.seekg(0);
    f.read(buf.data(), size);
    f.close();
    return buf;
}

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    if (vkCreateShaderModule(device, &ci, nullptr, &module) != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module!");
    return module;
}

void HelloTriangleApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
    VkBuffer& buffer, VkDeviceMemory& mem) {
    VkBufferCreateInfo bi{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bi, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("failed to create buffer!");

    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(device, buffer, &req);

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);
    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate buffer memory!");

    vkBindBufferMemory(device, buffer, mem, 0);
}

void HelloTriangleApplication::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool = commandPool;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &ai, &cmd);

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &bi);

    VkBufferCopy copy{};
    copy.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copy);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void HelloTriangleApplication::createImage(uint32_t w, uint32_t h, VkFormat format, VkImageUsageFlags usage,
    VkImage& image, VkDeviceMemory& mem) {
    VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.extent = { w, h, 1 };
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.format = format;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ci.usage = usage;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &ci, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("failed to create image!");

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device, image, &req);

    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate image memory!");

    vkBindImageMemory(device, image, mem, 0);
}

VkImageView HelloTriangleApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask) {
    VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vi.image = image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = format;
    vi.subresourceRange.aspectMask = aspectMask;
    vi.subresourceRange.baseMipLevel = 0;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount = 1;

    VkImageView view{};
    if (vkCreateImageView(device, &vi, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("failed to create image view!");
    return view;
}

/////////////////////////////////////////////////////
// Callbacks
/////////////////////////////////////////////////////

void HelloTriangleApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData
) {
    std::cerr << "Validation layer: " << data->pMessage << std::endl;
    return VK_FALSE;
}

/////////////////////////////////////////////////////
// main
/////////////////////////////////////////////////////

int main() {
    HelloTriangleApplication app;
    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
