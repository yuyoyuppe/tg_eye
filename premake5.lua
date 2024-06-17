if os.host() == "windows" then
    newoption {
        trigger = "tdpath",
        value = "path",
        description = "Path to the compiled https://github.com/tdlib/td"
    }
end

ps = require "deps.premake_scaffold"
workspace "tg_eye"
ps.generate({})
