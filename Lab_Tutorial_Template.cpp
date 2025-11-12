//==================================================
// Vulkan 1.3 — Single pipeline, push-constant light sphere gizmo
//====================================================

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
#include <cmath>

// --- Small step logger (helps catch where init dies) ---
#define STEP(msg) do { std::cerr << "[STEP] " << msg << std::endl; } while(0)

static void glfwErrorCallback(int code, const char* desc) {
    std::cerr << "[GLFW] (" << code << ") " << desc << std::endl;
}

static constexpr bool USE_DEPTH = true;

// --- Configuration ---
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

// --- Helper Structs ---
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

// --- Vertex / UBO / Push Constant ---
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 binormal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription b{};
        b.binding = 0;
        b.stride = sizeof(Vertex);
        b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return b;
    }

    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 6> a{};

        a[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) };
        a[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) };
        a[2] = { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) };
        a[3] = { 3, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv) };
        a[4] = { 4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent) };
        a[5] = { 5, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, binormal) };

        return a;
    }
};


struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 eyePos;
};

struct PushConstants {
    glm::mat4 modelOverride; // 64 bytes
    uint32_t  useOverride;   // 4
    uint32_t  unlit;         // 4
    uint32_t  _pad0;         // 4
    uint32_t  _pad1;         // 4   -> total 80 bytes
};

auto faceUV = [&](float S, glm::vec2 uv) { return uv; };
std::vector<Vertex> cubeVertices = {
    // Front (+Z)
    {{-0.5f,-0.5f,  0.5f}, {1,0,0}, {0,0, 1}, faceUV(1.0f,{0,1}), {1,0,0}, {0,1,0}},
    {{ 0.5f,-0.5f,  0.5f}, {1,0,0}, {0,0, 1}, faceUV(1.0f,{1,1}), {1,0,0}, {0,1,0}},
    {{ 0.5f, 0.5f,  0.5f}, {1,0,0}, {0,0, 1}, faceUV(1.0f,{1,0}), {1,0,0}, {0,1,0}},
    {{ 0.5f, 0.5f,  0.5f}, {1,0,0}, {0,0, 1}, faceUV(1.0f,{1,0}), {1,0,0}, {0,1,0}},
    {{-0.5f, 0.5f,  0.5f}, {1,0,0}, {0,0, 1}, faceUV(1.0f,{0,0}), {1,0,0}, {0,1,0}},
    {{-0.5f,-0.5f,  0.5f}, {1,0,0}, {0,0, 1}, faceUV(1.0f,{0,1}), {1,0,0}, {0,1,0}},

    // Back (−Z)
    {{ 0.5f,-0.5f, -0.5f}, {0,1,0}, {0,0,-1}, faceUV(2.0f,{0,1}), {-1,0,0}, {0,1,0}},
    {{-0.5f,-0.5f, -0.5f}, {0,1,0}, {0,0,-1}, faceUV(2.0f,{1,1}), {-1,0,0}, {0,1,0}},
    {{-0.5f, 0.5f, -0.5f}, {0,1,0}, {0,0,-1}, faceUV(2.0f,{1,0}), {-1,0,0}, {0,1,0}},
    {{-0.5f, 0.5f, -0.5f}, {0,1,0}, {0,0,-1}, faceUV(2.0f,{1,0}), {-1,0,0}, {0,1,0}},
    {{ 0.5f, 0.5f, -0.5f}, {0,1,0}, {0,0,-1}, faceUV(2.0f,{0,0}), {-1,0,0}, {0,1,0}},
    {{ 0.5f,-0.5f, -0.5f}, {0,1,0}, {0,0,-1}, faceUV(2.0f,{0,1}), {-1,0,0}, {0,1,0}},

    // Left (−X)
    {{-0.5f,-0.5f, -0.5f}, {0,0,1}, {-1,0,0}, faceUV(3.0f,{0,1}), {0,0,1}, {0,1,0}},
    {{-0.5f,-0.5f,  0.5f}, {0,0,1}, {-1,0,0}, faceUV(3.0f,{1,1}), {0,0,1}, {0,1,0}},
    {{-0.5f, 0.5f,  0.5f}, {0,0,1}, {-1,0,0}, faceUV(3.0f,{1,0}), {0,0,1}, {0,1,0}},
    {{-0.5f, 0.5f,  0.5f}, {0,0,1}, {-1,0,0}, faceUV(3.0f,{1,0}), {0,0,1}, {0,1,0}},
    {{-0.5f, 0.5f, -0.5f}, {0,0,1}, {-1,0,0}, faceUV(3.0f,{0,0}), {0,0,1}, {0,1,0}},
    {{-0.5f,-0.5f, -0.5f}, {0,0,1}, {-1,0,0}, faceUV(3.0f,{0,1}), {0,0,1}, {0,1,0}},

    // Right (+X)
    {{ 0.5f,-0.5f,  0.5f}, {1,1,0}, {1,0,0}, faceUV(4.0f,{0,1}), {0,0,-1}, {0,1,0}},
    {{ 0.5f,-0.5f, -0.5f}, {1,1,0}, {1,0,0}, faceUV(4.0f,{1,1}), {0,0,-1}, {0,1,0}},
    {{ 0.5f, 0.5f, -0.5f}, {1,1,0}, {1,0,0}, faceUV(4.0f,{1,0}), {0,0,-1}, {0,1,0}},
    {{ 0.5f, 0.5f, -0.5f}, {1,1,0}, {1,0,0}, faceUV(4.0f,{1,0}), {0,0,-1}, {0,1,0}},
    {{ 0.5f, 0.5f,  0.5f}, {1,1,0}, {1,0,0}, faceUV(4.0f,{0,0}), {0,0,-1}, {0,1,0}},
    {{ 0.5f,-0.5f,  0.5f}, {1,1,0}, {1,0,0}, faceUV(4.0f,{0,1}), {0,0,-1}, {0,1,0}},

    // Top (+Y)
    {{-0.5f, 0.5f,  0.5f}, {1,0,1}, {0,1,0}, faceUV(5.0f,{0,1}), {1,0,0}, {0,0,-1}},
    {{ 0.5f, 0.5f,  0.5f}, {1,0,1}, {0,1,0}, faceUV(5.0f,{1,1}), {1,0,0}, {0,0,-1}},
    {{ 0.5f, 0.5f, -0.5f}, {1,0,1}, {0,1,0}, faceUV(5.0f,{1,0}), {1,0,0}, {0,0,-1}},
    {{ 0.5f, 0.5f, -0.5f}, {1,0,1}, {0,1,0}, faceUV(5.0f,{1,0}), {1,0,0}, {0,0,-1}},
    {{-0.5f, 0.5f, -0.5f}, {1,0,1}, {0,1,0}, faceUV(5.0f,{0,0}), {1,0,0}, {0,0,-1}},
    {{-0.5f, 0.5f,  0.5f}, {1,0,1}, {0,1,0}, faceUV(5.0f,{0,1}), {1,0,0}, {0,0,-1}},

    // Bottom (−Y)
    {{-0.5f,-0.5f, -0.5f}, {0,1,1}, {0,-1,0}, faceUV(6.0f,{0,1}), {1,0,0}, {0,0,1}},
    {{ 0.5f,-0.5f, -0.5f}, {0,1,1}, {0,-1,0}, faceUV(6.0f,{1,1}), {1,0,0}, {0,0,1}},
    {{ 0.5f,-0.5f,  0.5f}, {0,1,1}, {0,-1,0}, faceUV(6.0f,{1,0}), {1,0,0}, {0,0,1}},
    {{ 0.5f,-0.5f,  0.5f}, {0,1,1}, {0,-1,0}, faceUV(6.0f,{1,0}), {1,0,0}, {0,0,1}},
    {{-0.5f,-0.5f,  0.5f}, {0,1,1}, {0,-1,0}, faceUV(6.0f,{0,0}), {1,0,0}, {0,0,1}},
    {{-0.5f,-0.5f, -0.5f}, {0,1,1}, {0,-1,0}, faceUV(6.0f,{0,1}), {1,0,0}, {0,0,1}},
};

