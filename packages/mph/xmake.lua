package("mph")
    set_kind("library", { headeronly = true })

    add_urls("https://github.com/qlibs/mph.git")

    on_install(function (package)
        os.cp("mph", package:installdir("include"))
    end)