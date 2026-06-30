target("clib-util")
    set_kind("headeronly")
    add_headerfiles("include/**.h", "include/**.hpp")
    add_includedirs("include", {public = true})