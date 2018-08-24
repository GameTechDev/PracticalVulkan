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

#include <cmath>
#include <fstream>
#include <iostream>
#include "Tools.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ApiWithoutSecrets {

  namespace Tools {

    // ************************************************************ //
    // GetBinaryFileContents                                        //
    //                                                              //
    // Function reading binary contents of a file                   //
    // ************************************************************ //
    std::vector<char> GetBinaryFileContents( std::string const &filename ) {

      std::ifstream file( filename, std::ios::binary );
      if( file.fail() ) {
        throw std::exception( std::string( "Could not open \"" + filename + "\" file!" ).c_str() );
      }

      std::streampos begin, end;
      begin = file.tellg();
      file.seekg( 0, std::ios::end );
      end = file.tellg();

      std::vector<char> result( static_cast<size_t>(end - begin) );
      file.seekg( 0, std::ios::beg );
      file.read( &result[0], end - begin );
      file.close();

      return result;
    }

    // ************************************************************ //
    // GetImageData                                                 //
    //                                                              //
    // Function loading image (texture) data from a specified file  //
    // ************************************************************ //
    std::vector<char> GetImageData( std::string const &filename, int requested_components, int *width, int *height, int *components, int *data_size ) {
      std::vector<char> file_data = Tools::GetBinaryFileContents( filename );
      if( file_data.size() == 0 ) {
        throw std::exception( "Could not get image data!" );
      }

      int tmp_width = 0, tmp_height = 0, tmp_components = 0;
      unsigned char *image_data = stbi_load_from_memory( reinterpret_cast<unsigned char*>(&file_data[0]), static_cast<int>(file_data.size()), &tmp_width, &tmp_height, &tmp_components, requested_components );
      if( (image_data == nullptr) ||
          (tmp_width <= 0) ||
          (tmp_height <= 0) ||
          (tmp_components <= 0) ) {
        throw std::exception( "Could not get image data!" );
      }

      int size = (tmp_width) * (tmp_height) * (requested_components <= 0 ? tmp_components : requested_components);
      if( data_size ) {
        *data_size = size;
      }
      if( width ) {
        *width = tmp_width;
      }
      if( height ) {
        *height = tmp_height;
      }
      if( components ) {
        *components = tmp_components;
      }

      std::vector<char> output(size);
      memcpy( &output[0], image_data, size );

      stbi_image_free( image_data );
      return output;
    }

    // ************************************************************ //
    // GetPerspectiveProjectionMatrix                               //
    //                                                              //
    // Function calculating perspective projection matrix           //
    // ************************************************************ //
    std::array<float, 16> GetPerspectiveProjectionMatrix( float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip ) {
      float f = 1.0f / std::tan( field_of_view * 0.5f * 0.01745329251994329576923690768489f );

      return {
        f / aspect_ratio,
        0.0f,
        0.0f,
        0.0f,

        0.0f,
        -f,
        0.0f,
        0.0f,

        0.0f,
        0.0f,
        far_clip / (near_clip - far_clip),
        -1.0f,

        0.0f,
        0.0f,
        (near_clip * far_clip) / (near_clip - far_clip),
        0.0f
      };
    }

    // ************************************************************ //
    // GetOrthographicsProjectionMatrix                             //
    //                                                              //
    // Function calculating orthographic projection matrix          //
    // ************************************************************ //
    std::array<float, 16> GetOrthographicProjectionMatrix( float const left_plane, float const right_plane, float const top_plane, float const bottom_plane, float const near_plane, float const far_plane ) {
      return {
        2.0f / (right_plane - left_plane),
        0.0f,
        0.0f,
        0.0f,

        0.0f,
        2.0f / (bottom_plane - top_plane),
        0.0f,
        0.0f,

        0.0f,
        0.0f,
        1.0f / (near_plane - far_plane),
        0.0f,

        -(right_plane + left_plane) / (right_plane - left_plane),
        -(bottom_plane + top_plane) / (bottom_plane - top_plane),
        near_plane / (near_plane - far_plane),
        1.0f
      };
    }

    // ************************************************************ //
    // GetRotationMatrix                                            //
    //                                                              //
    // Function calculating rotation matrix around arbitrary axis   //
    // ************************************************************ //
    std::array<float, 16> GetRotationMatrix( float const angle, std::array<float, 3> const axis ) {
      float x = axis[0];
      float y = axis[1];
      float z = axis[2];

      const float c = std::cos( angle * 0.01745329251994329576923690768489f );
      const float _1_c = 1.0f - c;
      const float s = std::sin( angle * 0.01745329251994329576923690768489f );

      return {
        x * x * _1_c + c,
        y * x * _1_c - z * s,
        z * x * _1_c + y * s,
        0.0f,

        x * y * _1_c + z * s,
        y * y * _1_c + c,
        z * y * _1_c - x * s,
        0.0f,

        x * z * _1_c - y * s,
        y * z * _1_c + x * s,
        z * z * _1_c + c,
        0.0f,

        0.0f,
        0.0f,
        0.0f,
        1.0f
      };
    }

  } // namespace Tools

} // namespace ApiWithoutSecrets