-- Watches are per mod: this one watches Player.Xp and receives that field only, never the money
-- or the health wxl.test-fields watches. Giving back a watch it never took is no mistake and
-- leaves the other mod's watch in place.

local ok, why = wxl.object.Watch("Player", "Xp")
print("watch Player.Xp: " .. tostring(ok) .. " " .. tostring(why))
print("Unwatch of a field this mod never watched (Unit.Health): " ..
      tostring(wxl.object.Unwatch("Unit", "Health")))

Observe("ObjectScript", "OnObjectFieldChanged", function(object, kind, field, count, old, new)
    if field == wxl.field.Player.Xp then
        print("Xp " .. old .. " -> " .. new .. " on " .. tostring(object))
    else
        print("UNEXPECTED field " .. field .. " reached wxl.test-watch-other")
    end
end)
