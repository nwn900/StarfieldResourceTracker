-- set minimum xmake version
set_xmakever("3.0.0")

-- project
set_project("ResourceTracker")
set_version("1.0.0")

set_arch("x64")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

-- build modes
add_rules("mode.debug", "mode.releasedbg")

-- include CommonLibSF (maintained fork with Address Library format 5 support)
includes("extern/commonlibsf")

-- packages
add_requires("nlohmann_json")

-- plugin target
target("ResourceTracker", function()
    add_rules("commonlibsf.plugin", {
        name = "ResourceTracker",
        author = "Starfield Resource Tracker Author",
        description = "Track missing crafting components across workbenches",
        options = {
            address_library = true,
            no_struct_use = false,
        }
    })

    set_license("MIT")

    -- packages
    add_packages("nlohmann_json")

    -- source files
    add_files("src/**.cpp")
    add_includedirs("src")

    -- precompiled header
    set_pcxxheader("src/PCH.h")
end)
