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

#if !defined(SAMPLE_COMMON_HEADER)
#define SAMPLE_COMMON_HEADER

#include <memory>
#include "GUI.h"

namespace ApiWithoutSecrets {

  // ************************************************************ //
  // VertexData                                                   //
  //                                                              //
  // Struct describing data type and format of vertex attributes  //
  // ************************************************************ //
  struct VertexData {
    struct PositionData {
      float x, y, z, w;
    } Position;
    struct TexcoordData {
      float   u, v;
    } Texcoords;
  };

  // ************************************************************ //
  // FrameResourcesData                                           //
  //                                                              //
  // Struct containing data used to generate a single frame       //
  // ************************************************************ //
  struct FrameResourcesData {
    ImageParameters                       DepthAttachment;
    vk::UniqueFramebuffer                 Framebuffer;
    vk::UniqueSemaphore                   ImageAvailableSemaphore;
    vk::UniqueSemaphore                   FinishedRenderingSemaphore;
    vk::UniqueFence                       Fence;

    FrameResourcesData() :
      DepthAttachment(),
      Framebuffer(),
      ImageAvailableSemaphore(),
      FinishedRenderingSemaphore(),
      Fence() {
    }

    virtual ~FrameResourcesData() {
    }
  };

  // ************************************************************ //
  // CurrentFrameData                                             //
  //                                                              //
  // Struct for managing frame rendering process                  //
  // ************************************************************ //
  struct CurrentFrameData {
    uint32_t                    ResourceIndex;
    uint32_t                    ResourceCount;
    FrameResourcesData         *FrameResources;
    const SwapChainParameters  *Swapchain;
    uint32_t                    SwapchainImageIndex;
  };

#define SAMPLE_FRAME_RESOURCES_PTR( index ) static_cast<SampleFrameResourcesData*>(FrameResources[index].get())
#define SAMPLE_CURRENT_FRAME_RESOURCES_PTR( current_frame ) static_cast<SampleFrameResourcesData*>(current_frame.FrameResources)

  // ************************************************************ //
  // RenderPassAttachmentData                                     //
  //                                                              //
  // Struct simplifying render pass creation                      //
  // ************************************************************ //
  struct RenderPassAttachmentData {
    vk::Format            Format;
    vk::AttachmentLoadOp  LoadOp;
    vk::AttachmentStoreOp StoreOp;
    vk::ImageLayout       InitialLayout;
    vk::ImageLayout       FinalLayout;
  };

  // ************************************************************ //
  // RenderPassSubpassData                                        //
  //                                                              //
  // Struct simplifying render pass creation                      //
  // ************************************************************ //
  struct RenderPassSubpassData {
    std::vector<vk::AttachmentReference> const  InputAttachments;
    std::vector<vk::AttachmentReference> const  ColorAttachments;
    vk::AttachmentReference const               DepthStencilAttachment;
  };

  // ************************************************************ //
  // SampleCommon                                                 //
  //                                                              //
  // Base class for all samples                                   //
  // ************************************************************ //
  class SampleCommon : public VulkanCommon {
  public:
    static const vk::Format   DefaultDepthFormat = vk::Format::eD16Unorm;

    SampleCommon( std::string const & title );
    virtual ~SampleCommon();

    std::string const & GetTitle() const;
    TimerData const   & GetTimer() const;

    void                Prepare( OS::WindowParameters window_parameters );

  protected:
    std::vector<std::unique_ptr<FrameResourcesData>>  FrameResources;

    virtual void        PrepareSample() = 0;                  // <- Override this for sample-specific resource initialization
    virtual void        PrepareGUIFrame() = 0;                // <- Required to add sample-specific GUI
  
    void                StartFrame( CurrentFrameData & current_frame );
    void                AcquireImage( CurrentFrameData & current_frame, vk::RenderPass & render_pass );
    void                ClearFramebuffer( CurrentFrameData &current_frame, vk::CommandBuffer & command_buffer, vk::RenderPass & render_pass );
    void                FinishFrame( CurrentFrameData & current_frame, vk::CommandBuffer & command_buffer, vk::RenderPass & render_pass );

    virtual void        OnWindowSizeChanged_Pre() override;
    virtual void        OnWindowSizeChanged_Post() override;

    virtual void        OnSampleWindowSizeChanged_Pre() = 0;
    virtual void        OnSampleWindowSizeChanged_Post() = 0;

