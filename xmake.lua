local VERSION = "0.1.0"

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

    add_files("$(builddir)/gen/*.cpp")
    add_includedirs("$(builddir)/gen", { public = true })

    on_load(function(target)
        target:add("defines", "KNIGHT_LIB")
        target:add("defines", 'KNIGHT_VERSION="' .. VERSION .. '"')
        target:add("defines", 'KNIGHT_GIT_HASH="' .. os.iorunv("git", {
            "rev-parse",
            "--short",
            "HEAD"
        }):trim() .. '"')
    end)

    add_packages("mph", "dynasm", "minilua")
    set_optimize("fastest")

    set_targetdir("$(builddir)/bin")