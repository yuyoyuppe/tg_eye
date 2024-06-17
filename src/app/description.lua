kind "ConsoleApp"
cppdialect "C++latest"
runtime "Release"

-- pacman -Ql libtd | grep -e "\.a$"
-- Note: order is important for linux linker ._.
local telegramLibs = {
    "tdjson_static",
    "tdjson_private",
    "tdclient",
    "tdcore",
    "tdactor",
    "tdapi",
    "tddb",
    "tdmtproto",
    "tdsqlite",
    "tdnet",
    "tdutils"
}

if os.host() == "windows" then
    defines {"_ITERATOR_DEBUG_LEVEL=0"}
    externalincludedirs {_OPTIONS["tdpath"], _OPTIONS["tdpath"] .. "/tdlib/include"}
    libdirs {_OPTIONS["tdpath"] .. "/tdlib/lib"}

    for _, lib in ipairs(telegramLibs) do
        links { lib .. ".lib" }
    end

    links {
        "crypt32.lib",
        "Ws2_32.lib",
        "Normaliz.lib",
        "Kernel32.lib",
        "psapi.lib"
    }
elseif os.host() == "linux" then
    for _, lib in ipairs(telegramLibs) do
        links { lib }
    end
    -- linux specific libraries
    links {
        "pthread",
        "dl",
        "sqlite3",
        "stdc++exp",
        "ssl",
        "crypto",
        "dl",
        "z",
        "m",
        "stdc++",
    }
end