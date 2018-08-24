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

#if !defined(VULKAN_COMMON_HEADER)
#define VULKAN_COMMON_HEADER

#include <vector>
#include "vulkan.hpp"
#include "OperatingSystem.h"

namespace ApiWithoutSecrets {

  // ************************************************************ //
  // QueueParameters                                              //
  //                                                              //
  // Vulkan Queue's parameters container class                    //
  // ************************************************************ //
  struct QueueParameters {
    vk::Queue                     Handle;
    uint32_t                      FamilyIndex;

    QueueParameters() :
      Handle(),
      FamilyIndex( 0 ) {
    }
  };

  // ************************************************************ //
  // ImageParameters                                              //
  //                                                              //
  // Vulkan Image's parameters container class                    //
  // ************************************************************ //
  struct ImageParameters {
    vk::UniqueImage               Handle;
    vk::UniqueImageView           View;
    vk::UniqueSampler             Sampler;
    vk::UniqueDeviceMemory        Memory;

    ImageParameters() :
      Handle(),
      View(),
      Sampler(),
      Memory() {
    }
  };

  // ************************************************************ //
  // BufferParameters                                             //
  //                                                              //
  // Vulkan Buffer's parameters container class                   //
  // ************************************************************ //
  struct BufferParameters {
    vk::UniqueBuffer                Handle;
    vk::UniqueDeviceMemory          Memory;
    uint32_t                        Size;

    BufferParameters() :
      Handle(),
      Memory(),
      Size( 0 ) {
    }
  };

  // ************************************************************ //
  // StagingBufferParameters                                      //
  //                                                              //
  // Struct for handling staging buffer's memory mapping          //
  // ************************************************************ //
  struct StagingBufferParameters {
    BufferParameters          Buffer;
    void                     *Pointer;

    StagingBufferParameters() :
      Buffer(),
      Pointer( nullptr ) {
    }
  };

  // ************************************************************ //
  // DescriptorParameters                                         //
  //                                                              //
  // Container class for descriptor related resources             //
  // ************************************************************ //
  struct DescriptorSetParameters {
    vk::UniqueDescriptorPool        Pool;
    vk::UniqueDescriptorSetLayout   Layout;
    vk::UniqueDescriptorSet         Handle;

    DescriptorSetParameters() :
      Pool(),
      Layout(),
      Handle() {
    }
  };

  // ************************************************************ //
  // SwapChainParameters                                          //
  //                                                              //
  // Vulkan SwapChain's parameters container class                //
  // ************************************************************ //
  struct SwapChainParameters {
    vk::UniqueSwapchainKHR            Handle;
    vk::Format                        Format;
    std::vector<vk::Image>            Images;
    std::vector<vk::UniqueImageView>  ImageViews;
    vk::Extent2D                      Extent;
    vk::PresentModeKHR                PresentMode;
    vk::ImageUsageFlags               UsageFlags;

    SwapChainParameters() :
      Handle(),
      Format( vk::Format::eUndefined ),
      Images(),
      Extent(),
      PresentMode(),
      UsageFlags() {
    }
  };

  // ************************************************************ //
  // VulkanCommonParameters                                       //
  //                                                              //
  // General Vulkan parameters' container class                   //
  // ************************************************************ //
  struct VulkanCommonParameters {
    vk::UniqueInstance            Instance;
    vk::PhysicalDevice            PhysicalDevice;
    std::string                   PhysicalDeviceName;
    vk::UniqueDevice              Device;
    QueueParameters               GraphicsQueue;
    QueueParameters               PresentQueue;
    vk::UniqueSurfaceKHR          PresentationSurface;
    SwapChainParameters           SwapChain;

    VulkanCommonParameters() :
      Instance(),
      PhysicalDevice(),
      Device(),
      GraphicsQueue(),
      PresentQueue(),
      PresentationSurface(),
      SwapChain() {
    }
  };

  // ************************************************************ //
  // VulkanCommon                                                 //
  //                                                              //
  // Base class for Vulkan more advanced tutorial classes         //
  // ************************************************************ //
  class VulkanCommon : public OS::ProjectBase {
  public:
    VulkanCommon();
    virtual ~VulkanCommon();

    void                          PrepareVulkan( OS::WindowParameters parameters, uint32_t version = VK_MAKE_VERSION( 1, 0, 0 ) );
    virtual void                  OnWindowSizeChanged() final override;

    vk::PhysicalDevice const    & GetPhysicalDevice() const;
    std::string const           & GetPhysicalDeviceName() const;
    vk::Device const            & GetDevice() const;

    QueueParameters const       & GetGraphicsQueue() const;
    QueueParameters const       & GetPresentQueue() const;

    vk::SurfaceKHR const        & GetPresentationSurface() const;

    SwapChainParameters const   & GetSwapChain() const;

  protected:
    void                          CreateSwapChain( vk::PresentModeKHR const selected_present_mode = vk::PresentModeKHR::eMailbox, vk::ImageUsageFlags const selected_usage = vk::ImageUsageFlagBits::eColorAttachment, uint32_t const selected_image_count = 3 );

  private:
    OS::WindowParameters    Window;
    VulkanCommonParameters  Vulkan;

    void                            CheckVulkanLibrary();
    void                            LoadExportedEntryPoints();
    void                            LoadGlobalLevelEntryPoints( uint32_t version );
    void                            CreateInstance( uint32_t version );
    void                            LoadInstanceLevelEntryPoints( uint32_t version, std::vector<const char*> const & enabled_extensions );
    void                            CreatePresentationSurface();
    void                            CreateDevice( uint32_t version );
    bool                            CheckPhysicalDeviceProperties( vk::PhysicalDevice const & physical_device, uint32_t & graphics_queue_family_index, uint32_t & present_queue_family_index );
    void                            LoadDeviceLevelEntryPoints( uint32_t version, std::vector<const char*> const & enabled_extensions );
    void                            GetDeviceQueue();
    void                            CreateSwapChainImageViews();
    virtual void                    OnWindowSizeChanged_Pre() = 0;
    virtual void                    OnWindowSizeChanged_Post() = 0;

    bool                            CheckExtensionAvailability( const char * extension_name, const std::vector<vk::ExtensionProperties> &available_extensions ) const;
    uint32_t                        GetSwapChainNumImages( vk::SurfaceCapabilitiesKHR const & surface_capabilities, uint32_t selected_image_count ) const;
    vk::SurfaceFormatKHR            GetSwapChainFormat( std::vector<vk::SurfaceFormatKHR> const & surface_formats ) const;
    vk::Extent2D                    GetSwapChainExtent( vk::SurfaceCapabilitiesKHR const & surface_capabilities ) const;
    vk::ImageUsageFlags             GetSwapChainUsageFlags( vk::SurfaceCapabilitiesKHR const & surface_capabilities, vk::ImageUsageFlags const selected_usage ) const;
    vk::SurfaceTransformFlagBitsKHR GetSwapChainTransform( vk::SurfaceCapabilitiesKHR const & surface_capabilities ) const;
    vk::PresentModeKHR              GetSwapChainPresentMode( std::vector<vk::PresentModeKHR> const & available_present_modes, vk::PresentModeKHR const selected_present_mode ) const;
  };

} // namespace ApiWithoutSecrets

#endif // VULKAN_COMMON_HEADER