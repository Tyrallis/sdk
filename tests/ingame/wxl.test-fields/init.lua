-- Descriptor fields. Watches Player.Coinage for good and Unit.Health for ten changes, then
-- gives Unit.Health back from inside the event. In the chat box:
--   /run print(TestFields())          reads the player's (and the target's) fields
--   /run print(TestFieldsHealth())    watches Unit.Health again for ten more changes

local HEALTH_LINES = 10

local function show(v, why)
    if v == nil then return "nil(" .. tostring(why) .. ")" end
    return tostring(v)
end

local function watch(kind, name)
    local ok, why = wxl.object.Watch(kind, name)
    print("watch " .. kind .. "." .. name .. ": " .. show(ok, why))
end

watch("Player", "Coinage")
watch("Unit", "Health")
-- A field the client keeps no copy of: refused, saying why.
watch("Unit", "CreatedBy")

local healthLines = 0

Observe("ObjectScript", "OnObjectFieldChanged", function(object, kind, field, count, old, new)
    if field == wxl.field.Player.Coinage then
        print(string.format("Coinage %s -> %s (%+d copper) on %s", old, new, new - old, tostring(object)))
    elseif field == wxl.field.Unit.Health then
        healthLines = healthLines + 1
        print(string.format("Health %s %s %d -> %d (%d/%d)", kind, tostring(object), old, new,
                            healthLines, HEALTH_LINES))
        if healthLines == HEALTH_LINES then
            print("Unwatch Unit.Health from inside the event: " .. show(wxl.object.Unwatch("Unit", "Health")))
        end
    else
        print("UNEXPECTED field " .. field .. " reached wxl.test-fields")
    end
end)

local function read(object, names)
    local parts = {}
    for _, name in ipairs(names) do
        parts[#parts + 1] = name .. "=" .. show(object:Field(name))
    end
    return table.concat(parts, " ")
end

local function reading()
    local me = wxl.object.LocalPlayer()
    if not me then return "no player yet" end
    local text = "me " .. read(me, { "Health", "MaxHealth", "Level", "Coinage", "Xp", "NextLevelXp" })
    -- Three spellings of one field, and the getter, read the same.
    local same = me:Field("UNIT_FIELD_HEALTH") == me:Field("Unit.Health") and
                 me:Field("Health") == me:Health()
    local race, class, sex, power = me:Field("Bytes0")
    text = text .. " | spellings agree " .. tostring(same) ..
           " | Bytes0 race " .. show(race) .. " class " .. show(class) .. " sex " .. show(sex) .. " power " .. show(power)
    local target = me:Target()
    if target then
        text = text .. " | target " .. tostring(target) .. " " .. read(target, { "Health", "MaxHealth", "Level" })
        -- Another player's block stops before the local fields: Xp answers nil and why.
        if target:Type() == "Player" and target ~= me then text = text .. " Xp=" .. show(target:Field("Xp")) end
    end
    return text
end

if wxl.script then
    wxl.script.RegisterFunction("TestFields", function()
        local text = reading()
        print(text)
        return text
    end)
    wxl.script.RegisterFunction("TestFieldsHealth", function()
        healthLines = 0
        local ok, why = wxl.object.Watch("Unit", "Health")
        local text = "watch Unit.Health again: " .. show(ok, why)
        print(text)
        return text
    end)
end

-- The errors a typo gets: a name two kinds share on a player, a name the kind lacks.
function WorldScript:OnWorldEnter(map_id)
    print("world " .. map_id .. ": " .. reading())
    local me = wxl.object.LocalPlayer()
    if not me then return end
    local _, ambiguous = pcall(me.Field, me, "Flags")
    print("ambiguous: " .. tostring(ambiguous))
    local _, unknown = pcall(me.Field, me, "NoSuchField")
    print("unknown: " .. tostring(unknown))
end

registerForEvent("onInit", function()
    print("test-fields ready, abi " .. wxl.runtime.abi .. " on " .. wxl.runtime.name ..
          "; Coinage is field " .. wxl.field.Player.Coinage .. ", Health " .. wxl.field.Unit.Health)
end)
