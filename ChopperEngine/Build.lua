-- premake5.lua
workspace "Chopper Engine"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "app"

   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "vendor"
   include "vendor/GLFW/build-GLFW.lua"
group ""

group "core"
   include "core/build-core.lua"
group ""

include "app/build-app.lua"
