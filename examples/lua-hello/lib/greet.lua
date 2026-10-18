-- Reached as require("lib.greet"): require resolves inside the mod's folder.

local greet = {}

function greet.hello(name)
    return "hello from " .. name
end

return greet
