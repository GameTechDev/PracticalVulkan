@ECHO OFF

REM /////////////////////////////////////////////////////////////////////////////////////////////
REM // Copyright 2018 Intel Corporation
REM //
REM // Licensed under the Apache License, Version 2.0 (the "License");
REM // you may not use this file except in compliance with the License.
REM // You may obtain a copy of the License at
REM //
REM // http://www.apache.org/licenses/LICENSE-2.0
REM //
REM // Unless required by applicable law or agreed to in writing, software
REM // distributed under the License is distributed on an "AS IS" BASIS,
REM // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM // See the License for the specific language governing permissions and
REM // limitations under the License.
REM /////////////////////////////////////////////////////////////////////////////////////////////

echo Preparing 'API without Secrets: The Practical Approach to Vulkan' solution...

mkdir build
cd build

cmake.exe .. -DUSE_PLATFORM=VK_USE_PLATFORM_WIN32_KHR -G "Visual Studio 16 2019" -DARCHITECTURE=x64

start "" "PracticalApproachToVulkan.sln"

cd ..