// --- Forward declarations of Vulkan helpers ---
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*,
    const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*);
void DestroyDebugUtilsMessengerEXT(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks*);
// ---- Debug utils loader helpers (define these in your .cpp) ----
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func) {
        func(instance, debugMessenger, pAllocator);
    }
}

class HelloTriangleApplication {
public:
    void run();

private:
    // --- Camera state ---
    glm::vec3 camPos{ 0.0f, 1.0f, 2.0f };
    float yaw = -90.0f;   // looking toward -Z initially
    float pitch = 0.0f;
    float moveSpeed = 2.5f; // units per second
    float turnSpeed = 90.0f; // degrees per second

    void handleInput(float dt);

    // Core
    GLFWwindow* window = {};
    bool framebufferResized = false;
    uint32_t currentFrame = 0;

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

	// Depth resources
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    
    // Pipeline / descriptors
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    // Texture
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;

    VkImage textureImage2;
    VkDeviceMemory textureImageMemory2;
    VkImageView textureImageView2;
    VkSampler textureSampler2;

    VkImage textureImage3 = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory3 = VK_NULL_HANDLE;
    VkImageView textureImageView3 = VK_NULL_HANDLE;
    VkSampler textureSampler3 = VK_NULL_HANDLE;

	// Depth format finder
    VkFormat findDepthFormat();

    // Buffers
    VkBuffer cubeVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory cubeVertexBufferMemory = VK_NULL_HANDLE;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    // Descriptors
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    // Sync
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    // Flow
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Init
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();

    //void createTextureImage();
    //void createTextureImageView();
    //void createTextureSampler();

    //void createTextureImage2();
    //void createTextureImageView2();
    //void createTextureSampler2();

    void createTextureImage3();
    void createTextureImageView3();
    void createTextureSampler3();