  public:
    vk::UniqueShaderModule                CreateShaderModule( char const * filename ) const;
    ImageParameters                       CreateImage( uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, vk::MemoryPropertyFlagBits property, vk::ImageAspectFlags aspect ) const;
    BufferParameters                      CreateBuffer( uint32_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlagBits memoryProperty ) const;
    DescriptorSetParameters               CreateDescriptorResources( std::vector<vk::DescriptorSetLayoutBinding> const & layout_bindings, std::vector<vk::DescriptorPoolSize> const & pool_sizes ) const;
    vk::UniqueFramebuffer                 CreateFramebuffer( std::vector<vk::ImageView> const & image_views, vk::Extent2D const & extent, vk::RenderPass const & render_pass ) const;
    vk::UniqueSampler                     CreateSampler( vk::SamplerMipmapMode mipmap_mode, vk::SamplerAddressMode address_mode, vk::Bool32 unnormalized_coords ) const;
    vk::UniqueRenderPass                  CreateRenderPass( std::vector<RenderPassAttachmentData> const & attachment_descriptions, std::vector<RenderPassSubpassData> const & subpass_descriptions, std::vector<vk::SubpassDependency> const & dependencies ) const;
    vk::UniquePipelineLayout              CreatePipelineLayout( std::vector<vk::DescriptorSetLayout> const & descriptor_set_layouts, std::vector<vk::PushConstantRange> const & push_constant_ranges ) const;
    vk::UniqueSemaphore                   CreateSemaphore() const;
    vk::UniqueFence                       CreateFence( bool signaled ) const;
    vk::UniqueCommandPool                 CreateCommandPool( uint32_t queue_family_index, vk::CommandPoolCreateFlags flags ) const;
    std::vector<vk::UniqueCommandBuffer>  AllocateCommandBuffers( vk::CommandPool const & pool, vk::CommandBufferLevel level, uint32_t count ) const;
  
    void                UpdateDescriptorSet( vk::DescriptorSet & descriptor_set, vk::DescriptorType descriptor_type, uint32_t binding, uint32_t array_element, std::vector<vk::DescriptorImageInfo> const & image_infos = {}, std::vector<vk::DescriptorBufferInfo> const & buffer_infos = {}, std::vector<vk::BufferView> const & buffer_views = {} ) const;
    void                SetImageMemoryBarrier( vk::Image const & image, vk::ImageSubresourceRange const & image_subresource_range, vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages, vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages ) const;
    void                CopyDataToImage( uint32_t data_size, void const * data, vk::Image & target_image, uint32_t width, uint32_t height, vk::ImageSubresourceRange const & image_subresource_range, vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages, vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages ) const;
    void                CopyDataToBuffer( uint32_t data_size, void const * data, vk::Buffer target_buffer, vk::DeviceSize buffer_offset, vk::AccessFlags current_buffer_access, vk::PipelineStageFlags generating_stages, vk::AccessFlags new_buffer_access, vk::PipelineStageFlags consuming_stages ) const;
    void                PerformHardcoreCalculations( int duration ) const;
  
  private:
    std::string                                         Title;
    GUI                                                 Gui;
    TimerData                                           Timer;
  
    void                CreateImage( uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage, vk::UniqueImage & image ) const;
    void                AllocateImageMemory( vk::Image & image, vk::MemoryPropertyFlagBits property, vk::UniqueDeviceMemory & memory ) const;
    void                CreateImageView( vk::Image & image, vk::Format format, vk::ImageAspectFlags aspect, vk::UniqueImageView & image_view ) const;
    void                CreateBuffer( uint32_t size, vk::BufferUsageFlags usage, vk::UniqueBuffer & buffer ) const;
    void                AllocateBufferMemory( vk::Buffer & buffer, vk::MemoryPropertyFlagBits property, vk::UniqueDeviceMemory & memory ) const;
    void                CreateDescriptorSetLayout( std::vector<vk::DescriptorSetLayoutBinding> const & layout_bindings, vk::UniqueDescriptorSetLayout & set_layout ) const;
    void                CreateDescriptorPool( std::vector<vk::DescriptorPoolSize> const & pool_sizes, uint32_t max_sets, vk::UniqueDescriptorPool & descriptor_pool ) const;
    void                AllocateDescriptorSets( std::vector<vk::DescriptorSetLayout> const & descriptor_set_layout, vk::DescriptorPool & descriptor_pool, std::vector<vk::UniqueDescriptorSet> & descriptor_sets ) const;
  };

} // namespace ApiWithoutSecrets

#endif // SAMPLE_COMMON_HEADER