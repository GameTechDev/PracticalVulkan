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

#include <chrono>
#include <algorithm>
#include "Sample.h"
#include "VulkanFunctions.h"
#include "imgui/imgui.h"

namespace ApiWithoutSecrets {

  Sample::Sample( std::string const & title ) :
    SampleCommon( title ) {
  }

  void Sample::PrepareSample() {
    FrameResources.resize( 5 );

    CreateFrameResources();
    CreateRenderPasses();
    CreateDescriptorSet();
    CreateTextures();
    CreatePipelineLayout();
    CreateGraphicsPipeline();
    CreateVertexBuffers();
  }

  void Sample::PrepareGUIFrame() {
    ImGui::Begin( GetTitle().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize );
    ImGui::Text( std::string( "Hardware: " + GetPhysicalDeviceName() ).c_str() );
    ImGui::NewLine();

    ImGui::SliderIntWithStep( "Scene complexity", &Parameters.ObjectsCount, 10, Parameters.MaxObjectsCount, 50 );

    ImGui::SliderInt( "Frame resources count", &Parameters.FrameResourcesCount, 1, static_cast<int>(FrameResources.size()) );

    ImGui::SliderInt( "Pre-submit CPU work time [ms]", &Parameters.PreSubmitCpuWorkTime, 0, 20 );

    ImGui::SliderInt( "Post-submit CPU work time [ms]", &Parameters.PostSubmitCpuWorkTime, 0, 20 );

    ImGui::Text( "Frame generation time: %5.2f ms", Parameters.FrameGenerationTime );

    ImGui::Text( "Total frame time: %5.2f ms", Parameters.TotalFrameTime );

    ImGui::End();
  }

  void Sample::Draw() {
    static CurrentFrameData current_frame = {
      0,                                                        // uint32_t                     ResourceIndex
      static_cast<uint32_t>(FrameResources.size()),             // uint32_t                     ResourceCount
      FrameResources[0].get(),                                  // FrameResourcesData          *FrameResources
      &GetSwapChain(),                                          // const SwapChainParameters   *Swapchain
      0                                                         // uint32_t                     SwapchainImageIndex
    };

    current_frame.ResourceCount = Parameters.FrameResourcesCount;

    auto frame_begin_time = std::chrono::high_resolution_clock::now();

    // Star frame - calculate times and prepare GUI
    SampleCommon::StartFrame( current_frame );

    // Acquire swapchain image and create a framebuffer
    SampleCommon::AcquireImage( current_frame, *Parameters.RenderPass );

    // Draw scene/prepare scene's command buffers
    {
      auto frame_generation_begin_time = std::chrono::high_resolution_clock::now();

      // Perform calculation influencing current frame
      SampleCommon::PerformHardcoreCalculations( Parameters.PreSubmitCpuWorkTime);

      // Draw sample-specific data - includes command buffer submission!!
      DrawSample( current_frame );

      // Perform calculations influencing rendering of a next frame
      SampleCommon::PerformHardcoreCalculations( Parameters.PostSubmitCpuWorkTime);

      auto frame_generation_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_generation_begin_time);
      float float_frame_generation_time = static_cast<float>(frame_generation_time.count() * 0.001f);
      Parameters.FrameGenerationTime = Parameters.FrameGenerationTime * 0.99f + float_frame_generation_time * 0.01f;
    }