    void createVertexBuffers();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    // Draw / swapchain
    void drawFrame();
    void recreateSwapChain();
    void cleanupSwapChain();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateUniformBuffer(uint32_t currentImage);

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
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask);
    void createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageTiling tiling,
        VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkImage& image, VkDeviceMemory& memory);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer);

    void transitionImageLayout(VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t w, uint32_t h);

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

// --- Implementation --------------------------------------------------------
void HelloTriangleApplication::handleInput(float dt) {
    // Compute basis from yaw/pitch
    glm::vec3 front{
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    };
    front = glm::normalize(front);
    const glm::vec3 worldUp{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    float speed = moveSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 3.0f; // sprint

    // Translate
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += front * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= front * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos += up * speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camPos -= up * speed;

    // Rotate with arrow keys
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  yaw -= turnSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)  yaw += turnSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)  pitch += turnSpeed * dt;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  pitch -= turnSpeed * dt;

    // Clamp pitch to avoid gimbal flip
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}

void HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) throw std::runtime_error("GLFW init failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan 1.3 - Light Sphere Gizmo", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void HelloTriangleApplication::initVulkan() {
    STEP("createInstance");        createInstance();
    STEP("setupDebugMessenger");   setupDebugMessenger();
    STEP("createSurface");         createSurface();
    STEP("pickPhysicalDevice");    pickPhysicalDevice();
    STEP("createLogicalDevice");   createLogicalDevice();
    
    STEP("createSwapChain");       createSwapChain();
    STEP("createImageViews");      createImageViews();
    STEP("createDescriptorSetLayout"); createDescriptorSetLayout();
    STEP("createGraphicsPipeline"); createGraphicsPipeline();
    STEP("createCommandPool");     createCommandPool();
    STEP("createDepthResources");  createDepthResources();

    //STEP("createTextureImage");    createTextureImage();
    //STEP("createTextureImageView"); createTextureImageView();
    //STEP("createTextureSampler");  createTextureSampler();

    //STEP("createTextureImage2");    createTextureImage2();
    //STEP("createTextureImageView2"); createTextureImageView2();
    //STEP("createTextureSampler2");  createTextureSampler2();

    STEP("createTextureImage3");    createTextureImage3();
    STEP("createTextureImageView3"); createTextureImageView3();
    STEP("createTextureSampler3");  createTextureSampler3();



    STEP("build geometry");

    STEP("createVertexBuffers");
    createVertexBuffers();

    STEP("createUniformBuffers");  createUniformBuffers();
    STEP("createDescriptorPool");  createDescriptorPool();
    STEP("createDescriptorSets");  createDescriptorSets();
    STEP("createCommandBuffers");  createCommandBuffers();
    STEP("createSyncObjects");     createSyncObjects();
}

void HelloTriangleApplication::mainLoop() {
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float dt = static_cast<float>(now - last);
        last = now;

        handleInput(dt);   // <-- add this line
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}


void HelloTriangleApplication::cleanup() {
    cleanupSwapChain();

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    //vkDestroySampler(device, textureSampler, nullptr);
    //vkDestroyImageView(device, textureImageView, nullptr);
    //vkDestroyImage(device, textureImage, nullptr);
    //vkFreeMemory(device, textureImageMemory, nullptr);
    //
    //vkDestroySampler(device, textureSampler2, nullptr);
    //vkDestroyImageView(device, textureImageView2, nullptr);
    //vkDestroyImage(device, textureImage2, nullptr);
    //vkFreeMemory(device, textureImageMemory2, nullptr);

    vkDestroySampler(device, textureSampler3, nullptr);
    vkDestroyImageView(device, textureImageView3, nullptr);
    vkDestroyImage(device, textureImage3, nullptr);
    vkFreeMemory(device, textureImageMemory3, nullptr);


    if (cubeVertexBuffer) {
        vkDestroyBuffer(device, cubeVertexBuffer, nullptr);
        vkFreeMemory(device, cubeVertexBufferMemory, nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

// --- Vulkan setup (mostly identical to your version) -----------------------

void HelloTriangleApplication::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available!");

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Light Sphere Gizmo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

void HelloTriangleApplication::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& ci) {
    ci = {};
    ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = debugCallback;
}
void HelloTriangleApplication::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT ci{};
    populateDebugMessengerCreateInfo(ci);
    if (CreateDebugUtilsMessengerEXT(instance, &ci, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger!");
}
bool HelloTriangleApplication::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* layerName : validationLayers) {
        bool found = false;
        for (auto& lp : availableLayers) {
            if (strcmp(layerName, lp.layerName) == 0) { found = true; break; }
        }
        if (!found) return false;
    }
    return true;
}
std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    return extensions;
}
VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* cb, void*) {
    std::cerr << "[Vulkan] " << cb->pMessage << std::endl;
    return VK_FALSE;
}

void HelloTriangleApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice dev) {
    QueueFamilyIndices indices;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
    std::vector<VkQueueFamilyProperties> qf(count);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, qf.data());
    for (uint32_t i = 0; i < count; i++) {
        if (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
        if (presentSupport) indices.presentFamily = i;
        if (indices.isComplete()) break;
    }
    return indices;
}
bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice dev) {
    uint32_t extensionCount = 0; vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> exts(extensionCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, exts.data());
    std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
    for (auto& e : exts) required.erase(e.extensionName);
    return required.empty();
}
SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice dev) {
    SwapChainSupportDetails d;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &d.capabilities);
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, nullptr);
    if (count) { d.formats.resize(count); vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &count, d.formats.data()); }
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &count, nullptr);
    if (count) { d.presentModes.resize(count); vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &count, d.presentModes.data()); }
    return d;
}
bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice dev) {
    QueueFamilyIndices indices = findQueueFamilies(dev);
    if (!indices.isComplete()) return false;
    if (!checkDeviceExtensionSupport(dev)) return false;

    SwapChainSupportDetails sc = querySwapChainSupport(dev);
    if (sc.formats.empty() || sc.presentModes.empty()) return false;

    VkPhysicalDeviceDynamicRenderingFeatures drf{};
    drf.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    VkPhysicalDeviceFeatures2 f2{};
    f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    f2.pNext = &drf;
    vkGetPhysicalDeviceFeatures2(dev, &f2);
    return drf.dynamicRendering == VK_TRUE;
}
void HelloTriangleApplication::pickPhysicalDevice() {
    uint32_t deviceCount = 0; vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (!deviceCount) throw std::runtime_error("No GPUs with Vulkan support");
    std::vector<VkPhysicalDevice> devs(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devs.data());
    for (auto& d : devs) { if (isDeviceSuitable(d)) { physicalDevice = d; break; } }
    if (physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("No suitable GPU found");
}
void HelloTriangleApplication::createLogicalDevice() {
    QueueFamilyIndices idx = findQueueFamilies(physicalDevice);
    std::set<uint32_t> unique{ idx.graphicsFamily.value(), idx.presentFamily.value() };
    std::vector<VkDeviceQueueCreateInfo> qinfos;
    float prio = 1.0f;
    for (auto qf : unique) {
        VkDeviceQueueCreateInfo qi{};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = qf; qi.queueCount = 1; qi.pQueuePriorities = &prio;
        qinfos.push_back(qi);
    }

    VkPhysicalDeviceDynamicRenderingFeatures drf{};
    drf.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES; drf.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceSynchronization2Features sync2{};
    sync2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES; sync2.synchronization2 = VK_TRUE;
    drf.pNext = &sync2;

    VkPhysicalDeviceFeatures2 f2{};
    f2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    f2.pNext = &drf;
    f2.features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pNext = &f2;
    ci.queueCreateInfoCount = (uint32_t)qinfos.size();
    ci.pQueueCreateInfos = qinfos.data();
    ci.pEnabledFeatures = nullptr;
    ci.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    ci.ppEnabledExtensionNames = deviceExtensions.data();
    if (enableValidationLayers) {
        ci.enabledLayerCount = (uint32_t)validationLayers.size();
        ci.ppEnabledLayerNames = validationLayers.data();
    }

    if (vkCreateDevice(physicalDevice, &ci, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device");

    vkGetDeviceQueue(device, idx.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, idx.presentFamily.value(), 0, &presentQueue);
}
VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& af) {
    for (auto& f : af) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    }
    return af[0];
}
VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& ap) {
    for (auto& p : ap) if (p == VK_PRESENT_MODE_MAILBOX_KHR) return p;
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& caps) {
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) return caps.currentExtent;
    int w, h; glfwGetFramebufferSize(window, &w, &h);
    VkExtent2D e{ (uint32_t)w, (uint32_t)h };
    e.width = std::clamp(e.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    e.height = std::clamp(e.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}
void HelloTriangleApplication::createSwapChain() {
    auto sup = querySwapChainSupport(physicalDevice);
    auto fmt = chooseSwapSurfaceFormat(sup.formats);
    auto pm = chooseSwapPresentMode(sup.presentModes);
    auto ext = chooseSwapExtent(sup.capabilities);

    uint32_t imageCount = sup.capabilities.minImageCount + 1;
    if (sup.capabilities.maxImageCount > 0 && imageCount > sup.capabilities.maxImageCount)
        imageCount = sup.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = surface;
    ci.minImageCount = imageCount;
    ci.imageFormat = fmt.format;
    ci.imageColorSpace = fmt.colorSpace;
    ci.imageExtent = ext;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices idx = findQueueFamilies(physicalDevice);
    uint32_t qidx[] = { idx.graphicsFamily.value(), idx.presentFamily.value() };
    if (idx.graphicsFamily != idx.presentFamily) {
        ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = qidx;
    }
    else {
        ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    ci.preTransform = sup.capabilities.currentTransform;
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = pm;
    ci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swapchain");

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = fmt.format;
    swapChainExtent = ext;
}
void HelloTriangleApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}
VkFormat HelloTriangleApplication::findDepthFormat() {
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported depth format!");
}

void HelloTriangleApplication::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = createImageView(
        depthImage,
        depthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    transitionImageLayout(
        depthImage,
        depthFormat,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}



void HelloTriangleApplication::createDescriptorSetLayout() {
    // binding 0 = UBO
    VkDescriptorSetLayoutBinding ubo{};
    ubo.binding = 0;
    ubo.descriptorCount = 1;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding sky{};
    sky.binding = 1;
    sky.descriptorCount = 1;
    sky.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sky.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings{ ubo, sky};

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}


void HelloTriangleApplication::createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");
    VkShaderModule vs = createShaderModule(vertShaderCode);
    VkShaderModule fs = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vssi{};
    vssi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vssi.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vssi.module = vs;
    vssi.pName = "main";

    VkPipelineShaderStageCreateInfo fssi{};
    fssi.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fssi.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fssi.module = fs;
    fssi.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vssi, fssi };

    auto bindDesc = Vertex::getBindingDescription();
    auto attrDesc = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &bindDesc;
    vi.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDesc.size());
    vi.pVertexAttributeDescriptions = attrDesc.data();

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_FRONT_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cba.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    std::vector<VkDynamicState> dyn = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ds.dynamicStateCount = (uint32_t)dyn.size();
    ds.pDynamicStates = dyn.data();

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.offset = 0;
    pcr.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pl{};
    pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl.setLayoutCount = 1;
    pl.pSetLayouts = &descriptorSetLayout;
    pl.pushConstantRangeCount = 1;
    pl.pPushConstantRanges = &pcr;

    if (vkCreatePipelineLayout(device, &pl, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");
    
    auto makeDepthState = [&](VkBool32 test, VkBool32 write, VkCompareOp op) {
        VkPipelineDepthStencilStateCreateInfo d{};
        d.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        d.depthTestEnable = test;
        d.depthWriteEnable = write;
        d.depthCompareOp = op;
        d.depthBoundsTestEnable = VK_FALSE;
        d.stencilTestEnable = VK_FALSE;
        return d;
        };

    // NEW: Depth stencil state
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = USE_DEPTH ? VK_TRUE : VK_FALSE;
    depth.depthWriteEnable = VK_FALSE; // USE_DEPTH ? VK_TRUE : VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth.depthBoundsTestEnable = VK_FALSE;
    depth.stencilTestEnable = VK_FALSE;


    // Dynamic rendering info includes depth format
    VkFormat depthFormat = findDepthFormat();

    VkPipelineRenderingCreateInfo rend{};
    rend.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rend.colorAttachmentCount = 1;
    rend.pColorAttachmentFormats = &swapChainImageFormat;
    rend.depthAttachmentFormat = depthFormat;

    VkGraphicsPipelineCreateInfo gp{};
    gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp.pNext = &rend;
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &ds;
    gp.pDepthStencilState = &depth; // <-- Added
    gp.layout = pipelineLayout;
    gp.renderPass = VK_NULL_HANDLE;
    gp.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, nullptr, &graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");

    vkDestroyShaderModule(device, fs, nullptr);
    vkDestroyShaderModule(device, vs, nullptr);
}


void HelloTriangleApplication::createCommandPool() {
    QueueFamilyIndices q = findQueueFamilies(physicalDevice);
    VkCommandPoolCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = q.graphicsFamily.value();
    if (vkCreateCommandPool(device, &ci, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");
}


//void HelloTriangleApplication::createTextureImage3() {
//    int w, h, ch;
//    stbi_uc* pixels = stbi_load("rockHeight.tga", &w, &h, &ch, STBI_rgb_alpha);
//    if (!pixels) {
//        throw std::runtime_error("failed to load height.png");
//    }
//
//    VkDeviceSize imageSize = (VkDeviceSize)w * h * 4;
//
//    VkBuffer staging;
//    VkDeviceMemory stagingMem;
//    createBuffer(
//        imageSize,
//        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//        staging,
//        stagingMem
//    );
//
//    void* data;
//    vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
//    memcpy(data, pixels, (size_t)imageSize);
//    vkUnmapMemory(device, stagingMem);
//    stbi_image_free(pixels);
//
//    createImage(
//        (uint32_t)w,
//        (uint32_t)h,
//        VK_FORMAT_R8G8B8A8_SRGB,
//        VK_IMAGE_TILING_OPTIMAL,
//        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
//        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//        textureImage3,
//        textureImageMemory3
//    );
//
//    transitionImageLayout(
//        textureImage3,
//        VK_FORMAT_R8G8B8A8_SRGB,
//        VK_IMAGE_LAYOUT_UNDEFINED,
//        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//        VK_IMAGE_ASPECT_COLOR_BIT
//    );
//
//    copyBufferToImage(staging, textureImage3, (uint32_t)w, (uint32_t)h);
//
//    transitionImageLayout(
//        textureImage3,
//        VK_FORMAT_R8G8B8A8_SRGB,
//        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//        VK_IMAGE_ASPECT_COLOR_BIT
//    );
//
//    vkDestroyBuffer(device, staging, nullptr);
//    vkFreeMemory(device, stagingMem, nullptr);
//}

void HelloTriangleApplication::createTextureImage3() {
    // Order: +X, -X, +Y, -Y, +Z, -Z (match your shader)
    const char* faceFiles[6] = {
        "cubemap_0.jpg","cubemap_1.jpg","cubemap_2.jpg",
        "cubemap_3.jpg","cubemap_4.jpg","cubemap_5.jpg"
    };

    int W = 0, H = 0, C = 0;
    std::vector<stbi_uc*> faces(6, nullptr);
    for (int i = 0; i < 6; i++) {
        faces[i] = stbi_load(faceFiles[i], &W, &H, &C, STBI_rgb_alpha);
        if (!faces[i]) throw std::runtime_error(std::string("failed to load ") + faceFiles[i]);
    }

    const VkDeviceSize faceSize = (VkDeviceSize)W * H * 4;
    const VkDeviceSize total = faceSize * 6;

    // staging buffer for all 6 faces (back-to-back)
    VkBuffer staging; VkDeviceMemory stagingMem;
    createBuffer(total, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    void* data; vkMapMemory(device, stagingMem, 0, total, 0, &data);
    for (int i = 0; i < 6; i++) {
        memcpy((uint8_t*)data + faceSize * i, faces[i], (size_t)faceSize);
        stbi_image_free(faces[i]);
    }
    vkUnmapMemory(device, stagingMem);

    // create cubemap image in textureImage3/textureImageMemory3
    VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.extent = { (uint32_t)W, (uint32_t)H, 1 };
    ci.mipLevels = 1;
    ci.arrayLayers = 6;
    ci.format = VK_FORMAT_R8G8B8A8_SRGB;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &ci, nullptr, &textureImage3) != VK_SUCCESS)
        throw std::runtime_error("create cubemap image failed");

    VkMemoryRequirements req{};
    vkGetImageMemoryRequirements(device, textureImage3, &req);
    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &ai, nullptr, &textureImageMemory3) != VK_SUCCESS)
        throw std::runtime_error("alloc cubemap mem failed");
    vkBindImageMemory(device, textureImage3, textureImageMemory3, 0);

    // UNDEFINED → TRANSFER_DST_OPTIMAL (all 6 layers)
    VkCommandBuffer cmd = beginSingleTimeCommands();
    VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = textureImage3;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.baseMipLevel = 0; b.subresourceRange.levelCount = 1;
    b.subresourceRange.baseArrayLayer = 0; b.subresourceRange.layerCount = 6;
    b.srcAccessMask = 0;
    b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &b);

    // copy each face into its array layer
    std::vector<VkBufferImageCopy> copies(6);
    for (uint32_t i = 0; i < 6; i++) {
        copies[i].bufferOffset = faceSize * i;
        copies[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copies[i].imageSubresource.mipLevel = 0;
        copies[i].imageSubresource.baseArrayLayer = i;
        copies[i].imageSubresource.layerCount = 1;
        copies[i].imageExtent = { (uint32_t)W,(uint32_t)H,1 };
    }
    vkCmdCopyBufferToImage(cmd, staging, textureImage3,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        (uint32_t)copies.size(), copies.data());

    // TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL (all layers)
    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &b);

    endSingleTimeCommands(cmd);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
}


