////////////////////////////////////////////////////////////////////////////////
// Copyright 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////

#include "VulkanCommon.h"
#include "VulkanFunctions.h"

namespace ApiWithoutSecrets {

  VulkanCommon::VulkanCommon() :
    Window(),
    Vulkan() {
  }

  void VulkanCommon::PrepareVulkan( OS::WindowParameters parameters, uint32_t version ) {
    Window = parameters;

    CheckVulkanLibrary();
    LoadExportedEntryPoints();
    LoadGlobalLevelEntryPoints( version );
    CreateInstance( version );
    CreatePresentationSurface();
    CreateDevice( version );
    GetDeviceQueue();
    CreateSwapChain();
  }

  void VulkanCommon::OnWindowSizeChanged() {
    if( !Vulkan.Device ) {
      return;
    }

    Vulkan.Device->waitIdle();

    OnWindowSizeChanged_Pre();
    CreateSwapChain();
    if( CanRender ) {
      OnWindowSizeChanged_Post();
    }
  }

  vk::PhysicalDevice const & VulkanCommon::GetPhysicalDevice() const {
    return Vulkan.PhysicalDevice;
  }

  const std::string & VulkanCommon::GetPhysicalDeviceName() const {
    return Vulkan.PhysicalDeviceName;
  }

  vk::Device const & VulkanCommon::GetDevice() const {
    return *Vulkan.Device;
  }

  const QueueParameters & VulkanCommon::GetGraphicsQueue() const {
    return Vulkan.GraphicsQueue;
  }

  const QueueParameters & VulkanCommon::GetPresentQueue() const {
    return Vulkan.PresentQueue;
  }

  vk::SurfaceKHR const & VulkanCommon::GetPresentationSurface() const {
    return *Vulkan.PresentationSurface;
  }

  const SwapChainParameters & VulkanCommon::GetSwapChain() const {
    return Vulkan.SwapChain;
  }

  void VulkanCommon::CheckVulkanLibrary() {
    if( VulkanLibrary == nullptr ) {
      throw std::exception( "Vulkan library could not be loaded!" );
    }
  }

