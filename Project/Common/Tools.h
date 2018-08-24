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

#if !defined(TOOLS_HEADER)
#define TOOLS_HEADER

#include <string>
#include <vector>
#include <array>
#include <functional>
#include "vulkan.hpp"

namespace ApiWithoutSecrets {

  namespace Tools {

    // ************************************************************ //
    // AutoDeleter                                                  //
    //                                                              //
    // Auto-deleter helper template class responsible for calling   //
    // provided function which deletes given object of type T       //
    // ************************************************************ //
    template<class T, class F>
    class AutoDeleter {
    public:
      AutoDeleter() :
        Object( VK_NULL_HANDLE ),
        Deleter( nullptr ),
        Device( VK_NULL_HANDLE ) {
      }

      AutoDeleter( T object, F deleter, VkDevice device ) :
        Object( object ),
        Deleter( deleter ),
        Device( device ) {
      }

      AutoDeleter( AutoDeleter&& other ) {
        *this = std::move( other );
      }

      ~AutoDeleter() {
        if( (Object != VK_NULL_HANDLE) && (Deleter != nullptr) && (Device != VK_NULL_HANDLE) ) {
          Deleter( Device, Object, nullptr );
        }
      }

      AutoDeleter& operator=(AutoDeleter&& other) {
        if( this != &other ) {
          Object = other.Object;
          Deleter = other.Deleter;
          Device = other.Device;
          other.Object = VK_NULL_HANDLE;
        }
        return *this;
      }

      T& Get() {
        return Object;
      }

      bool operator !() const {
        return Object == VK_NULL_HANDLE;
      }

    private:
      AutoDeleter( const AutoDeleter& );
      AutoDeleter& operator=( const AutoDeleter& );
      T         Object;
      F         Deleter;
      VkDevice  Device;
    };

    // ************************************************************ //
    // GetBinaryFileContents                                        //
    //                                                              //
    // Function reading binary contents of a file                   //
    // ************************************************************ //
    std::vector<char> GetBinaryFileContents( std::string const &filename );

    // ************************************************************ //
    // GetImageData                                                 //
    //                                                              //
    // Function loading image (texture) data from a specified file  //
    // ************************************************************ //
    std::vector<char> GetImageData( std::string const &filename, int requested_components, int *width, int *height, int *components, int *data_size );

    // ************************************************************ //
    // GetPerspectiveProjectionMatrix                               //
    //                                                              //
    // Function calculating perspective projection matrix           //
    // ************************************************************ //
    std::array<float, 16> GetPerspectiveProjectionMatrix( float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip );

    // ************************************************************ //
    // GetOrthographicsProjectionMatrix                             //
    //                                                              //
    // Function calculating orthographic projection matrix          //
    // ************************************************************ //
    std::array<float, 16> GetOrthographicProjectionMatrix( float const left_plane, float const right_plane, float const top_plane, float const bottom_plane, float const near_plane, float const far_plane );

    // ************************************************************ //
    // GetRotationMatrix                                            //
    //                                                              //
    // Function calculating rotation matrix around arbitrary axis   //
    // ************************************************************ //
    std::array<float, 16> GetRotationMatrix( float const angle, std::array<float, 3> const axis );

  } // namespace Tools

} // namespace ApiWithoutSecrets


#endif // TOOLS_HEADER