    // Draw GUI and present swapchain image
    SampleCommon::FinishFrame( current_frame, *SAMPLE_CURRENT_FRAME_RESOURCES_PTR( current_frame )->PostCommandBuffer, *Parameters.PostRenderPass );

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);
    float float_frame_time = static_cast<float>(total_frame_time.count() * 0.001f);
    Parameters.TotalFrameTime = Parameters.TotalFrameTime * 0.99f + float_frame_time * 0.01f;
  }

  void Sample::DrawSample( CurrentFrameData & current_frame ) {
    auto frame_resources = SAMPLE_CURRENT_FRAME_RESOURCES_PTR( current_frame );

    std::vector<vk::ClearValue> clear_values = {
      vk::ClearColorValue( std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } ),  // VkClearColorValue                      color
      vk::ClearDepthStencilValue( 1.0f )                                      // VkClearDepthStencilValue               depthStencil
    };

    vk::RenderPassBeginInfo render_pass_begin_info(
      *Parameters.RenderPass,                           // VkRenderPass                             renderPass
      *current_frame.FrameResources->Framebuffer,       // VkFramebuffer                            framebuffer
      {                                                 // VkRect2D                                 renderArea
        vk::Offset2D(),                                   // VkOffset2D                               offset
        GetSwapChain().Extent,                            // VkExtent2D                               extent
      },
      static_cast<uint32_t>(clear_values.size()),       // uint32_t                                 clearValueCount
      clear_values.data()                               // const VkClearValue                     * pClearValues
    );

    auto swapchain_extent = GetSwapChain().Extent;

    vk::Viewport viewport(
      0,                                                // float                                    x
      0,                                                // float                                    y
      static_cast<float>(swapchain_extent.width),       // float                                    width
      static_cast<float>(swapchain_extent.height),      // float                                    height
      0.0f,                                             // float                                    minDepth
      1.0f                                              // float                                    maxDepth
    );

    vk::Rect2D scissor(
      vk::Offset2D(),                                   // VkOffset2D                               offset
      GetSwapChain().Extent                             // VkExtent2D                               extent
    );

    float scaling_factor = static_cast<float>(swapchain_extent.width) / static_cast<float>(swapchain_extent.height);
    vk::CommandBuffer & command_buffer = *SAMPLE_CURRENT_FRAME_RESOURCES_PTR( current_frame )->PreCommandBuffer;

    command_buffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
    command_buffer.beginRenderPass( render_pass_begin_info, vk::SubpassContents::eInline );
    command_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *Parameters.GraphicsPipeline );
    command_buffer.setViewport( 0, { viewport } );
    command_buffer.setScissor( 0, { scissor } );
    command_buffer.bindVertexBuffers( 0, { *Parameters.VertexBuffer.Handle, *Parameters.InstanceBuffer.Handle }, { 0, 0 } );
    command_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, *Parameters.PipelineLayout, 0, { *Parameters.DescriptorSet.Handle }, {} );
    command_buffer.pushConstants( *Parameters.PipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof( float ), &scaling_factor );
    command_buffer.draw( 6 * Parameters.QuadTessellation * Parameters.QuadTessellation, Parameters.ObjectsCount, 0, 0 );
    command_buffer.endRenderPass();
    command_buffer.end();

    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info(
      1,                                                          // uint32_t                       waitSemaphoreCount
      &(*current_frame.FrameResources->ImageAvailableSemaphore),  // const VkSemaphore             *pWaitSemaphores
      &wait_dst_stage_mask,                                       // const VkPipelineStageFlags    *pWaitDstStageMask
      1,                                                          // uint32_t                       commandBufferCount;
      &command_buffer                                             // const VkCommandBuffer         *pCommandBuffers
    );
    GetGraphicsQueue().Handle.submit( { submit_info }, vk::Fence() );
  }

  void Sample::OnSampleWindowSizeChanged_Pre() {
  }

  void Sample::OnSampleWindowSizeChanged_Post() {
    // Create depth attachmnet and transition it away from an undefined layout
    {
      vk::ImageSubresourceRange image_subresource_range(
        vk::ImageAspectFlagBits::eDepth,                                // VkImageAspectFlags               aspectMask
        0,                                                              // uint32_t                         baseMipLevel
        1,                                                              // uint32_t                         levelCount
        0,                                                              // uint32_t                         baseArrayLayer
        1                                                               // uint32_t                         layerCount
      );
      for( size_t i = 0; i < FrameResources.size(); ++i ) {
        SAMPLE_FRAME_RESOURCES_PTR( i )->DepthAttachment = SampleCommon::CreateImage( GetSwapChain().Extent.width, GetSwapChain().Extent.height, DefaultDepthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth );
        SampleCommon::SetImageMemoryBarrier( *SAMPLE_FRAME_RESOURCES_PTR( i )->DepthAttachment.Handle, image_subresource_range, vk::ImageLayout::eUndefined, vk::AccessFlags( 0 ), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests );
      }
    }
    // Pre-transition all swapchain images away from an undefined layout
    {
      vk::ImageSubresourceRange image_subresource_range(
        vk::ImageAspectFlagBits::eColor,                                // VkImageAspectFlags               aspectMask
        0,                                                              // uint32_t                         baseMipLevel
        1,                                                              // uint32_t                         levelCount
        0,                                                              // uint32_t                         baseArrayLayer
        1                                                               // uint32_t                         layerCount
      );
      for( auto & swapchain_image : GetSwapChain().Images ) {
        SampleCommon::SetImageMemoryBarrier( swapchain_image, image_subresource_range, vk::ImageLayout::eUndefined, vk::AccessFlags( 0 ), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eMemoryRead, vk::PipelineStageFlagBits::eBottomOfPipe );
      }
    }
  }
 
  void Sample::CreateFrameResources() {
    for( size_t i = 0; i < FrameResources.size(); ++i ) {
      FrameResources[i] = std::make_unique<SampleFrameResourcesData>();
      auto frame_resources = SAMPLE_FRAME_RESOURCES_PTR( i );

      frame_resources->ImageAvailableSemaphore = SampleCommon::CreateSemaphore();
      frame_resources->FinishedRenderingSemaphore = SampleCommon::CreateSemaphore();
      frame_resources->Fence = SampleCommon::CreateFence( true );
      frame_resources->CommandPool = SampleCommon::CreateCommandPool( GetGraphicsQueue().FamilyIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient );
      frame_resources->PreCommandBuffer = std::move( SampleCommon::AllocateCommandBuffers( *frame_resources->CommandPool, vk::CommandBufferLevel::ePrimary, 1 )[0] );
      frame_resources->PostCommandBuffer = std::move( SampleCommon::AllocateCommandBuffers( *frame_resources->CommandPool, vk::CommandBufferLevel::ePrimary, 1 )[0] );
      
    }

    OnSampleWindowSizeChanged_Post();
  }

  void Sample::CreateRenderPasses() {
    std::vector<RenderPassSubpassData> subpass_descriptions = {
      {
        {},                                                         // std::vector<VkAttachmentReference> const  &InputAttachments
        {                                                           // std::vector<VkAttachmentReference> const  &ColorAttachments
          {
            0,                                                        // uint32_t                                   attachment
            vk::ImageLayout::eColorAttachmentOptimal                  // VkImageLayout                              layout
          }
        },
        {                                                           // VkAttachmentReference const               &DepthStencilAttachment;
          1,                                                          // uint32_t                                   attachment
          vk::ImageLayout::eDepthStencilAttachmentOptimal             // VkImageLayout                              layout
        }
      }
    };

    std::vector<vk::SubpassDependency> dependencies = {
      {
        VK_SUBPASS_EXTERNAL,                                        // uint32_t                       srcSubpass
        0,                                                          // uint32_t                       dstSubpass
        vk::PipelineStageFlagBits::eColorAttachmentOutput,          // VkPipelineStageFlags           srcStageMask
        vk::PipelineStageFlagBits::eColorAttachmentOutput,          // VkPipelineStageFlags           dstStageMask
        vk::AccessFlagBits::eColorAttachmentWrite,                  // VkAccessFlags                  srcAccessMask
        vk::AccessFlagBits::eColorAttachmentWrite,                  // VkAccessFlags                  dstAccessMask
        vk::DependencyFlagBits::eByRegion                           // VkDependencyFlags              dependencyFlags
      },
      {
        0,                                                          // uint32_t                       srcSubpass
        VK_SUBPASS_EXTERNAL,                                        // uint32_t                       dstSubpass
        vk::PipelineStageFlagBits::eColorAttachmentOutput,          // VkPipelineStageFlags           srcStageMask
        vk::PipelineStageFlagBits::eColorAttachmentOutput,          // VkPipelineStageFlags           dstStageMask
        vk::AccessFlagBits::eColorAttachmentWrite,                  // VkAccessFlags                  srcAccessMask
        vk::AccessFlagBits::eColorAttachmentWrite,                  // VkAccessFlags                  dstAccessMask
        vk::DependencyFlagBits::eByRegion                           // VkDependencyFlags              dependencyFlags
      }
    };
    // Render pass - from present_src to color_attachment
    {
      std::vector<RenderPassAttachmentData> attachment_descriptions = {
        {
          GetSwapChain().Format,                                    // VkFormat                       format
          vk::AttachmentLoadOp::eClear,                             // VkAttachmentLoadOp             loadOp
          vk::AttachmentStoreOp::eStore,                            // VkAttachmentStoreOp            storeOp
          vk::ImageLayout::ePresentSrcKHR,                          // VkImageLayout                  initialLayout
          vk::ImageLayout::eColorAttachmentOptimal                  // VkImageLayout                  finalLayout
        },
        {
          SampleCommon::DefaultDepthFormat,                         // VkFormat                       format
          vk::AttachmentLoadOp::eClear,                             // VkAttachmentLoadOp             loadOp
          vk::AttachmentStoreOp::eStore,                            // VkAttachmentStoreOp            storeOp
          vk::ImageLayout::eDepthStencilAttachmentOptimal,          // VkImageLayout                  initialLayout
          vk::ImageLayout::eDepthStencilAttachmentOptimal           // VkImageLayout                  finalLayout
        }
      };
      Parameters.RenderPass = SampleCommon::CreateRenderPass( attachment_descriptions, subpass_descriptions, dependencies );
    }
    // Post-render pass - from color_attachment to present_src
    {
      std::vector<RenderPassAttachmentData> attachment_descriptions = {
        {
          GetSwapChain().Format,                                    // VkFormat                       format
          vk::AttachmentLoadOp::eLoad,                              // VkAttachmentLoadOp             loadOp
          vk::AttachmentStoreOp::eStore,                            // VkAttachmentStoreOp            storeOp
          vk::ImageLayout::eColorAttachmentOptimal,                 // VkImageLayout                  initialLayout
          vk::ImageLayout::ePresentSrcKHR                           // VkImageLayout                  finalLayout
        },
        {
          SampleCommon::DefaultDepthFormat,                         // VkFormat                       format
          vk::AttachmentLoadOp::eDontCare,                          // VkAttachmentLoadOp             loadOp
          vk::AttachmentStoreOp::eDontCare,                         // VkAttachmentStoreOp            storeOp
          vk::ImageLayout::eDepthStencilAttachmentOptimal,          // VkImageLayout                  initialLayout
          vk::ImageLayout::eDepthStencilAttachmentOptimal           // VkImageLayout                  finalLayout
        }
      };
      Parameters.PostRenderPass = SampleCommon::CreateRenderPass( attachment_descriptions, subpass_descriptions, dependencies );
    }
  }

  void Sample::CreateDescriptorSet() {
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
      {
        0,                                                          // uint32_t                       binding
        vk::DescriptorType::eCombinedImageSampler,                  // VkDescriptorType               descriptorType
        1,                                                          // uint32_t                       descriptorCount
        vk::ShaderStageFlagBits::eFragment,                         // VkShaderStageFlags             stageFlags
        nullptr                                                     // const VkSampler               *pImmutableSamplers
      },
      {
        1,                                                          // uint32_t                       binding
        vk::DescriptorType::eCombinedImageSampler,                  // VkDescriptorType               descriptorType
        1,                                                          // uint32_t                       descriptorCount
        vk::ShaderStageFlagBits::eFragment,                         // VkShaderStageFlags             stageFlags
        nullptr                                                     // const VkSampler               *pImmutableSamplers
      }
    };
    std::vector<vk::DescriptorPoolSize> pool_sizes = {
      {
        vk::DescriptorType::eCombinedImageSampler,                  // VkDescriptorType               type
        2                                                           // uint32_t                       descriptorCount
      }
    };
    Parameters.DescriptorSet = SampleCommon::CreateDescriptorResources( layout_bindings, pool_sizes );
  }

  void Sample::CreateTextures() {
    // Background texture
    {
      int width = 0, height = 0, data_size = 0;
      std::vector<char> texture_data = Tools::GetImageData( "../Data/Common/Background.png", 4, &width, &height, nullptr, &data_size );
      // Create descriptor resources
      {
        Parameters.BackgroundTexture = SampleCommon::CreateImage( width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor );
        Parameters.BackgroundTexture.Sampler = SampleCommon::CreateSampler( vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, VK_FALSE );
      }
      // Copy data
      {
        vk::ImageSubresourceRange image_subresource_range(
          vk::ImageAspectFlagBits::eColor,                              // VkImageAspectFlags             aspectMask
          0,                                                            // uint32_t                       baseMipLevel
          1,                                                            // uint32_t                       levelCount
          0,                                                            // uint32_t                       baseArrayLayer
          1                                                             // uint32_t                       layerCount
        );
        SampleCommon::CopyDataToImage( data_size, texture_data.data(), *Parameters.BackgroundTexture.Handle, width, height, image_subresource_range, vk::ImageLayout::eUndefined, vk::AccessFlags( 0 ), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader );
      }
      // Update descriptor set
      {
        std::vector<vk::DescriptorImageInfo> image_infos = {
          {
            *Parameters.BackgroundTexture.Sampler,                      // VkSampler                      sampler
            *Parameters.BackgroundTexture.View,                         // VkImageView                    imageView
            vk::ImageLayout::eShaderReadOnlyOptimal                     // VkImageLayout                  imageLayout
          }
        };
        SampleCommon::UpdateDescriptorSet( *Parameters.DescriptorSet.Handle, vk::DescriptorType::eCombinedImageSampler, 0, 0, image_infos );
      }
    }
    // Sample texture
    {
      int width = 0, height = 0, data_size = 0;
      std::vector<char> texture_data = Tools::GetImageData( "../Data/" PROJECT_NUMBER_STRING "/FrameResources.png", 4, &width, &height, nullptr, &data_size );
      // Create descriptor resources
      {
        Parameters.Texture = SampleCommon::CreateImage( width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor );
        Parameters.Texture.Sampler = SampleCommon::CreateSampler( vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, VK_FALSE );
      }
      // Copy data
      {
        vk::ImageSubresourceRange image_subresource_range(
          vk::ImageAspectFlagBits::eColor,                              // VkImageAspectFlags             aspectMask
          0,                                                            // uint32_t                       baseMipLevel
          1,                                                            // uint32_t                       levelCount
          0,                                                            // uint32_t                       baseArrayLayer
          1                                                             // uint32_t                       layerCount
        );
        SampleCommon::CopyDataToImage( data_size, texture_data.data(), *Parameters.Texture.Handle, width, height, image_subresource_range, vk::ImageLayout::eUndefined, vk::AccessFlags( 0 ), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader );
      }
      // Update descriptor set
      {
        std::vector<vk::DescriptorImageInfo> image_infos = {
          {
            *Parameters.Texture.Sampler,                                  // VkSampler                      sampler
            *Parameters.Texture.View,                                     // VkImageView                    imageView
            vk::ImageLayout::eShaderReadOnlyOptimal                       // VkImageLayout                  imageLayout
          }
        };
        SampleCommon::UpdateDescriptorSet( *Parameters.DescriptorSet.Handle, vk::DescriptorType::eCombinedImageSampler, 1, 0, image_infos );
      }
    }
  }

  void Sample::CreatePipelineLayout() {
    vk::PushConstantRange push_constant_ranges(
      vk::ShaderStageFlagBits::eVertex,                                 // VkShaderStageFlags             stageFlags
      0,                                                                // uint32_t                       offset
      4                                                                 // uint32_t                       size
    );
    Parameters.PipelineLayout = SampleCommon::CreatePipelineLayout( { *Parameters.DescriptorSet.Layout }, { push_constant_ranges } );
  }

  void Sample::CreateGraphicsPipeline() {
    vk::UniqueShaderModule vertex_shader_module = SampleCommon::CreateShaderModule( "../Data/" PROJECT_NUMBER_STRING "/shader.vert.spv" );
    vk::UniqueShaderModule fragment_shader_module = SampleCommon::CreateShaderModule( "../Data/" PROJECT_NUMBER_STRING "/shader.frag.spv" );

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos = {
      // Vertex shader
      {
        vk::PipelineShaderStageCreateFlags( 0 ),                    // VkPipelineShaderStageCreateFlags               flags
        vk::ShaderStageFlagBits::eVertex,                           // VkShaderStageFlagBits                          stage
        *vertex_shader_module,                                      // VkShaderModule                                 module
        "main"                                                      // const char                                    *pName
      },
      // Fragment shader
      {
        vk::PipelineShaderStageCreateFlags( 0 ),                    // VkPipelineShaderStageCreateFlags               flags
        vk::ShaderStageFlagBits::eFragment,                         // VkShaderStageFlagBits                          stage
        *fragment_shader_module,                                    // VkShaderModule                                 module
        "main"                                                      // const char                                    *pName
      }
    };

    std::vector<vk::VertexInputBindingDescription> vertex_binding_description = {
      {
        0,                                                          // uint32_t                                       binding
        sizeof( VertexData ),                                       // uint32_t                                       stride
        vk::VertexInputRate::eVertex                                // VkVertexInputRate                              inputRate
      },
      {
        1,                                                          // uint32_t                                       binding
        4 * sizeof( float ),                                        // uint32_t                                       stride
        vk::VertexInputRate::eInstance                              // VkVertexInputRate                              inputRate
      }
    };

    std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions = {
      {
        0,                                                          // uint32_t                                       location
        vertex_binding_description[0].binding,                      // uint32_t                                       binding
        vk::Format::eR32G32B32A32Sfloat,                            // VkFormat                                       format
        0                                                           // uint32_t                                       offset
      },
      {
        1,                                                          // uint32_t                                       location
        vertex_binding_description[0].binding,                      // uint32_t                                       binding
        vk::Format::eR32G32Sfloat,                                  // VkFormat                                       format
        reinterpret_cast<size_t>(&((VertexData*)0)->Texcoords)      // uint32_t                                       offset
      },
      {
        2,                                                          // uint32_t                                       location
        vertex_binding_description[1].binding,                      // uint32_t                                       binding
        vk::Format::eR32G32B32A32Sfloat,                            // VkFormat                                       format
        0                                                           // uint32_t                                       offset
      }
    };

    vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info(
      vk::PipelineVertexInputStateCreateFlags( 0 ),                 // VkPipelineVertexInputStateCreateFlags          flags;
      static_cast<uint32_t>(vertex_binding_description.size()),     // uint32_t                                       vertexBindingDescriptionCount
      vertex_binding_description.data(),                            // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
      static_cast<uint32_t>(vertex_attribute_descriptions.size()),  // uint32_t                                       vertexAttributeDescriptionCount
      vertex_attribute_descriptions.data()                          // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
    );

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info(
      vk::PipelineInputAssemblyStateCreateFlags( 0 ),               // VkPipelineInputAssemblyStateCreateFlags        flags
      vk::PrimitiveTopology::eTriangleList,                         // VkPrimitiveTopology                            topology
      VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
    );

    vk::PipelineViewportStateCreateInfo viewport_state_create_info(
      vk::PipelineViewportStateCreateFlags( 0 ),                    // VkPipelineViewportStateCreateFlags             flags
      1,                                                            // uint32_t                                       viewportCount
      nullptr,                                                      // const VkViewport                              *pViewports
      1,                                                            // uint32_t                                       scissorCount
      nullptr                                                       // const VkRect2D                                *pScissors
    );

    vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info(
      vk::PipelineRasterizationStateCreateFlags( 0 ),               // VkPipelineRasterizationStateCreateFlags        flags
      VK_FALSE,                                                     // VkBool32                                       depthClampEnable
      VK_FALSE,                                                     // VkBool32                                       rasterizerDiscardEnable
      vk::PolygonMode::eFill,                                       // VkPolygonMode                                  polygonMode
      vk::CullModeFlagBits::eBack,                                  // VkCullModeFlags                                cullMode
      vk::FrontFace::eCounterClockwise,                             // VkFrontFace                                    frontFace
      VK_FALSE,                                                     // VkBool32                                       depthBiasEnable
      0.0f,                                                         // float                                          depthBiasConstantFactor
      0.0f,                                                         // float                                          depthBiasClamp
      0.0f,                                                         // float                                          depthBiasSlopeFactor
      1.0f                                                          // float                                          lineWidth
    );

    vk::PipelineMultisampleStateCreateInfo multisample_state_create_info(
      vk::PipelineMultisampleStateCreateFlags( 0 ),                 // VkPipelineMultisampleStateCreateFlags          flags
      vk::SampleCountFlagBits::e1,                                  // VkSampleCountFlagBits                          rasterizationSamples
      VK_FALSE,                                                     // VkBool32                                       sampleShadingEnable
      1.0f,                                                         // float                                          minSampleShading
      nullptr,                                                      // const VkSampleMask                            *pSampleMask
      VK_FALSE,                                                     // VkBool32                                       alphaToCoverageEnable
      VK_FALSE                                                      // VkBool32                                       alphaToOneEnable
    );

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info(
      vk::PipelineDepthStencilStateCreateFlags( 0 ),                // VkPipelineDepthStencilStateCreateFlags         flags
      VK_TRUE,                                                      // VkBool32                                       depthTestEnable
      VK_TRUE,                                                      // VkBool32                                       depthWriteEnable
      vk::CompareOp::eLessOrEqual                                   // VkCompareOp                                    depthCompareOp
    );

    vk::PipelineColorBlendAttachmentState color_blend_attachment_state(
      VK_FALSE,                                                     // VkBool32                                       blendEnable
      vk::BlendFactor::eSrcAlpha,                                   // VkBlendFactor                                  srcColorBlendFactor
      vk::BlendFactor::eOneMinusSrcAlpha,                           // VkBlendFactor                                  dstColorBlendFactor
      vk::BlendOp::eAdd,                                            // VkBlendOp                                      colorBlendOp
      vk::BlendFactor::eOne,                                        // VkBlendFactor                                  srcAlphaBlendFactor
      vk::BlendFactor::eZero,                                       // VkBlendFactor                                  dstAlphaBlendFactor
      vk::BlendOp::eAdd,                                            // VkBlendOp                                      alphaBlendOp
      vk::ColorComponentFlagBits::eR |                              // VkColorComponentFlags                          colorWriteMask
      vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB |
      vk::ColorComponentFlagBits::eA
    );

    vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info(
      vk::PipelineColorBlendStateCreateFlags( 0 ),                  // VkPipelineColorBlendStateCreateFlags           flags
      VK_FALSE,                                                     // VkBool32                                       logicOpEnable
      vk::LogicOp::eCopy,                                           // VkLogicOp                                      logicOp
      1,                                                            // uint32_t                                       attachmentCount
      &color_blend_attachment_state                                 // const VkPipelineColorBlendAttachmentState     *pAttachments
    );

    std::vector<vk::DynamicState> dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_create_info(
      vk::PipelineDynamicStateCreateFlags( 0 ),                     // VkPipelineDynamicStateCreateFlags              flags
      static_cast<uint32_t>(dynamic_states.size()),                 // uint32_t                                       dynamicStateCount
      dynamic_states.data()                                         // const VkDynamicState                          *pDynamicStates
    );

    vk::GraphicsPipelineCreateInfo pipeline_create_info(
      vk::PipelineCreateFlags( 0 ),                                 // VkPipelineCreateFlags                          flags
      static_cast<uint32_t>(shader_stage_create_infos.size()),      // uint32_t                                       stageCount
      shader_stage_create_infos.data(),                             // const VkPipelineShaderStageCreateInfo         *pStages
      &vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
      &input_assembly_state_create_info,                            // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
      nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
      &viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
      &rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
      &multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
      &depth_stencil_state_create_info,                             // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
      &color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
      &dynamic_state_create_info,                                   // const VkPipelineDynamicStateCreateInfo        *pDynamicState
      *Parameters.PipelineLayout,                                   // VkPipelineLayout                               layout
      *Parameters.RenderPass,                                       // VkRenderPass                                   renderPass
      0,                                                            // uint32_t                                       subpass
      vk::Pipeline(),                                               // VkPipeline                                     basePipelineHandle
      -1                                                            // int32_t                                        basePipelineIndex
    );
    Parameters.GraphicsPipeline = GetDevice().createGraphicsPipelineUnique( vk::PipelineCache(), pipeline_create_info );
  }

  void Sample::CreateVertexBuffers() {
    // 3D model
    std::vector<VertexData> vertex_data;

    const float size = 0.12f;
    float step = 2.0f * size / Parameters.QuadTessellation;
    for( int x = 0; x < Parameters.QuadTessellation; ++x ) {
      for( int y = 0; y < Parameters.QuadTessellation; ++y ) {
        float pos_x = -size + x * step;
        float pos_y = -size + y * step;

        vertex_data.push_back(
        {
          { pos_x, pos_y, 0.0f, 1.0f },
          { static_cast<float>(x) / (Parameters.QuadTessellation), static_cast<float>(y) / (Parameters.QuadTessellation) }
        } );
        vertex_data.push_back(
        {
          { pos_x, pos_y + step, 0.0f, 1.0f },
          { static_cast<float>(x) / (Parameters.QuadTessellation), static_cast<float>(y + 1) / (Parameters.QuadTessellation) }
        } );
        vertex_data.push_back(
        {
          { pos_x + step, pos_y, 0.0f, 1.0f },
          { static_cast<float>(x + 1) / (Parameters.QuadTessellation), static_cast<float>(y) / (Parameters.QuadTessellation) }
        } );
        vertex_data.push_back(
        {
          { pos_x + step, pos_y, 0.0f, 1.0f },
          { static_cast<float>(x + 1) / (Parameters.QuadTessellation), static_cast<float>(y) / (Parameters.QuadTessellation) }
        } );
        vertex_data.push_back(
        {
          { pos_x, pos_y + step, 0.0f, 1.0f },
          { static_cast<float>(x) / (Parameters.QuadTessellation), static_cast<float>(y + 1) / (Parameters.QuadTessellation) }
        } );
        vertex_data.push_back(
        {
          { pos_x + step, pos_y + step, 0.0f, 1.0f },
          { static_cast<float>(x + 1) / (Parameters.QuadTessellation), static_cast<float>(y + 1) / (Parameters.QuadTessellation) }
        } );
      }
    }
    Parameters.VertexBuffer = SampleCommon::CreateBuffer( static_cast<uint32_t>(vertex_data.size()) * sizeof( VertexData ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal );
    SampleCommon::CopyDataToBuffer( static_cast<uint32_t>(vertex_data.size()) * sizeof( VertexData ), vertex_data.data(), *Parameters.VertexBuffer.Handle, 0, vk::AccessFlags( 0 ), vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput );

    // Per instance data (position offsets and distance)
    std::vector<float> instance_data( Parameters.MaxObjectsCount * 4 );
    for( size_t i = 0; i < instance_data.size(); i+=4 ) {
      instance_data[i] = static_cast<float>(std::rand() % 513) / 256.0f - 1.0f;
      instance_data[i + 1] = static_cast<float>(std::rand() % 513) / 256.0f - 1.0f;
      instance_data[i + 2] = static_cast<float>(std::rand() % 513) / 512.0f;
      instance_data[i + 3] = 0.0f;
    }
    Parameters.InstanceBuffer = SampleCommon::CreateBuffer( static_cast<uint32_t>(instance_data.size()) * sizeof( float ), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal );
    SampleCommon::CopyDataToBuffer( static_cast<uint32_t>(instance_data.size()) * sizeof( float ), instance_data.data(), *Parameters.InstanceBuffer.Handle, 0, vk::AccessFlags( 0 ), vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput );
  }

  Sample::~Sample() {
    if( GetDevice() ) {
      GetDevice().waitIdle();
    }
  }

} // namespace ApiWithoutSecrets