  void VulkanCommon::LoadExportedEntryPoints() {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    #define LoadProcAddress GetProcAddress
#elif defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
    #define LoadProcAddress dlsym
#endif

#define VK_EXPORTED_FUNCTION( fun )                                               \
    if( !(fun = (PFN_##fun)LoadProcAddress( VulkanLibrary, #fun )) ) {            \
      throw std::exception( "Could not load exported function: " #fun "!" );      \
    }

#include "ListOfFunctions.inl"
  }

  void VulkanCommon::LoadGlobalLevelEntryPoints( uint32_t version ) {
#define VK_GLOBAL_LEVEL_FUNCTION( fun, ver )                                      \
    if( (ver <= version) &&                                                       \
        !(fun = (PFN_##fun)Vulkan.Instance->getProcAddr( #fun )) ) {              \
      throw std::exception( "Could not load global level function: " #fun "!" );  \
    }

#include "ListOfFunctions.inl"
  }

  void VulkanCommon::CreateInstance( uint32_t version ) {
    auto available_extensions = vk::enumerateInstanceExtensionProperties();

    std::vector<const char*> extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XCB_KHR)
      VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
      VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#endif
    };

    for( size_t i = 0; i < extensions.size(); ++i ) {
      if( !CheckExtensionAvailability( extensions[i], available_extensions ) ) {
        throw std::exception( std::string( "Could not find instance extension named \"" + std::string( extensions[i] ) + "\"!" ).c_str() );
      }
    }

    vk::ApplicationInfo application_info(
      "API without Secrets: Introduction to Vulkan",  // const char               * pApplicationName
      VK_MAKE_VERSION( 1, 0, 0 ),                     // uint32_t                   applicationVersion
      "Vulkan Tutorial by Intel",                     // const char               * pEngineName
      VK_MAKE_VERSION( 1, 0, 0 ),                     // uint32_t                   engineVersion
      version                                         // uint32_t                   apiVersion
    );

    vk::InstanceCreateInfo instance_create_info(
      vk::InstanceCreateFlags( 0 ),                   // VkInstanceCreateFlags      flags
      &application_info,                              // const VkApplicationInfo  * pApplicationInfo
      0,                                              // uint32_t                   enabledLayerCount
      nullptr,                                        // const char * const       * ppEnabledLayerNames
      static_cast<uint32_t>(extensions.size()),       // uint32_t                   enabledExtensionCount
      extensions.data()                               // const char * const       * ppEnabledExtensionNames
    );

    Vulkan.Instance = vk::createInstanceUnique( instance_create_info );
    LoadInstanceLevelEntryPoints( version, extensions );
  }

  void VulkanCommon::LoadInstanceLevelEntryPoints( uint32_t version, std::vector<const char*> const & enabled_extensions ) {
#define VK_INSTANCE_LEVEL_FUNCTION( fun, ver )                                          \
    if( (ver <= version) &&                                                             \
        !(fun = (PFN_##fun)Vulkan.Instance->getProcAddr( #fun )) ) {                    \
      throw std::exception( "Could not load instance level function: " #fun "!" );      \
    }

#define VK_INSTANCE_LEVEL_FUNCTION_FROM_EXTENSION( fun, ext )                           \
    for( auto & enabled_extension : enabled_extensions ) {                              \
      if( std::string( enabled_extension ) == std::string( ext ) ) {                    \
        fun = (PFN_##fun)Vulkan.Instance->getProcAddr( #fun );                          \
        if( fun == nullptr ) {                                                          \
          throw std::exception( "Could not load instance level function: " #fun "!" );  \
        }                                                                               \
      }                                                                                 \
    }

#include "ListOfFunctions.inl"
  }

  void VulkanCommon::CreatePresentationSurface() {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    vk::Win32SurfaceCreateInfoKHR surface_create_info(
      vk::Win32SurfaceCreateFlagsKHR(),               // VkWin32SurfaceCreateFlagsKHR     flags
      Window.Instance,                                // HINSTANCE                        hinstance
      Window.Handle                                   // HWND                             hwnd
    );

    Vulkan.PresentationSurface = Vulkan.Instance->createWin32SurfaceKHRUnique( surface_create_info );

#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,    // VkStructureType                  sType
      nullptr,                                          // const void                      *pNext
      0,                                                // VkXcbSurfaceCreateFlagsKHR       flags
      Window.Connection,                                // xcb_connection_t                *connection
      Window.Handle                                     // xcb_window_t                     window
    };

    if( vkCreateXcbSurfaceKHR( Vulkan.Instance, &surface_create_info, nullptr, &Vulkan.PresentationSurface ) == VK_SUCCESS ) {
      return true;
    }

#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkXlibSurfaceCreateInfoKHR surface_create_info = {
      VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,   // VkStructureType                sType
      nullptr,                                          // const void                    *pNext
      0,                                                // VkXlibSurfaceCreateFlagsKHR    flags
      Window.DisplayPtr,                                // Display                       *dpy
      Window.Handle                                     // Window                         window
    };
    if( vkCreateXlibSurfaceKHR( Vulkan.Instance, &surface_create_info, nullptr, &Vulkan.PresentationSurface ) == VK_SUCCESS ) {
      return true;
    }

#endif
  }

  void VulkanCommon::CreateDevice( uint32_t version ) {
    auto physical_devices = Vulkan.Instance->enumeratePhysicalDevices();

    uint32_t selected_graphics_queue_family_index = UINT32_MAX;
    uint32_t selected_present_queue_family_index = UINT32_MAX;

    for( auto & physical_device : physical_devices ) {
      if( CheckPhysicalDeviceProperties( physical_device, selected_graphics_queue_family_index, selected_present_queue_family_index ) ) {
        Vulkan.PhysicalDevice = physical_device;
        break;
      }
    }
    if( !Vulkan.PhysicalDevice ) {
      throw std::exception( "Could not select physical device based on the chosen properties!" );
    }

    std::vector<float> queue_priorities = { 1.0f };
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos = {
      vk::DeviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags( 0 ),                // VkDeviceQueueCreateFlags     flags
        selected_graphics_queue_family_index,           // uint32_t                     queueFamilyIndex
        static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
        queue_priorities.data()                         // const float                 *pQueuePriorities
      )
    };

    if( selected_graphics_queue_family_index != selected_present_queue_family_index ) {
      queue_create_infos.emplace_back(
        vk::DeviceQueueCreateFlags( 0 ),                // VkDeviceQueueCreateFlags     flags
        selected_present_queue_family_index,            // uint32_t                     queueFamilyIndex
        static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
        queue_priorities.data()                         // const float                 *pQueuePriorities
      );
    }

    std::vector<const char*> extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    vk::DeviceCreateInfo device_create_info(
      vk::DeviceCreateFlags( 0 ),                       // VkDeviceCreateFlags                flags
      static_cast<uint32_t>(queue_create_infos.size()), // uint32_t                           queueCreateInfoCount
      queue_create_infos.data(),                        // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
      0,                                                // uint32_t                           enabledLayerCount
      nullptr,                                          // const char * const                *ppEnabledLayerNames
      static_cast<uint32_t>(extensions.size()),         // uint32_t                           enabledExtensionCount
      extensions.data(),                                // const char * const                *ppEnabledExtensionNames
      nullptr                                           // const VkPhysicalDeviceFeatures    *pEnabledFeatures
    );

    Vulkan.Device = Vulkan.PhysicalDevice.createDeviceUnique( device_create_info );
    Vulkan.GraphicsQueue.FamilyIndex = selected_graphics_queue_family_index;
    Vulkan.PresentQueue.FamilyIndex = selected_present_queue_family_index;
    LoadDeviceLevelEntryPoints( version, extensions );
  }

  bool VulkanCommon::CheckPhysicalDeviceProperties( vk::PhysicalDevice const & physical_device, uint32_t & selected_graphics_queue_family_index, uint32_t & selected_present_queue_family_index ) {
    try {
      auto available_extensions = physical_device.enumerateDeviceExtensionProperties();

      vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();
      vk::PhysicalDeviceFeatures   device_features = physical_device.getFeatures();
      Vulkan.PhysicalDeviceName = device_properties.deviceName;

      std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
      };

      for( size_t i = 0; i < device_extensions.size(); ++i ) {
        if( !CheckExtensionAvailability( device_extensions[i], available_extensions ) ) {
          throw std::exception( std::string( "Physical device " + std::string( device_properties.deviceName ) + " doesn't support extension named \"" + device_extensions[i] + "\"!" ).c_str() );
        }
      }

      uint32_t major_version = VK_VERSION_MAJOR( device_properties.apiVersion );

      if( (major_version < 1) ||
        (device_properties.limits.maxImageDimension2D < 4096) ) {
        throw std::exception( std::string( "Physical device " + std::string( device_properties.deviceName ) + " doesn't support required parameters!" ).c_str() );
      }

      auto queue_family_properties = physical_device.getQueueFamilyProperties();
      std::vector<VkBool32> queue_present_support( queue_family_properties.size() );

      uint32_t graphics_queue_family_index = UINT32_MAX;
      uint32_t present_queue_family_index = UINT32_MAX;

      for( uint32_t i = 0; i < queue_family_properties.size(); ++i ) {
        queue_present_support[i] = physical_device.getSurfaceSupportKHR( i, *Vulkan.PresentationSurface );

        if( (queue_family_properties[i].queueCount > 0) &&
          (queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) ) {
          // Select first queue that supports graphics
          if( graphics_queue_family_index == UINT32_MAX ) {
            graphics_queue_family_index = i;
          }

          // If there is queue that supports both graphics and present - prefer it
          if( queue_present_support[i] ) {
            selected_graphics_queue_family_index = i;
            selected_present_queue_family_index = i;
            return true;
          }
        }
      }

      // We don't have queue that supports both graphics and present so we have to use separate queues
      for( uint32_t i = 0; i < queue_family_properties.size(); ++i ) {
        if( queue_present_support[i] ) {
          present_queue_family_index = i;
          break;
        }
      }

      // If this device doesn't support queues with graphics and present capabilities don't use it
      if( (graphics_queue_family_index == UINT32_MAX) ||
        (present_queue_family_index == UINT32_MAX) ) {
        throw std::exception( std::string( "Could not find queue families with required properties on physical device " + std::string( device_properties.deviceName ) + "!" ).c_str() );
      }

      selected_graphics_queue_family_index = graphics_queue_family_index;
      selected_present_queue_family_index = present_queue_family_index;
      return true;
    } catch( std::exception & exception ) {
      std::cout << exception.what() << std::endl;
      return false;
    }
  }

  void VulkanCommon::LoadDeviceLevelEntryPoints( uint32_t version, std::vector<const char*> const & enabled_extensions ) {
#define VK_DEVICE_LEVEL_FUNCTION( fun, ver )                                                \
    if( (ver <= version) &&                                                                 \
        !(fun = (PFN_##fun)Vulkan.Device->getProcAddr( #fun )) ) {                          \
      throw std::exception( "Could not load device level function: " #fun "!" );            \
    }

#define VK_DEVICE_LEVEL_FUNCTION_FROM_EXTENSION( fun, ext )                                 \
    for( auto & enabled_extension : enabled_extensions ) {                                  \
      if( std::string( enabled_extension ) == std::string( ext ) ) {                        \
        fun = (PFN_##fun)Vulkan.Device->getProcAddr( #fun );                                \
        if( fun == nullptr ) {                                                              \
          throw std::exception( "Could not load device level Vulkan function: " #fun "!" ); \
        }                                                                                   \
      }                                                                                     \
    }

#include "ListOfFunctions.inl"
  }

  void VulkanCommon::GetDeviceQueue() {
    Vulkan.GraphicsQueue.Handle = Vulkan.Device->getQueue( Vulkan.GraphicsQueue.FamilyIndex, 0 );
    Vulkan.PresentQueue.Handle = Vulkan.Device->getQueue( Vulkan.PresentQueue.FamilyIndex, 0 );
  }

  void VulkanCommon::CreateSwapChain( vk::PresentModeKHR const selected_present_mode, vk::ImageUsageFlags const selected_usage, uint32_t const selected_image_count ) {
    CanRender = false;

    if( !Vulkan.Device ) {
      return;
    }

    Vulkan.Device->waitIdle();
    Vulkan.SwapChain.Images.clear();
    Vulkan.SwapChain.ImageViews.clear();

    auto surface_capabilities = Vulkan.PhysicalDevice.getSurfaceCapabilitiesKHR( *Vulkan.PresentationSurface );
    auto surface_formats = Vulkan.PhysicalDevice.getSurfaceFormatsKHR( *Vulkan.PresentationSurface );
    auto present_modes = Vulkan.PhysicalDevice.getSurfacePresentModesKHR( *Vulkan.PresentationSurface );

    uint32_t                        desired_number_of_images = GetSwapChainNumImages( surface_capabilities, selected_image_count );
    vk::SurfaceFormatKHR            desired_format = GetSwapChainFormat( surface_formats );
    vk::Extent2D                    desired_extent = GetSwapChainExtent( surface_capabilities );
    vk::ImageUsageFlags             desired_usage = GetSwapChainUsageFlags( surface_capabilities, selected_usage );
    vk::SurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform( surface_capabilities );
    vk::PresentModeKHR              desired_present_mode = GetSwapChainPresentMode( present_modes, selected_present_mode);
    vk::UniqueSwapchainKHR          old_swap_chain = std::move( Vulkan.SwapChain.Handle );

    if( (desired_extent.width == 0) || (desired_extent.height == 0) ) {
      // Current surface size is (0, 0) so we can't create a swap chain and render anything (CanRender == false)
      // But we don't wont to kill the application as this situation may occur i.e. when window gets minimized
      return;
    }

    vk::SwapchainCreateInfoKHR swap_chain_create_info(
      vk::SwapchainCreateFlagsKHR(),                // VkSwapchainCreateFlagsKHR      flags
      *Vulkan.PresentationSurface,                  // VkSurfaceKHR                   surface
      desired_number_of_images,                     // uint32_t                       minImageCount
      desired_format.format,                        // VkFormat                       imageFormat
      desired_format.colorSpace,                    // VkColorSpaceKHR                imageColorSpace
      desired_extent,                               // VkExtent2D                     imageExtent
      1,                                            // uint32_t                       imageArrayLayers
      desired_usage,                                // VkImageUsageFlags              imageUsage
      vk::SharingMode::eExclusive,                  // VkSharingMode                  imageSharingMode
      0,                                            // uint32_t                       queueFamilyIndexCount
      nullptr,                                      // const uint32_t                *pQueueFamilyIndices
      desired_transform,                            // VkSurfaceTransformFlagBitsKHR  preTransform
      vk::CompositeAlphaFlagBitsKHR::eOpaque,       // VkCompositeAlphaFlagBitsKHR    compositeAlpha
      desired_present_mode,                         // VkPresentModeKHR               presentMode
      VK_TRUE,                                      // VkBool32                       clipped
      *old_swap_chain                               // VkSwapchainKHR                 oldSwapchain
    );

    Vulkan.SwapChain.Handle = Vulkan.Device->createSwapchainKHRUnique( swap_chain_create_info );
    Vulkan.SwapChain.Format = desired_format.format;
    Vulkan.SwapChain.Images = Vulkan.Device->getSwapchainImagesKHR( *Vulkan.SwapChain.Handle );
    Vulkan.SwapChain.Extent = desired_extent;
    Vulkan.SwapChain.PresentMode = desired_present_mode;
    Vulkan.SwapChain.UsageFlags = desired_usage;
    CreateSwapChainImageViews();
  }

  void VulkanCommon::CreateSwapChainImageViews() {
    for( size_t i = 0; i < Vulkan.SwapChain.Images.size(); ++i ) {
      vk::ImageViewCreateInfo image_view_create_info(
        vk::ImageViewCreateFlags(),                 // VkImageViewCreateFlags         flags
        Vulkan.SwapChain.Images[i],                 // VkImage                        image
        vk::ImageViewType::e2D,                     // VkImageViewType                viewType
        GetSwapChain().Format,                      // VkFormat                       format
        vk::ComponentMapping(),                     // VkComponentMapping             components
        vk::ImageSubresourceRange(                  // VkImageSubresourceRange        subresourceRange
          vk::ImageAspectFlagBits::eColor,            // VkImageAspectFlags             aspectMask
          0,                                          // uint32_t                       baseMipLevel
          1,                                          // uint32_t                       levelCount
          0,                                          // uint32_t                       baseArrayLayer
          1                                           // uint32_t                       layerCount
        )
      );

      Vulkan.SwapChain.ImageViews.emplace_back( Vulkan.Device->createImageViewUnique( image_view_create_info ) );
    }

    CanRender = true;
  }

  bool VulkanCommon::CheckExtensionAvailability( const char * extension_name, std::vector<vk::ExtensionProperties> const & available_extensions ) const {
    for( size_t i = 0; i < available_extensions.size(); ++i ) {
      if( strcmp( available_extensions[i].extensionName, extension_name ) == 0 ) {
        return true;
      }
    }
    return false;
  }

  uint32_t VulkanCommon::GetSwapChainNumImages( vk::SurfaceCapabilitiesKHR const & surface_capabilities, uint32_t selected_image_count ) const {
    // Set of images defined in a swap chain may not always be available for application to render to:
    // One may be displayed and one may wait in a queue to be presented
    // If application wants to use more images at the same time it must ask for more images
    uint32_t image_count = std::max( selected_image_count, surface_capabilities.minImageCount );
    if( surface_capabilities.maxImageCount > 0 ) {
      image_count = std::min( image_count, surface_capabilities.maxImageCount );
    }
    return image_count;
  }

  vk::SurfaceFormatKHR VulkanCommon::GetSwapChainFormat( std::vector<vk::SurfaceFormatKHR> const & surface_formats ) const {
    // If the list contains only one entry with undefined format
    // it means that there are no preferred surface formats and any can be chosen
    if( (surface_formats.size() == 1) &&
        (surface_formats[0].format == vk::Format::eUndefined) ) {
      return { vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }

    // Check if list contains most widely used R8 G8 B8 A8 format
    // with nonlinear color space
    for( auto & surface_format : surface_formats ) {
      if( surface_format.format == vk::Format::eR8G8B8A8Unorm ) {
        return surface_format;
      }
    }

    // Return the first format from the list
    return surface_formats[0];
  }

  vk::Extent2D VulkanCommon::GetSwapChainExtent( vk::SurfaceCapabilitiesKHR const & surface_capabilities ) const {
    // Special value of surface extent is width == height == -1
    // If this is so we define the size by ourselves but it must fit within defined confines
    if( surface_capabilities.currentExtent.width == -1 ) {
      vk::Extent2D swap_chain_extent = { 640, 480 };
      if( swap_chain_extent.width < surface_capabilities.minImageExtent.width ) {
        swap_chain_extent.width = surface_capabilities.minImageExtent.width;
      }
      if( swap_chain_extent.height < surface_capabilities.minImageExtent.height ) {
        swap_chain_extent.height = surface_capabilities.minImageExtent.height;
      }
      if( swap_chain_extent.width > surface_capabilities.maxImageExtent.width ) {
        swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
      }
      if( swap_chain_extent.height > surface_capabilities.maxImageExtent.height ) {
        swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
      }
      return swap_chain_extent;
    }

    // Most of the cases we define size of the swap_chain images equal to current window's size
    return surface_capabilities.currentExtent;
  }

  vk::ImageUsageFlags VulkanCommon::GetSwapChainUsageFlags( vk::SurfaceCapabilitiesKHR const & surface_capabilities, vk::ImageUsageFlags const selected_usage ) const {
    // Color attachment flag must always be supported
    // We can define other usage flags but we always need to check if they are supported
    if( !(surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eColorAttachment) ) {
      throw std::exception( std::string( std::string( "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT image usage is not supported by the swap chain!\n" ) +
        "Supported swap chain's image usages include:\n" +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc             ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n" : "") +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst             ? "    VK_IMAGE_USAGE_TRANSFER_DST\n" : "") +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eSampled                 ? "    VK_IMAGE_USAGE_SAMPLED\n" : "") +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eStorage                 ? "    VK_IMAGE_USAGE_STORAGE\n" : "") +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eColorAttachment         ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n" : "") +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eDepthStencilAttachment  ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" : "") +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransientAttachment     ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" : "") +
        (surface_capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eInputAttachment         ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT" : "") ).c_str()
      );
    }
    return vk::ImageUsageFlagBits::eColorAttachment;
  }

  vk::SurfaceTransformFlagBitsKHR VulkanCommon::GetSwapChainTransform( vk::SurfaceCapabilitiesKHR const & surface_capabilities ) const {
    // Sometimes images must be transformed before they are presented (i.e. due to device's orienation
    // being other than default orientation)
    // If the specified transform is other than current transform, presentation engine will transform image
    // during presentation operation; this operation may hit performance on some platforms
    // Here we don't want any transformations to occur so if the identity transform is supported use it
    // otherwise just use the same transform as current transform
    if( surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity ) {
      return vk::SurfaceTransformFlagBitsKHR::eIdentity;
    } else {
      return surface_capabilities.currentTransform;
    }
  }

  vk::PresentModeKHR VulkanCommon::GetSwapChainPresentMode( std::vector<vk::PresentModeKHR> const & available_present_modes, vk::PresentModeKHR const selected_present_mode ) const {
#define GET_PRESENT_MODE( selected_mode )                   \
    for( auto present_mode : available_present_modes ) {    \
      if( present_mode == selected_mode ) {                 \
        return present_mode;                                \
      }                                                     \
    }

    // Check if selected present mode is available
    GET_PRESENT_MODE( selected_present_mode )

    // Fallback to one of the typical present modes

    // IMMEDIATE mode allows us to display frames in a V-Sync independent manner so it can introduce screen tearing
    // But this mode is the best for performance measurements if we want to check the real number of FPS
    GET_PRESENT_MODE( vk::PresentModeKHR::eImmediate )

    // MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
    // If there are enough swapchain images, it allows us to generate frames with the highest performance (as with the IMMEDIATE mode)
    // but only the most recent one is presented on screen on blanking intervals
    GET_PRESENT_MODE( vk::PresentModeKHR::eMailbox );

    // FIFO present mode is always available
    GET_PRESENT_MODE( vk::PresentModeKHR::eFifo )

    throw std::exception( "FIFO present mode is not supported by the swap chain!" );
  }

  VulkanCommon::~VulkanCommon() {
    if( Vulkan.Device ) {
      Vulkan.Device->waitIdle();
    }
  }

} // namespace ApiWithoutSecrets