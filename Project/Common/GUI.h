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

#if !defined(GUI_HEADER)
#define GUI_HEADER

#include "Tools.h"
#include "VulkanCommon.h"
#include "Timer.h"

namespace ApiWithoutSecrets {
  class SampleCommon;
  // ************************************************************ //
  // GUIResources                                                 //
  //                                                              //
  // Vulkan specific parameters needed to render GUI              //
  // ************************************************************ //
  struct GUIResources {
    struct DrawDataParameters {
      StagingBufferParameters             VertexBuffer;
      StagingBufferParameters             IndexBuffer;

      DrawDataParameters() :
        VertexBuffer(),
        IndexBuffer() {
      }
    };

    ImageParameters                       Image;
    DescriptorSetParameters               DescriptorSet;
    vk::UniquePipelineLayout              PipelineLayout;
    vk::UniquePipeline                    GraphicsPipeline;
    std::vector<DrawDataParameters>       DrawingResources;

    GUIResources() :
      Image(),
      DescriptorSet(),
      PipelineLayout(),
      GraphicsPipeline(),
      DrawingResources() {
    }
  };

  // ************************************************************ //
  // GUIBase                                                      //
  //                                                              //
  // Class for handling GUI-related rendering                     //
  // ************************************************************ //
  class GUI {
  public:
    GUI( const SampleCommon & parent );
    virtual ~GUI();

    void              Prepare( size_t resource_count, uint32_t width, uint32_t height );
    void              OnWindowSizeChanged( uint32_t width, uint32_t height );

    void              StartFrame( TimerData const & timer, OS::ProjectBase::MouseInputStateData & mouse_state );
    void              Draw( uint32_t resource_index, vk::CommandBuffer & command_buffer, vk::RenderPass const & render_pass, vk::Framebuffer const & framebuffer );

  private:
    const SampleCommon  & Parent;
    GUIResources          Vulkan;

    void              DrawFrameData( vk::CommandBuffer & command_buffer, GUIResources::DrawDataParameters & drawing_resources );
    void              CreateTexture();
    void              CreateDescriptorResources();
    void              CreatePipelineLayout();
    void              CreateGraphicsPipeline();
  };

} // namespace ApiWithoutSecrets

#endif // GUI_HEADER