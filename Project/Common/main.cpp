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

#include "Sample.h"

int main( int argc, char **argv ) {
  ApiWithoutSecrets::OS::Window window;
  ApiWithoutSecrets::Sample sample( PROJECT_NAME_STRING );

  try {
    // Window creation
    window.Create( PROJECT_NAME_STRING, 1000, 800 );

    // Vulkan preparations and initialization
    sample.Prepare( window.GetParameters() );

    // Rendering loop
    window.RenderingLoop( sample );
  } catch( std::exception & exception ) {
    std::cout << exception.what() << std::endl;
    return -1;
  }

  return 0;
}
