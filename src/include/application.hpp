#pragma once
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>
#include <shaderc/shaderc.hpp>

#include <array>
#include <vector>

struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;
struct VmaAllocation_T;
typedef struct VmaAllocation_T* VmaAllocation;


struct FrameResources {
  VkCommandPool command_pool = nullptr;
  VkCommandBuffer command_buffer = nullptr;
  VkSemaphore image_required_semaphore = nullptr;
};

class Application {
private:
  constexpr static uint32_t VulkanVersion{VK_API_VERSION_1_4};
  constexpr static uint32_t MaxFramesInFlight{2};
  constexpr static VkFormat swapchainFormat{VK_FORMAT_B8G8R8A8_SRGB};
  constexpr static VkFormat depthFormat{VK_FORMAT_D32_SFLOAT};
  uint16_t width{1600}, height{900};

  SDL_Window *window = nullptr;

  // vulkan core
  VkInstance vulkan_instance = nullptr;
  VkPhysicalDevice physical_device = nullptr;
  VkDevice device = nullptr;
  VkSurfaceKHR surface = nullptr;
  VmaAllocator vma_allocator = nullptr;
  uint32_t gfxQueueFamIdx = UINT32_MAX;
  VkQueue gfxQueue = nullptr;

  // swapchain related
  VkSwapchainKHR swapchain = nullptr;
  std::vector<VkImage> swapchainImages;
  std::vector<VkImageView> swapchainImageViews;
  std::vector<VkSemaphore> renderCompleteSemaphores;
  bool requireSwapchainRecreate = false;
  uint32_t swapchainWidth = 0;
  uint32_t swapchainHeight = 0;

  VkImage depthImage = nullptr;
  VkImageView depthImageView = nullptr;
  VmaAllocation depthImageAllocation = nullptr;

  // graphics pipeline related
  VkPipelineLayout pipelineLayout = nullptr;
  VkPipeline pipeline = nullptr;

  // shader resources
  VkShaderModule vertShader = nullptr;
  VkShaderModule fragShader = nullptr;

  // frame and synchronization resources
  VkSemaphore timelineSemaphore = nullptr;
  std::array<FrameResources, MaxFramesInFlight> frameResources;

  void showError(const std::string &errorMessasge) const;
  void showError(const std::string &errorMessasge, VkResult error_code) const;

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);

  bool initializeVulkan();
  bool createVulkanInstance();
  bool createSurface();
  VkPhysicalDevice findPhysicalDevice();
  bool findGraphicsQueue();
  bool createDevice(VkPhysicalDevice physicalDevice);
  bool initializeVMA();
  bool createSwapchain(uint32_t width, uint32_t height);
  void destroySwapchain();
  VkShaderModule createShaderModule(const std::string &fileName,
                                    shaderc_shader_kind kind) const;
  bool createShaders();
  VkPipeline createGraphicsPipeline();
  bool createSyncResources();
  bool createCommandBuffers();
  void render();

public:
  bool initialize();
  void run();
  void close();
};