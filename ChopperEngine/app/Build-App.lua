project "app"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "source/**.h", "source/**.cpp", "source/**.cc", "source/**.hh" , "source/**.c", "source/**.hpp", "shaders/**.slang"}

   includedirs
   {
    "source",
    "../vendor/GLFW/include",
    "../vendor/GLM",
    "../vendor/include",
    "../vendor",
    os.getenv("VULKAN_SDK") .. "/include",
	  -- Include Core
	  "../core/source"
   }

    libdirs {
      os.getenv("VULKAN_SDK") .. "/lib"
   }

   links
   {
      "core",
      "GLFW",
      "vulkan-1"
   }

   targetdir ("../binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../binaries/intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE", "NDEBUG" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST", "NDEBUG" }
       runtime "Release"
       optimize "On"
       symbols "Off"