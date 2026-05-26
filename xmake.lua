local VERSION = "0.1.0"
local HASH = os.iorun("git rev-parse --short HEAD")

add_rules("mode.debug", "mode.release")

set_version("0.1.0")

includes("packages/dynasm")
includes("packages/mph")
includes("packages/minilua")

add_requires("mph", "dynasm", "minilua")

target("knight")
    set_languages("c++17")
    set_kind("binary")
    add_rules("dynasm")

    add_files("src/**.cpp")
    add_files("src/**.dpp")

    add_files("build/gen/*.cpp")
    add_includedirs("build/gen", { public = true })

    add_configfiles("src/knight.hpp.in", {
        variables = {
            VERSION = VERSION,
            GIT_HASH = GIT_HASH
        }
    })

    add_packages("mph", "dynasm", "minilua")
    set_optimize("fastest")