package("dynasm")
    set_kind("library", { headeronly = true })

    add_urls("https://github.com/LuaJIT/LuaJIT.git")

    on_install(function (package)
        os.cp("dynasm", package:installdir("include"))
    end)

rule("dynasm")
    set_extensions(".dpp")

    on_build_file(function (target, sourcefile, opt)
        import("core.project.depend")
        import("utils.progress")

        local dynasm = target:pkg("dynasm")
        local lua = target:pkg("minilua") and path.join(target:pkg("minilua"):installdir(), "bin/minilua") or "lua"

        local dynasm_lua = path.join(dynasm:installdir(), "include/dynasm/dynasm.lua")

        local outdir = path.join(target:targetdir(), "../gen")
        os.mkdir(outdir)

        local outfile = path.join(outdir, path.basename(sourcefile) .. ".cpp")

        depend.on_changed(function ()
            os.vrunv(lua, {
                dynasm_lua,
                sourcefile
            }, { stdout = outfile })

            progress.show(opt.progress, "generating dynasm %s", sourcefile)
        end, {
            files = { sourcefile }
        })
    end)