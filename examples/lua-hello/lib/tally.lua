-- A count per name. init.lua reaches this file as require("lib.tally"): require looks for
-- lib/tally.lua inside the mod's own folder, and returns what the file returns.

local Tally = {}
Tally.__index = Tally

function Tally.new()
    return setmetatable({ counts = {}, total = 0 }, Tally)
end

-- Counts one more for a name, and returns how many that name has now.
function Tally:add(name)
    self.counts[name] = (self.counts[name] or 0) + 1
    self.total = self.total + 1
    return self.counts[name]
end

-- "3 seen: Boar x2, Thorgrim x1": the most counted first, then by name.
function Tally:summary()
    local names = {}
    for name in pairs(self.counts) do
        names[#names + 1] = name
    end
    table.sort(names, function(a, b)
        if self.counts[a] ~= self.counts[b] then
            return self.counts[a] > self.counts[b]
        end
        return a < b
    end)

    local parts = {}
    for i, name in ipairs(names) do
        parts[i] = name .. " x" .. self.counts[name]
    end
    if #parts == 0 then
        return "0 seen"
    end
    return self.total .. " seen: " .. table.concat(parts, ", ")
end

return Tally