void HelloTriangleApplication::createTextureImageView3() {
    VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vi.image = textureImage3;
    vi.viewType = VK_IMAGE_VIEW_TYPE_CUBE;          // <-- cube view
    vi.format = VK_FORMAT_R8G8B8A8_SRGB;
    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.baseMipLevel = 0;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount = 6;            // <-- six faces
    if (vkCreateImageView(device, &vi, nullptr, &textureImageView3) != VK_SUCCESS)
        throw std::runtime_error("failed to create cubemap image view");
}


void HelloTriangleApplication::createTextureSampler3() {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);

    VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;

    // Clamp to edge is standard for cubemaps to avoid seams
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    info.anisotropyEnable = VK_TRUE;
    info.maxAnisotropy = props.limits.maxSamplerAnisotropy;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &info, nullptr, &textureSampler3) != VK_SUCCESS)
        throw std::runtime_error("failed to create sampler3");
}


// --- Vertex buffers for cube and sphere ------------------------------------
void HelloTriangleApplication::createVertexBuffers() {
    auto makeVB = [&](const std::vector<Vertex>& verts, VkBuffer& buf, VkDeviceMemory& mem) {
        VkDeviceSize size = sizeof(Vertex) * verts.size();
        VkBuffer staging; VkDeviceMemory stagingMem;
        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            staging, stagingMem);
        void* data; vkMapMemory(device, stagingMem, 0, size, 0, &data);
        memcpy(data, verts.data(), (size_t)size);
        vkUnmapMemory(device, stagingMem);

        createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buf, mem);
        copyBuffer(staging, buf, size);
        vkDestroyBuffer(device, staging, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);
        };

    makeVB(cubeVertices, cubeVertexBuffer, cubeVertexBufferMemory);
}

