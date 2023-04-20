workspace "ImageEditor"
    language "C++"
    cppdialect "C++17"
    
    architecture "x86_64"
    configurations { "Debug", "Release" }

    warnings "Extra"

    filter { "configurations:Debug" }
        defines { "_DEBUG" }
        symbols "On"

    filter { "configurations:Release" }
        optimize "On"

    filter { "configurations:Release", "system:Windows" }
        linkoptions { "-static", "-static-libgcc", "-static-libstdc++" }
        links { "pthread" }

    filter { }

    targetdir ("bin/%{prj.name}/%{cfg.longname}")
    objdir ("obj/%{prj.name}/%{cfg.longname}")

project "ImageEditor"
    kind "ConsoleApp"
    files "ImageEditor/**"

    includedirs {
		"ImageEditor/src",
		"../Difu/bin/Difu/%{cfg.longname}/include",
        "Dependencies/Raylib/%{cfg.system}/include",
        "Dependencies/fmt/%{cfg.system}/include",
        "Dependencies/entt/include",
        "Dependencies/nfd/include",
    }

    libdirs { 
		"../Difu/bin/Difu/%{cfg.longname}",
        "Dependencies/Raylib/%{cfg.system}/lib",
		"Dependencies/fmt/%{cfg.system}/lib",
		"Dependencies/nfd/lib"
    }

	prebuildcommands {
		"~/Dev/c++/ResourceManager/bin/ResourceManager/Debug/ResourceManager  ImageEditor/Globals.txt -o ImageEditor/src/Globals.hpp -p rl",
	}

	links { "Difu", "raylib", "fmt", "nfd" } -- , "m", "dl", "rt", "X11"
	linkoptions { "`pkg-config gtk+-3.0 --libs`" }

    filter {}

