@echo off
setlocal

REM Ensure glslc is in the path
set PATH=%VULKAN_SDK%\Bin;%PATH%

REM Specify the folder containing shaders
set SHADER_DIR=Resources\Shaders

REM Change to the shader directory
cd /d %SHADER_DIR%

REM Compile shaders and save as <filename>.<extension>.spv
for %%f in (*.vert *.frag *.comp *.geom *.tesc *.tese) do (
    echo Compiling %%f...
    glslc %%f -o %%f.spv
)

REM Print a completion message
echo All shaders compiled successfully
pause.