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

#if !defined(FRAME_RESOURCES_COUNT_HEADER)
#define FRAME_RESOURCES_COUNT_HEADER

#include "SampleCommon.h"

namespace ApiWithoutSecrets {

  // ************************************************************ //
  // SampleFrameResourcesData                                     //
  //                                                              //
  // Struct containing data used to generate a single frame       //
  // ************************************************************ //
  struct SampleFrameResourcesData : public FrameResourcesData {
    vk::UniqueCommandPool                 CommandPool;
    vk::UniqueCommandBuffer               PreCommandBuffer;
    vk::UniqueCommandBuffer               PostCommandBuffer;

    SampleFrameResourcesData() :
      FrameResourcesData(),
      CommandPool(),
      PreCommandBuffer(),
      PostCommandBuffer() {
    }
  };

  // ************************************************************ //
  // SampleParameters                                             //
  //                                                              //
  // Sample-specific parameters                                   //
  // ************************************************************ //
  struct SampleParameters {
    static const int                            MaxObjectsCount = 1000;
    static const int                            QuadTessellation = 40;

    int                                         ObjectsCount;
    int                                         PreSubmitCpuWorkTime;
    int                                         PostSubmitCpuWorkTime;
    float                                       FrameGenerationTime;
    float                                       TotalFrameTime;
    int                                         FrameResourcesCount;

    vk::UniqueRenderPass                        RenderPass;
    vk::UniqueRenderPass                        PostRenderPass;
    DescriptorSetParameters                     DescriptorSet;
    ImageParameters                             BackgroundTexture;
    ImageParameters                             Texture;
    vk::UniquePipelineLayout                    PipelineLayout;
    vk::UniquePipeline                          GraphicsPipeline;
    BufferParameters                            VertexBuffer;
    BufferParameters                            InstanceBuffer;

    SampleParameters() :
      ObjectsCount( 100 ),
      PreSubmitCpuWorkTime( 0 ),
      PostSubmitCpuWorkTime( 0 ),
      FrameGenerationTime( 0 ),
      TotalFrameTime( 0 ),
      FrameResourcesCount( 1 ),
      RenderPass(),
      PostRenderPass(),
      DescriptorSet(),
      BackgroundTexture(),
      Texture(),
      PipelineLayout(),
      GraphicsPipeline(),
      VertexBuffer(),
      InstanceBuffer() {
    }
  };

  // ************************************************************ //
  // Sample                                                       //
  //                                                              //
  // Class presenting practical approach of a specific usecase    //
  // ************************************************************ //
  class Sample : public SampleCommon {
  public:
    Sample( std::string const & title );
    ~Sample();

  private:
    SampleParameters Parameters;

    virtual void    PrepareSample() override;                 // <- Override this for sample-specific resource initialization
    virtual void    PrepareGUIFrame() override;               // <- Override this to add sample-specific GUI

    virtual void    Draw() override;
    void            DrawSample( CurrentFrameData & current_frame );

    virtual void    OnSampleWindowSizeChanged_Pre() override;
    virtual void    OnSampleWindowSizeChanged_Post() override;

    void            CreateFrameResources();
    void            CreateRenderPasses();
    void            CreateDescriptorSet();
    void            CreateTextures();
    void            CreatePipelineLayout();
    void            CreateGraphicsPipeline();
    void            CreateVertexBuffers();
  };

} // namespace ApiWithoutSecrets

#endif // FRAME_RESOURCES_COUNT_HEADER