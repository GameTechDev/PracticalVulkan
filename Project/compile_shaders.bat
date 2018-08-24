@ECHO OFF
SETLOCAL

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

if [%1] == [] (
    echo Folder not provided! Please specify sample number and shader file name without extension
    echo Example usage: compile_shaders.bat 01 shader
    goto end
)

if [%2] == [] (
    echo Shader name not provided! Please specify sample number name and shader file name without extension
    echo Example usage: compile_shaders.bat 01 shader
    goto end
)

if exist glslangValidator.exe (
    goto convert
)
for %%X in (glslangValidator.exe) do (set glslangValidator=%%~$PATH:X)
if defined glslangValidator (
    goto convert
)

echo Could not find "glslangValidator.exe" file.
goto end

:convert

set folder=Samples\%1\Data

if not exist %folder% (
    echo Could not find specified folder.
    goto end
)

if exist %folder%\%2.vert (
    echo Converting the following shader file: %folder%\%2.vert
    glslangValidator.exe -V -H -o %folder%\%2.vert.spv %folder%\%2.vert > %folder%\%2.vert.spv.txt
)

if exist %folder%\%2.frag (
    echo Converting the following shader file: %folder%\%2.frag
    glslangValidator.exe -V -H -o %folder%\%2.frag.spv %folder%\%2.frag > %folder%\%2.frag.spv.txt
)

set target=build\Data\%1
if exist %target% (
    echo Copying files to %target%:
    copy /Y %folder%\*.* %target%
)

:end

ENDLOCAL