// --- UBO / descriptors / command buffers / sync ----------------------------
void HelloTriangleApplication::createUniformBuffers() {
    VkDeviceSize bs = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bs, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i], uniformBuffersMemory[i]);
        vkMapMemory(device, uniformBuffersMemory[i], 0, bs, 0, &uniformBuffersMapped[i]);
    }
}

void HelloTriangleApplication::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    // UBOs
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    // first sampler
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        // height
        VkDescriptorImageInfo imageInfo3{};
        imageInfo3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo3.imageView = textureImageView3;
        imageInfo3.sampler = textureSampler3;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        // binding 0 = UBO
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo3;

        vkUpdateDescriptorSets(device,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0, nullptr);
    }
}


void HelloTriangleApplication::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = commandPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = (uint32_t)commandBuffers.size();
    if (vkAllocateCommandBuffers(device, &ai, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");
}

void HelloTriangleApplication::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo si{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fi{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &si, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &si, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fi, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects");
    }
}

// --- Per-frame -------------------------------------------------------------

void HelloTriangleApplication::updateUniformBuffer(uint32_t frame) {
    UniformBufferObject u{};

    // Model can stay as you had it
    u.model = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    // Recompute front/up from current yaw/pitch
    glm::vec3 front{
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    };
    front = glm::normalize(front);
    const glm::vec3 worldUp{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    u.view = glm::lookAt(camPos, camPos + front, up);
    u.proj = glm::perspective(glm::radians(45.0f),
        swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
    u.proj[1][1] *= -1;

    u.lightPos = glm::vec3(2.0f, 2.0f, 2.0f); // as before
    u.eyePos = camPos;

    memcpy(uniformBuffersMapped[frame], &u, sizeof(u));
}


void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer cb, uint32_t imageIndex) {
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    if (vkBeginCommandBuffer(cb, &bi) != VK_SUCCESS)
        throw std::runtime_error("BeginCmdBuffer failed");

    // Transition swapchain image for rendering
    VkImageMemoryBarrier2 toAtt{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    toAtt.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    toAtt.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toAtt.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    toAtt.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toAtt.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toAtt.image = swapChainImages[imageIndex];
    toAtt.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0,1,0,1 };

    VkDependencyInfo depToAtt{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depToAtt.imageMemoryBarrierCount = 1;
    depToAtt.pImageMemoryBarriers = &toAtt;
    vkCmdPipelineBarrier2(cb, &depToAtt);

    // Attachments: color + depth
    VkRenderingAttachmentInfo color{};
    color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color.imageView = swapChainImageViews[imageIndex];
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.clearValue.color = { {0.02f, 0.02f, 0.02f, 1.0f} };

    VkRenderingAttachmentInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
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

    vkCmdBeginRendering(cb, &ri);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport vp{ 0, 0, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f };
    VkRect2D sc{ {0,0}, swapChainExtent };
    vkCmdSetViewport(cb, 0, 1, &vp);
    vkCmdSetScissor(cb, 0, 1, &sc);

    // bind descriptors
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    // VB: reuse your cube geometry
    VkBuffer vbs[] = { cubeVertexBuffer };
    VkDeviceSize offs[] = { 0 };
    vkCmdBindVertexBuffers(cb, 0, 1, vbs, offs);

    // push constants (unlit skybox)
    PushConstants sky{};
    sky.useOverride = 1u;
    sky.unlit = 1u;
    sky.modelOverride = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
    vkCmdPushConstants(cb, pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, (uint32_t)sizeof(PushConstants), &sky);

    // draw once
    vkCmdDraw(cb, (uint32_t)cubeVertices.size(), 1, 0, 0);

    vkCmdEndRendering(cb);


    // Transition for presentation
    VkImageMemoryBarrier2 toPresent{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.image = swapChainImages[imageIndex];
    toPresent.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0,1,0,1 };

    VkDependencyInfo depPresent{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depPresent.imageMemoryBarrierCount = 1;
    depPresent.pImageMemoryBarriers = &toPresent;
    vkCmdPipelineBarrier2(cb, &depPresent);

    if (vkEndCommandBuffer(cb) != VK_SUCCESS)
        throw std::runtime_error("EndCmdBuffer failed");
}


void HelloTriangleApplication::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acq = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapChain(); return; }
    else if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain image");

    updateUniformBuffer(currentFrame);

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkCommandBufferSubmitInfo cbsi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
    cbsi.commandBuffer = commandBuffers[currentFrame];

    VkSemaphoreSubmitInfo waitSem{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
    waitSem.semaphore = imageAvailableSemaphores[currentFrame];
    waitSem.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signalSem{ VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO };
    signalSem.semaphore = renderFinishedSemaphores[currentFrame];
    signalSem.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 si{ VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
    si.waitSemaphoreInfoCount = 1; si.pWaitSemaphoreInfos = &waitSem;
    si.commandBufferInfoCount = 1; si.pCommandBufferInfos = &cbsi;
    si.signalSemaphoreInfoCount = 1; si.pSignalSemaphoreInfos = &signalSem;

    if (vkQueueSubmit2(graphicsQueue, 1, &si, inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("QueueSubmit2 failed");

    VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
    VkSwapchainKHR scs[] = { swapChain };
    pi.swapchainCount = 1; pi.pSwapchains = scs;
    pi.pImageIndices = &imageIndex;

    VkResult pres = vkQueuePresentKHR(presentQueue, &pi);
    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (pres != VK_SUCCESS) {
        throw std::runtime_error("QueuePresent failed");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::recreateSwapChain() {
    int w = 0, h = 0; glfwGetFramebufferSize(window, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(window, &w, &h);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();   // <-- add this
}

void HelloTriangleApplication::cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (auto v : swapChainImageViews)
        vkDestroyImageView(device, v, nullptr);

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}


// --- Helpers ---------------------------------------------------------------

std::vector<char> HelloTriangleApplication::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open file: " + filename);
    size_t size = (size_t)file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0); file.read(buffer.data(), size);
    return buffer;
}
VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule m;
    if (vkCreateShaderModule(device, &ci, nullptr, &m) != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module");
    return m;
}
void HelloTriangleApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
    VkBuffer& buf, VkDeviceMemory& mem) {
    VkBufferCreateInfo bi{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bi.size = size; bi.usage = usage; bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bi, nullptr, &buf) != VK_SUCCESS)
        throw std::runtime_error("failed to create buffer");
    VkMemoryRequirements req{}; vkGetBufferMemoryRequirements(device, buf, &req);
    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);
    if (vkAllocateMemory(device, &ai, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate buffer memory");
    vkBindBufferMemory(device, buf, mem, 0);
}
void HelloTriangleApplication::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool = commandPool;
    ai.commandBufferCount = 1;
    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &ai, &cb);
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);
    VkBufferCopy region{}; region.size = size;
    vkCmdCopyBuffer(cb, src, dst, 1, &region);
    vkEndCommandBuffer(cb);
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1; si.pCommandBuffers = &cb;
    vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cb);
}
uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp{}; vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (mp.memoryTypes[i].propertyFlags & props) == props) return i;
    }
    throw std::runtime_error("failed to find suitable memory type");
}
void HelloTriangleApplication::createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageTiling tiling,
    VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.extent = { w,h,1 };
    ci.mipLevels = 1; ci.arrayLayers = 1;
    ci.format = fmt; ci.tiling = tiling;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ci.usage = usage;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &ci, nullptr, &image) != VK_SUCCESS) throw std::runtime_error("createImage failed");
    VkMemoryRequirements req{}; vkGetImageMemoryRequirements(device, image, &req);
    VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = findMemoryType(req.memoryTypeBits, props);
    if (vkAllocateMemory(device, &ai, nullptr, &memory) != VK_SUCCESS) throw std::runtime_error("alloc image mem fail");
    vkBindImageMemory(device, image, memory, 0);
}
VkImageView HelloTriangleApplication::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    vi.image = image; vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.format = format;
    vi.subresourceRange.aspectMask = aspect;
    vi.subresourceRange.baseMipLevel = 0; vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.baseArrayLayer = 0; vi.subresourceRange.layerCount = 1;
    VkImageView view;
    if (vkCreateImageView(device, &vi, nullptr, &view) != VK_SUCCESS) throw std::runtime_error("createImageView fail");
    return view;
}
VkCommandBuffer HelloTriangleApplication::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandPool = commandPool;
    ai.commandBufferCount = 1;
    VkCommandBuffer cb; vkAllocateCommandBuffers(device, &ai, &cb);
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);
    return cb;
}
void HelloTriangleApplication::endSingleTimeCommands(VkCommandBuffer cb) {
    vkEndCommandBuffer(cb);
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1; si.pCommandBuffers = &cb;
    vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cb);
}

