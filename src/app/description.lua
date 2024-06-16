return {
    kind = "ConsoleApp",
    staticruntime = "On",
    runtime = "Release",
    defines = {"_ITERATOR_DEBUG_LEVEL=0"},
    externalincludedirs = {_OPTIONS["tdpath"], _OPTIONS["tdpath"] .. "/tdlib/include"},
    libdirs = {_OPTIONS["tdpath"] .. "/tdlib/lib"},
    links = {
        -- telegram
        "tdactor.lib",
        "tdapi.lib",
        "tdclient.lib",
        "tdcore.lib",
        "tddb.lib",
        "tdjson_static.lib",
        "tdmtproto.lib",
        "tdnet.lib",
        "tdsqlite.lib",
        "tdutils.lib",

        -- winapi 
        "crypt32.lib",
        "Ws2_32.lib",
        "Normaliz.lib",
        "Kernel32.lib",
        "psapi.lib",
    }
}
