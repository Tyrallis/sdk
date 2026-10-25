-- memory-report: where the client's memory goes. It starts counting Storm's allocations when it
-- loads, and writes a report into its own folder every few minutes, when you ask for one and
-- when the client closes: the address space, the consumers, Storm by source file, the archives
-- and the client databases.

local INTERVAL  = 5 * 60              -- seconds between two reports
local THRESHOLD = 256 * 1024 * 1024   -- MemoryPressure when the largest free block falls under this
local TOP_FILES = 30                  -- source files listed in the text report (the CSV has them all)
local TOP_DBS   = 15                  -- databases listed in the text report

local footprint = wxl.footprint

-- Counting starts now, before the client loads the world: what is allocated from here on is
-- followed, by the source file that asked for it.
local started, why = footprint.Start()
if not started then
    print("Storm's allocations are not counted: " .. why)
end
footprint.WatchPressure(THRESHOLD)

-- --- formatting --------------------------------------------------------------------------

local function mb(bytes)
    return string.format("%9.1f MB", (bytes or 0) / 1048576)
end

local function pad(text, width)
    text = tostring(text)
    if #text > width then return "..." .. text:sub(#text - width + 4) end
    return text .. string.rep(" ", width - #text)
end

-- A CSV field: quoted when it holds a comma or a quote.
local function field(value)
    local text = tostring(value)
    if text:find("[,\"]") then text = "\"" .. text:gsub("\"", "\"\"") .. "\"" end
    return text
end

-- A list of the last snapshot, or an empty one when the runtime cannot give it.
local function list(name)
    local rows = footprint[name]()
    return rows or {}
end

-- --- the report --------------------------------------------------------------------------

local reports = 0

local function writeFile(name, text, mode)
    local file, reason = io.open(name, mode or "w")
    if not file then
        print("cannot write " .. name .. ": " .. tostring(reason))
        return false
    end
    file:write(text)
    file:close()
    return true
end

-- Writes memory-report.txt and memory-sources.csv, and adds a line to memory-history.csv.
-- Answers the text report's name.
local function report(reason)
    local taken, why = footprint.Snapshot()
    if not taken then
        print("no report: " .. why)
        return nil
    end
    reports = reports + 1
    local now = os.date("%Y-%m-%d %H:%M:%S")
    local out = {}
    local function line(...) out[#out + 1] = string.format(...) end

    line("Memory report %d, %s (%s)", reports, now, reason)
    line("Storm's allocations counted since the mod loaded: %s", footprint.Running() and "yes" or "no")
    line("")

    -- The address space: what a 32-bit client runs out of first.
    local regions = list("Regions")
    local committed, reserved, free, largest, freeBlocks = 0, 0, 0, 0, 0
    local kinds = {}
    for _, r in ipairs(regions) do
        if r.state == "committed" then
            committed = committed + r.bytes
            kinds[#kinds + 1] = string.format("%s %.1f MB", r.type, r.bytes / 1048576)
        elseif r.state == "reserved" then
            reserved = reserved + r.bytes
        else
            free, largest, freeBlocks = r.bytes, r.largest, r.regions
        end
    end
    line("Address space")
    line("  committed   %s   (%s)", mb(committed), table.concat(kinds, ", "))
    line("  reserved    %s", mb(reserved))
    line("  free        %s   in %d blocks; the largest is %s", mb(free), freeBlocks, mb(largest))
    line("")

    -- The consumers. They overlap: the script heap, the databases and the archives' tables are
    -- allocated through Storm, so they are inside `storm` too.
    local categories = list("Categories")
    local byName = {}
    line("Consumers, largest first (they overlap: see the README)")
    line("  %s %s %12s %s", pad("name", 34), pad("owner", 14), "count", "         bytes        peak")
    for _, c in ipairs(categories) do
        byName[c.name] = c
        line("  %s %s %12d %s %s", pad(c.name, 34), pad(c.owner, 14), c.count, mb(c.bytes), mb(c.peak))
    end
    line("")

    -- Storm by the source file that asked.
    local sources = list("Sources")
    local stormBytes = 0
    for _, s in ipairs(sources) do stormBytes = stormBytes + s.bytes end
    line("Storm by source file: %d files, %s live (top %d)", #sources, mb(stormBytes), TOP_FILES)
    line("  %s %s %10s %s %12s", pad("file", 60), "     bytes", "blocks", "       peak", "allocations")
    for i = 1, math.min(TOP_FILES, #sources) do
        local s = sources[i]
        line("  %s %s %10d %s %12d", pad(s.file, 60), mb(s.bytes), s.count, mb(s.peak), s.allocations)
    end
    line("")

    -- The archives the client mounted.
    local archives = list("Archives")
    line("Archives: %d mounted", #archives)
    for _, a in ipairs(archives) do
        line("  %s priority %4d  hash %8d  blocks %8d  %s", pad(a.path, 44), a.priority, a.hash_entries,
             a.block_entries, mb(a.bytes))
    end
    line("")

    -- The client databases, largest first.
    local tables = list("Tables")
    table.sort(tables, function(x, y) return x.bytes > y.bytes end)
    local tableBytes = 0
    for _, t in ipairs(tables) do tableBytes = tableBytes + t.bytes end
    line("Client databases: %d loaded, %s (top %d)", #tables, mb(tableBytes), TOP_DBS)
    for i = 1, math.min(TOP_DBS, #tables) do
        local t = tables[i]
        line("  %s %8d records of %5d bytes  %s", pad(t.name, 44), t.records, t.record_size, mb(t.bytes))
    end

    writeFile("memory-report.txt", table.concat(out, "\n") .. "\n")

    -- Every source file, for a spreadsheet.
    local csv = {"file,bytes,blocks,peak,allocations"}
    for _, s in ipairs(sources) do
        csv[#csv + 1] = table.concat({field(s.file), s.bytes, s.count, s.peak, s.allocations}, ",")
    end
    writeFile("memory-sources.csv", table.concat(csv, "\n") .. "\n")

    -- One line per report, to follow the numbers over a session.
    local function bytesOf(name) return byName[name] and byName[name].bytes or 0 end
    local history = io.open("memory-history.csv", "r")
    if history then
        history:close()
    else
        writeFile("memory-history.csv",
                  "time,reason,committed,reserved,free,largest_free,storm,textures_managed,textures_default," ..
                  "buffers,sound,script,archives,tables\n")
    end
    writeFile("memory-history.csv", table.concat({
        now, field(reason), committed, reserved, free, largest, bytesOf("storm"), bytesOf("textures.managed"),
        bytesOf("textures.default"), bytesOf("buffers.managed") + bytesOf("buffers.default"), bytesOf("sound"),
        bytesOf("script"), bytesOf("archives"), bytesOf("tables"),
    }, ",") .. "\n", "a")

    print(string.format("report %d written (%s): %s committed, largest free block %s", reports, reason,
                        mb(committed), mb(largest)))
    return "memory-report.txt"
end

-- --- when it reports ---------------------------------------------------------------------

local sinceLast = 0

registerForEvent("onUpdate", function(delta)
    sinceLast = sinceLast + delta
    if sinceLast < INTERVAL then return end
    sinceLast = 0
    report("every " .. INTERVAL / 60 .. " minutes")
end)

registerForEvent("onShutdown", function()
    report("the client is closing")
    footprint.Stop()
end)

-- From the chat box: /run MemoryReport(), or /memreport once in the world.
wxl.script.RegisterFunction("MemoryReport", function()
    return report("asked for")
end)

function WorldScript:OnWorldEnter(map_id)
    wxl.script.Run("SLASH_MEMORYREPORT1 = '/memreport'; " ..
                   "SlashCmdList['MEMORYREPORT'] = function() print('memory-report: ' .. tostring(MemoryReport())) end",
                   "memory-report")
end

-- The address space running short: the moment worth a report of its own.
function WorldScript:OnMemoryPressure(largest_free, free, committed, threshold)
    print(string.format("address space low: the largest free block is %s, under %s; %s committed",
                        mb(largest_free), mb(threshold), mb(committed)))
    report("memory pressure")
end

-- What the client mounts after it started, as patches arrive.
function AssetScript:OnArchiveMounted(path, priority, hash_entries, block_entries, bytes)
    print(string.format("archive mounted: %s (priority %d, %d files, tables %s)", path, priority, block_entries,
                        mb(bytes)))
end
