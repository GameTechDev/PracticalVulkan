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

#include "SampleCommon.h"
#include "VulkanFunctions.h"
#include "imgui/imgui.h"

namespace ApiWithoutSecrets {

  GUI::GUI( const SampleCommon &parent ) :
    Parent( parent ) {
  }

  void GUI::Prepare( size_t resource_count, uint32_t width, uint32_t height ) {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiStyle &gui_style = ImGui::GetStyle();
    gui_style.Colors[ImGuiCol_TitleBg] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
    gui_style.Colors[ImGuiCol_TitleBgActive] = ImVec4( 0.16f, 0.29f, 0.48f, 0.9f );
    gui_style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.06f, 0.07f, 0.08f, 0.8f );
    gui_style.Colors[ImGuiCol_PlotHistogram] = ImVec4( 0.20f, 0.40f, 0.60f, 1.0f );
    gui_style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 0.20f, 0.45f, 0.90f, 1.0f );

    OnWindowSizeChanged( width, height );

    Vulkan.DrawingResources.resize( resource_count );

    CreateTexture();
    CreateDescriptorResources();
    CreatePipelineLayout();
    CreateGraphicsPipeline();
  }

  void GUI::OnWindowSizeChanged( uint32_t width, uint32_t height ) {
    ImGui::GetIO().DisplaySize.x = static_cast<float>(width);
    ImGui::GetIO().DisplaySize.y = static_cast<float>(height);
  }

  void GUI::StartFrame( TimerData const & timer, OS::ProjectBase::MouseInputStateData & mouse_state ) {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = timer.GetDeltaTime();
    io.Framerate = timer.GetAverageFPS();
    io.MouseDown[0] = mouse_state.Buttons[0].IsPressed;
    io.MouseDown[1] = mouse_state.Buttons[1].IsPressed;
    io.MousePos = ImVec2( static_cast<float>(mouse_state.Position.X), static_cast<float>(mouse_state.Position.Y) );
    mouse_state.Available = !io.WantCaptureMouse;

    ImGui::NewFrame();

    ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x - 120.0f, 20.0f ) );
    ImGui::SetNextWindowSize( ImVec2( 100.0f, 100.0 ) );
    ImGui::Begin( "Stats", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar );

    static bool show_fps = true;

    if( ImGui::RadioButton( "FPS", show_fps ) ) {
      show_fps = true;
    }
    ImGui::SameLine();
    if( ImGui::RadioButton( "ms", !show_fps ) ) {
      show_fps = false;
    }

    if( show_fps ) {
      ImGui::SetCursorPosX( 20.0f );
      ImGui::Text( "%7.1f", timer.GetAverageFPS() );

      auto & histogram = timer.GetFPSHistogram();
      ImGui::PlotHistogram( "", histogram.data(), static_cast<int>(histogram.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2( 85.0f, 30.0f ) );
    } else {
      ImGui::SetCursorPosX( 20.0f );
      ImGui::Text( "%9.3f", timer.GetAverageDeltaTime() );

      auto & histogram = timer.GetDeltaTimeHistogram();
      ImGui::PlotHistogram( "", histogram.data(), static_cast<int>(histogram.size()), 0, nullptr, 0.0f, FLT_MAX, ImVec2( 85.0f, 30.0f ) );
    }

    ImGui::End();
    ImGui::SetNextWindowPos( ImVec2( 20.0f, 20.0f ) );
  }

  void GUI::Draw( uint32_t resource_index, vk::CommandBuffer & command_buffer, vk::RenderPass const & render_pass, vk::Framebuffer const & framebuffer ) {
    // Begin command buffer
    command_buffer.begin( { vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
    // Begin render pass
    {
      vk::RenderPassBeginInfo render_pass_begin_info(
        render_pass,                                        // VkRenderPass                           renderPass
        framebuffer,                                        // VkFramebuffer                          framebuffer
        {                                                   // VkRect2D                               renderArea
          vk::Offset2D(),                                     // VkOffset2D                             offset
          Parent.GetSwapChain().Extent,                       // VkExtent2D                             extent;
        }
      );
      command_buffer.beginRenderPass( render_pass_begin_info, vk::SubpassContents::eInline );
    }
    // Bind pipeline and set pipeline state
    {
      command_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *Vulkan.GraphicsPipeline );
      command_buffer.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, *Vulkan.PipelineLayout, 0, { *Vulkan.DescriptorSet.Handle }, {} );

      vk::Viewport viewport(
        0.0f,                                               // float                                  x
        0.0f,                                               // float                                  y
        ImGui::GetIO().DisplaySize.x,                       // float                                  width
        ImGui::GetIO().DisplaySize.y,                       // float                                  height
        0.0f,                                               // float                                  minDepth
        1.0f                                                // float                                  maxDepth
      );
      command_buffer.setViewport( 0, { viewport } );
    }
    // Draw
    {
      DrawFrameData( command_buffer, Vulkan.DrawingResources[resource_index] );
    }
    // End command buffer
    {
      command_buffer.endRenderPass();
      command_buffer.end();
    }
  }

  void GUI::DrawFrameData( vk::CommandBuffer & command_buffer, GUIResources::DrawDataParameters &drawing_resources ) {
    ImGui::Render();

    ImDrawData* draw_data = ImGui::GetDrawData();

    if( draw_data->TotalVtxCount == 0 ) {
      return;
    }

    // Check if vertex buffer is created or if it is too small and needs to be re-created
    {
      uint32_t vertex_buffer_size = sizeof( ImDrawVert ) * draw_data->TotalVtxCount;
      if( (!drawing_resources.VertexBuffer.Buffer.Handle) ||
        (drawing_resources.VertexBuffer.Buffer.Size < vertex_buffer_size) ) {
        drawing_resources.VertexBuffer.Buffer = Parent.CreateBuffer( vertex_buffer_size, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible );
        drawing_resources.VertexBuffer.Pointer = Parent.GetDevice().mapMemory( *drawing_resources.VertexBuffer.Buffer.Memory, 0, vertex_buffer_size );
      }
    }
    // Check if index buffer is created or if it is too small and needs to be re-created
    {
      uint32_t index_buffer_size = sizeof( ImDrawIdx ) * draw_data->TotalIdxCount;
      if( (!drawing_resources.IndexBuffer.Buffer.Handle) ||
        (drawing_resources.IndexBuffer.Buffer.Size < index_buffer_size) ) {
        drawing_resources.IndexBuffer.Buffer = Parent.CreateBuffer( index_buffer_size, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible );
        drawing_resources.IndexBuffer.Pointer = Parent.GetDevice().mapMemory( *drawing_resources.IndexBuffer.Buffer.Memory, 0, index_buffer_size );
      }
    }
    // Upload vertex and index data
    {
      ImDrawVert *vertex_memory_pointer = reinterpret_cast<ImDrawVert*>(drawing_resources.VertexBuffer.Pointer);
      ImDrawIdx *index_memory_pointer = reinterpret_cast<ImDrawIdx*>(drawing_resources.IndexBuffer.Pointer);

      for( int i = 0; i < draw_data->CmdListsCount; i++ ) {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];

        memcpy( vertex_memory_pointer, cmd_list->VtxBuffer.Data, sizeof( ImDrawVert ) * cmd_list->VtxBuffer.Size );
        memcpy( index_memory_pointer, cmd_list->IdxBuffer.Data, sizeof( ImDrawIdx ) * cmd_list->IdxBuffer.Size );

        vertex_memory_pointer += cmd_list->VtxBuffer.Size;
        index_memory_pointer += cmd_list->IdxBuffer.Size;
      }

      std::vector<vk::MappedMemoryRange> memory_ranges = {
        {
          *drawing_resources.VertexBuffer.Buffer.Memory,  // VkDeviceMemory             memory
          0,                                              // VkDeviceSize               offset
          VK_WHOLE_SIZE                                   // VkDeviceSize               size
        },
        {
          *drawing_resources.IndexBuffer.Buffer.Memory,   // VkDeviceMemory             memory
          0,                                              // VkDeviceSize               offset
          VK_WHOLE_SIZE                                   // VkDeviceSize               size
        }
      };
      Parent.GetDevice().flushMappedMemoryRanges( memory_ranges );
    }
    // Bind vertex and index buffers
    {
      command_buffer.bindVertexBuffers( 0, { *drawing_resources.VertexBuffer.Buffer.Handle }, { 0 } );
      command_buffer.bindIndexBuffer( *drawing_resources.IndexBuffer.Buffer.Handle, 0, vk::IndexType::eUint16 );
    }
    // Setup scale and translation:
    {
      std::vector<float> scale_and_translation = {
        2.0f / ImGui::GetIO().DisplaySize.x,            // X scale
        2.0f / ImGui::GetIO().DisplaySize.y,            // Y scale
        -1.0f,                                          // X translation
        -1.0f                                           // Y translation
      };
      command_buffer.pushConstants( *Vulkan.PipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof( float ) * static_cast<uint32_t>(scale_and_translation.size()), scale_and_translation.data() );
    }
    // Render GUI
    {
      int vertex_offset = 0;
      int index_offset = 0;
      for( int list = 0; list < draw_data->CmdListsCount; list++ ) {
        const ImDrawList* cmd_list = draw_data->CmdLists[list];
        for( int command_index = 0; command_index < cmd_list->CmdBuffer.Size; command_index++ ) {
          const ImDrawCmd* draw_command = &cmd_list->CmdBuffer[command_index];

          vk::Rect2D scissor(
            vk::Offset2D(
              (int32_t)(draw_command->ClipRect.x) > 0 ? (int32_t)(draw_command->ClipRect.x) : 0,
              (int32_t)(draw_command->ClipRect.y) > 0 ? (int32_t)(draw_command->ClipRect.y) : 0
            ),
            vk::Extent2D(
              (uint32_t)(draw_command->ClipRect.z - draw_command->ClipRect.x),
              (uint32_t)(draw_command->ClipRect.w - draw_command->ClipRect.y)
            )
          );
          command_buffer.setScissor( 0, { scissor } );
          command_buffer.drawIndexed( draw_command->ElemCount, 1, index_offset, vertex_offset, 0 );

          index_offset += draw_command->ElemCount;
        }
        vertex_offset += cmd_list->VtxBuffer.Size;
      }
    }
  }

  void GUI::CreateTexture() {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t data_size = 0;
    unsigned char* pixels;
    // Get image data
    {
      int w = 0, h = 0;
      ImGui::GetIO().Fonts->GetTexDataAsRGBA32( &pixels, &w, &h );
      width = static_cast<uint32_t>(w);
      height = static_cast<uint32_t>(h);
      data_size = width * height * 4;
    }
    // Create image and sampler
    {
      Vulkan.Image = Parent.CreateImage( width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor );
      Vulkan.Image.Sampler = Parent.CreateSampler( vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, VK_FALSE );
    }
    // Copy data to image's memory
    {
      vk::ImageSubresourceRange image_subresource_range(
        vk::ImageAspectFlagBits::eColor,  // VkImageAspectFlags     aspectMask
        0,                                // uint32_t               baseMipLevel
        1,                                // uint32_t               levelCount
        0,                                // uint32_t               baseArrayLayer
        1                                 // uint32_t               layerCount
      );
      Parent.CopyDataToImage( data_size, pixels, *Vulkan.Image.Handle, width, height, image_subresource_range, vk::ImageLayout::eUndefined, vk::AccessFlags( 0 ), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader );
    }
  }

  void GUI::CreateDescriptorResources() {
    // Create descriptor pool and allocate descriptor set
    {
      std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
        {
          0,                                                  // uint32_t                             binding
          vk::DescriptorType::eCombinedImageSampler,          // VkDescriptorType                     descriptorType
          1,                                                  // uint32_t                             descriptorCount
          vk::ShaderStageFlagBits::eFragment                  // VkShaderStageFlags                   stageFlags
        }
      };
      std::vector<vk::DescriptorPoolSize> pool_sizes = {
        {
          vk::DescriptorType::eCombinedImageSampler,          // VkDescriptorType                     type
          1                                                   // uint32_t                             descriptorCount
        }
      };
      Vulkan.DescriptorSet = Parent.CreateDescriptorResources( layout_bindings, pool_sizes );
    }
    // Update descriptor set
    {
      std::vector<vk::DescriptorImageInfo> image_infos = {
        {
          *Vulkan.Image.Sampler,                              // VkSampler                      sampler
          *Vulkan.Image.View,                                 // VkImageView                    imageView
          vk::ImageLayout::eShaderReadOnlyOptimal             // VkImageLayout                  imageLayout
        }
      };
      Parent.UpdateDescriptorSet( *Vulkan.DescriptorSet.Handle, vk::DescriptorType::eCombinedImageSampler, 0, 0, image_infos );
    }
  }

  void GUI::CreatePipelineLayout() {
    vk::PushConstantRange range(
      vk::ShaderStageFlagBits::eVertex,               // VkShaderStageFlags             stageFlags
      0,                                              // uint32_t                       offset
      sizeof(float) * 4                               // uint32_t                       size
    );
    Vulkan.PipelineLayout = Parent.CreatePipelineLayout( { *Vulkan.DescriptorSet.Layout }, { range } );
  }

  void GUI::CreateGraphicsPipeline() {
    vk::UniqueShaderModule vertex_shader_module = Parent.CreateShaderModule( "Data/Common/gui.vert.spv" );
    vk::UniqueShaderModule fragment_shader_module = Parent.CreateShaderModule( "Data/Common/gui.frag.spv" );

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
        sizeof( ImDrawVert ),                                       // uint32_t                                       stride
        vk::VertexInputRate::eVertex                                // VkVertexInputRate                              inputRate
      }
    };

    std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions = {
      {
        0,                                                          // uint32_t                                       location
        vertex_binding_description[0].binding,                      // uint32_t                                       binding
        vk::Format::eR32G32Sfloat,                                  // VkFormat                                       format
        reinterpret_cast<size_t>(&((ImDrawVert*)0)->pos)            // uint32_t                                       offset
      },
      {
        1,                                                          // uint32_t                                       location
        vertex_binding_description[0].binding,                      // uint32_t                                       binding
        vk::Format::eR32G32Sfloat,                                  // VkFormat                                       format
        reinterpret_cast<size_t>(&((ImDrawVert*)0)->uv)             // uint32_t                                       offset
      },
      {
        2,                                                          // uint32_t                                       location
        vertex_binding_description[0].binding,                      // uint32_t                                       binding
        vk::Format::eR8G8B8A8Unorm,                                 // VkFormat                                       format
        reinterpret_cast<size_t>(&((ImDrawVert*)0)->col)            // uint32_t                                       offset
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
      vk::PrimitiveTopology::eTriangleList                          // VkPrimitiveTopology                            topology
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
      vk::CullModeFlagBits::eNone,                                  // VkCullModeFlags                                cullMode
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
      VK_FALSE,                                                     // VkBool32                                       depthTestEnable
      VK_FALSE,                                                     // VkBool32                                       depthWriteEnable
      vk::CompareOp::eAlways                                        // VkCompareOp                                    depthCompareOp
    );

    vk::PipelineColorBlendAttachmentState color_blend_attachment_state(
      VK_TRUE,                                                      // VkBool32                                       blendEnable
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

    vk::UniqueRenderPass render_pass;
    {
      std::vector<RenderPassAttachmentData> attachment_descriptions = {
        {
          Parent.GetSwapChain().Format,                             // VkFormat                       format
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

      std::vector<RenderPassSubpassData> subpass_descriptions = {
        {
          {},                                                       // std::vector<VkAttachmentReference> const  &InputAttachments
          {                                                         // std::vector<VkAttachmentReference> const  &ColorAttachments
            {
              0,                                                      // uint32_t                                   attachment
              vk::ImageLayout::eColorAttachmentOptimal                // VkImageLayout                              layout
            }
          },
          {                                                         // VkAttachmentReference const               &DepthStencilAttachment;
            1,                                                        // uint32_t                                   attachment
            vk::ImageLayout::eDepthStencilAttachmentOptimal           // VkImageLayout                              layout
          }
        }
      };

      std::vector<vk::SubpassDependency> dependencies = {
        {
          VK_SUBPASS_EXTERNAL,                                      // uint32_t                       srcSubpass
          0,                                                        // uint32_t                       dstSubpass
          vk::PipelineStageFlagBits::eColorAttachmentOutput,        // VkPipelineStageFlags           srcStageMask
          vk::PipelineStageFlagBits::eColorAttachmentOutput,        // VkPipelineStageFlags           dstStageMask
          vk::AccessFlagBits::eColorAttachmentWrite,                // VkAccessFlags                  srcAccessMask
          vk::AccessFlagBits::eColorAttachmentWrite,                // VkAccessFlags                  dstAccessMask
          vk::DependencyFlagBits::eByRegion                         // VkDependencyFlags              dependencyFlags
        },
        {
          0,                                                        // uint32_t                       srcSubpass
          VK_SUBPASS_EXTERNAL,                                      // uint32_t                       dstSubpass
          vk::PipelineStageFlagBits::eColorAttachmentOutput,        // VkPipelineStageFlags           srcStageMask
          vk::PipelineStageFlagBits::eColorAttachmentOutput,        // VkPipelineStageFlags           dstStageMask
          vk::AccessFlagBits::eColorAttachmentWrite,                // VkAccessFlags                  srcAccessMask
          vk::AccessFlagBits::eColorAttachmentWrite,                // VkAccessFlags                  dstAccessMask
          vk::DependencyFlagBits::eByRegion                         // VkDependencyFlags              dependencyFlags
        }
      };
      render_pass = Parent.CreateRenderPass( attachment_descriptions, subpass_descriptions, dependencies );
    }

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
      *Vulkan.PipelineLayout,                                       // VkPipelineLayout                               layout
      *render_pass,                                                 // VkRenderPass                                   renderPass
      0,                                                            // uint32_t                                       subpass
      vk::Pipeline(),                                               // VkPipeline                                     basePipelineHandle
      -1                                                            // int32_t                                        basePipelineIndex
    );
    Vulkan.GraphicsPipeline = Parent.GetDevice().createGraphicsPipelineUnique( vk::PipelineCache(), pipeline_create_info );
  }

  GUI::~GUI() {
    if (ImGui::GetCurrentContext()) {
      ImGui::DestroyContext();
    }
  }

} // namespace ApiWithoutSecrets
