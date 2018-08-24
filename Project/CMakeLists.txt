# /////////////////////////////////////////////////////////////////////////////////////////////
# // Copyright 2018 Intel Corporation
# //
# // Licensed under the Apache License, Version 2.0 (the "License");
# // you may not use this file except in compliance with the License.
# // You may obtain a copy of the License at
# //
# // http://www.apache.org/licenses/LICENSE-2.0
# //
# // Unless required by applicable law or agreed to in writing, software
# // distributed under the License is distributed on an "AS IS" BASIS,
# // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# // See the License for the specific language governing permissions and
# // limitations under the License.
# /////////////////////////////////////////////////////////////////////////////////////////////

cmake_minimum_required (VERSION 3.0)
project( "PracticalApproachToVulkan" )

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

if( CMAKE_BUILD_TYPE STREQUAL "" )
	set( CMAKE_BUILD_TYPE "debug" )
endif()

if( NOT USE_PLATFORM MATCHES "VK_USE_PLATFORM_.*" )
	if( WIN32 )
		set( USE_PLATFORM "VK_USE_PLATFORM_WIN32_KHR" )
	elseif( UNIX )
		set( USE_PLATFORM "VK_USE_PLATFORM_XCB_KHR" )
	endif()
endif()

if( USE_PLATFORM STREQUAL "VK_USE_PLATFORM_WIN32_KHR" )
	add_definitions( -DNOMINMAX )
	set( CMAKE_C_FLAGS_RELWITHDEBINFO    "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MT" )
	set( CMAKE_C_FLAGS_RELEASE           "${CMAKE_C_FLAGS_RELEASE} /MT" )
	set( CMAKE_C_FLAGS_DEBUG             "${CMAKE_C_FLAGS_DEBUG} /MTd" )
	set( CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT" )
	set( CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_RELEASE} /MT" )
	set( CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEBUG} /MTd" )
	set( PLATFORM_LIBRARY                "" )
	set( VULKAN_PLATFORM_HEADER_FILE     "External/Vulkan/vulkan_win32.h" )
elseif( USE_PLATFORM STREQUAL "VK_USE_PLATFORM_XCB_KHR" )
	add_definitions( -std=c++11 )
	set( PLATFORM_LIBRARY dl xcb )
	set( VULKAN_PLATFORM_HEADER_FILE     "External/Vulkan/vulkan_xcb.h" )
elseif( USE_PLATFORM STREQUAL "VK_USE_PLATFORM_XLIB_KHR" )
	add_definitions( -std=c++11 )
	set( PLATFORM_LIBRARY dl X11 )
	set( VULKAN_PLATFORM_HEADER_FILE     "External/Vulkan/vulkan_xlib.h" )
endif()

if( CMAKE_BUILD_TYPE STREQUAL "debug" )
	add_definitions(-D_DEBUG)
endif()

macro( add_executable _number _name )
	set( FOLDER_NAME "${_number} - ${_name}" )
	string( REPLACE " " "_" TARGET_NAME "${_number}-${_name}" )

	file( GLOB PROJECT_FILES "${CMAKE_CURRENT_LIST_DIR}/Samples/${FOLDER_NAME}/*.*" )
	_add_executable( ${TARGET_NAME} ${ARGN} ${PROJECT_FILES} )
	target_compile_definitions( ${TARGET_NAME} PRIVATE PROJECT_NAME_STRING="${FOLDER_NAME}" PROJECT_NUMBER_STRING="${_number}" )
	target_include_directories( ${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Samples/${FOLDER_NAME}" )
	target_link_libraries( ${TARGET_NAME} ${PLATFORM_LIBRARY} )

	if( EXISTS "${CMAKE_SOURCE_DIR}/Samples/${FOLDER_NAME}/Data" )
		file( GLOB DATA_FILES "${CMAKE_SOURCE_DIR}/Samples/${FOLDER_NAME}/Data/*.*" )
		file( COPY ${DATA_FILES} DESTINATION "${CMAKE_SOURCE_DIR}/build/Data/${_number}/" )
	endif()
endmacro()

add_definitions( -DVK_NO_PROTOTYPES )
add_definitions( -D${USE_PLATFORM} )
include_directories( "External" )
include_directories( "External/Vulkan" )
include_directories( "External/imgui" )
include_directories( "Common" )

file( GLOB SHARED_HEADER_FILES
		"Common/OperatingSystem.h"
		"Common/ListOfFunctions.inl"
		"Common/VulkanFunctions.h"
		"Common/Tools.h"
		"Common/VulkanCommon.h"
		"Common/SampleCommon.h"
		"Common/GUI.h"
		"Common/Timer.h" )

file( GLOB SHARED_SOURCE_FILES
		"Common/OperatingSystem.cpp"
		"Common/VulkanFunctions.cpp"
		"Common/Tools.cpp"
		"Common/VulkanCommon.cpp"
		"Common/SampleCommon.cpp"
		"Common/GUI.cpp"
		"Common/Timer.cpp"
		"Common/main.cpp" )

file( GLOB EXTERNAL_HEADER_FILES
		"External/vulkan.hpp"
		"External/stb_image.h" )

file( GLOB EXTERNAL_VULKAN_HEADER_FILES
		"External/Vulkan/vk_platform.h"
		"External/Vulkan/vulkan.h"
		"External/Vulkan/vulkan_core.h"
		${VULKAN_PLATFORM_HEADER_FILE} )

file( GLOB EXTERNAL_GUI_HEADER_FILES
		"External/imgui/*.h" )

file( GLOB EXTERNAL_GUI_SOURCE_FILES
		"External/imgui/*.cpp" )

set( ALL_SAMPLE_FILES
		${SHARED_HEADER_FILES}
		${SHARED_SOURCE_FILES}
		${EXTERNAL_HEADER_FILES}
		${EXTERNAL_VULKAN_HEADER_FILES}
		${EXTERNAL_GUI_HEADER_FILES}
		${EXTERNAL_GUI_SOURCE_FILES} )

source_group( "Header Files\\Common" FILES ${SHARED_HEADER_FILES} )
source_group( "Source Files\\Common" FILES ${SHARED_SOURCE_FILES} )
source_group( "Header Files\\External" FILES ${EXTERNAL_HEADER_FILES} )
source_group( "Header Files\\External\\Vulkan" FILES ${EXTERNAL_VULKAN_HEADER_FILES} )
source_group( "Header Files\\External\\Gui" FILES ${EXTERNAL_GUI_HEADER_FILES} )
source_group( "Source Files\\External\\Gui" FILES ${EXTERNAL_GUI_SOURCE_FILES} )

file( GLOB COMMON_DATA_FILES "${CMAKE_SOURCE_DIR}/Common/Data/*.*" )
file( COPY ${COMMON_DATA_FILES} DESTINATION "${CMAKE_SOURCE_DIR}/build/Data/Common/" )

add_executable( "01" "Frame Resources Count" ${ALL_SAMPLE_FILES} )