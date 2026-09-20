#include "application.hpp"
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <cstdint>
#include <print>
#include <vector>
#include <volk.h>

void Application::run() {}
bool Application::initialize() {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    window = SDL_CreateWindow("Hello Vulkan", width, height,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::println(stderr, "Validation Layer: {}", pCallbackData->pMessage);
  }
  return VK_FALSE;
}

void Application::showError(const std::string &msg) const {
  std::println(stderr, "Application error: {}", msg);
}

bool Application::createVulkanInstance() {
  if (volkInitialize() != VK_SUCCESS) {
    showError("Error initialzing Volk");
    return false;
  }
  VkApplicationInfo appInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "My first triangle",
      .apiVersion = VulkanVersion,
  };
  uint32_t instance_extension_count = 0;
  const auto *const *extensions =
      SDL_Vulkan_GetInstanceExtensions(&instance_extension_count);
  std::vector<const char *> requested_extensions{
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  for (int i = 0; i < instance_extension_count; ++i) {
    requested_extensions.push_back(extensions[i]);
  }

  std::vector<const char *> requested_layers{"VK_LAYER_KHRONOS_validation"};

  VkDebugUtilsMessengerCreateInfoEXT debug_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback};

  VkInstanceCreateInfo instance_create_info{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = &debug_info,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(requested_layers.size()),
      .ppEnabledLayerNames = requested_layers.data(),
      .enabledExtensionCount =
          static_cast<uint32_t>(requested_extensions.size()),
      .ppEnabledExtensionNames = requested_extensions.data()};

  if (vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance) !=
      VK_SUCCESS) {
    return false;
  }
  volkLoadInstance(vulkan_instance);
  return true;
}
bool Application::createSurface() {
  return SDL_Vulkan_CreateSurface(window, vulkan_instance, nullptr, &surface);
}
VkPhysicalDevice Application::findPhysicalDevice() {
  uint32_t physical_device_count = 0;
  vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
  vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count,
                             physical_devices.data());

  VkPhysicalDevice physical_device = nullptr;
  if (physical_device_count) {
    physical_device = physical_devices.at(0);
    for (auto &dev : physical_devices) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(dev, &props);
      if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        physical_device = dev;
        break;
      }
    }
  }
  return physical_device;
};
bool Application::findGraphicsQueue() {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties2(physical_device,
                                            &queue_family_count, nullptr);
  std::vector<VkQueueFamilyProperties2> queue_family_props(
      queue_family_count,
      {.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
  vkGetPhysicalDeviceQueueFamilyProperties2(
      physical_device, &queue_family_count, queue_family_props.data());

  for (int family_index = 0; family_index < queue_family_props.size();
       family_index++) {
    VkBool32 hasPresentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family_index, surface,
                                         &hasPresentSupport);
    const auto &props = queue_family_props.at(family_index);
    if (props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        hasPresentSupport) {
      gfxQueueFamIdx = family_index;
      return true;
    }
  }
  return false;
}
bool Application::createDevice(VkPhysicalDevice physical_device) {
  VkPhysicalDeviceVulkan14Features supported_features_14{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
      .pNext = nullptr,
  };
  VkPhysicalDeviceVulkan13Features supported_features_13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &supported_features_14,
  };
  VkPhysicalDeviceVulkan12Features supported_features_12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &supported_features_13,
  };
  VkPhysicalDeviceFeatures2 supported_features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &supported_features_12,
  };
  vkGetPhysicalDeviceFeatures2(physical_device, &supported_features);

  if (!supported_features_13.dynamicRendering ||
      !supported_features_13.synchronization2 ||
      !supported_features_12.timelineSemaphore) {
    showError("Physical device doesnt meet the requirements");
    return false;
  }

  VkPhysicalDeviceVulkan14Features features_14{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
      .pNext = nullptr};
  VkPhysicalDeviceVulkan13Features features_13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &supported_features_14,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };
  VkPhysicalDeviceVulkan12Features features_12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &supported_features_13,
      .timelineSemaphore = VK_TRUE,
  };
  VkPhysicalDeviceFeatures2 features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &supported_features_12};

      std::vector<float> queue_priority{1.0f};
      VkDeviceQueueCreateInfo gfxQueueInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = gfxQueueFamIdx,
        .queueCount = 1,
        .pQueuePriorities = queue_priority.data()
      };
      const std::vector<const char*> device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
      VkDeviceCreateInfo dev_create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &gfxQueueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = nullptr,
      };

      if (vkCreateDevice(physical_device,&dev_create_info,nullptr,&device) != VK_SUCCESS) {
        return false;
      }

      vkGetDeviceQueue(device,gfxQueueFamIdx,0,&gfxQueue);
      if (!gfxQueue) {
        showError("Couldn't get graphics queue");
        return false;
      }
      return true;

}
bool Application::initializeVulkan() {
  if (!createVulkanInstance()) {
    showError("Failure of Vulkan instanation");
    return false;
  }
  if (!createSurface()) {
    showError("Failure of Surface createtion");
    return false;
  }
  if (physical_device = findPhysicalDevice(); !physical_device) {
    showError("Unable to find physical device");
    return false;
  }
  if (!findGraphicsQueue()) {
    showError("Unable to find a compatible graphics queue");
    return false;
  }
  if (!createDevice(physical_device)) {
    showError("Unable to create logical device");
    return false;
  }
}

void Application::close() {

  if (vulkan_instance) {
    vkDestroyInstance(vulkan_instance, nullptr);
  }
  volkFinalize();

  if (window) {
    SDL_DestroyWindow(window);
  }
  SDL_Quit();
}