void HelloTriangleApplication::transitionImageLayout(
    VkImage image,
    VkFormat /*fmt*/,
    VkImageLayout oldL,
    VkImageLayout newL,
    VkImageAspectFlags aspect)
{
    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.oldLayout = oldL;
    b.newLayout = newL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange.aspectMask = aspect;
    b.subresourceRange.baseMipLevel = 0;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.baseArrayLayer = 0;
    b.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage{};
    VkPipelineStageFlags dstStage{};

    if (oldL == VK_IMAGE_LAYOUT_UNDEFINED &&
        newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        b.srcAccessMask = 0;
        b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    // NEW CASE: for depth images
    else if (oldL == VK_IMAGE_LAYOUT_UNDEFINED &&
        newL == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
        b.srcAccessMask = 0;
        b.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        cmd,
        srcStage, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &b
    );

    endSingleTimeCommands(cmd);
}


void HelloTriangleApplication::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t w, uint32_t h) {
    VkCommandBuffer cmd = beginSingleTimeCommands();
    VkBufferImageCopy r{};
    r.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    r.imageSubresource.layerCount = 1;
    r.imageExtent = { w,h,1 };
    vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);
    endSingleTimeCommands(cmd);
}

void HelloTriangleApplication::framebufferResizeCallback(GLFWwindow* window, int, int) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

int main() {
    try { HelloTriangleApplication().run(); }
    catch (const std::exception& e) { std::cerr << e.what() << std::endl; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
