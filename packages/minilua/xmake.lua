package("minilua")
    set_kind("binary")

    add_urls("https://github.com/LuaJIT/LuaJIT.git")

    on_install(function (package)
        local srcdir = os.curdir()
        local minilua_c = path.join(srcdir, "src", "host", "minilua.c")

        assert(os.isfile(minilua_c), minilua_c)

        local outdir = path.join(package:installdir(), "bin")
        os.mkdir(outdir)

        local cc = get_config("cc") or "gcc"

        local output = path.join(outdir, "minilua")

        os.execv(cc, {
            minilua_c,
            "-O2",
            "-std=c99",
            "-o",
            output
        })
    end)

    on_load(function (package)
        package:add("bindirs", "bin")
    end)