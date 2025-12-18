local function registerEquipment(moveevent, data)
    if data.level then
        moveevent:level(data.level)
    end

    if data.magiclevel then
        moveevent:magicLevel(data.magiclevel)
    end

    if data.premium then
        moveevent:premium(data.premium)
    end

    if data.vocations then
        for _, info in pairs(data.vocations) do
            moveevent:vocation(info[1], info[2] or true, info[3] or false)
        end
    end
end

local MagicFields = {
    -- Event when a creature steps on the item

    ["stepinfield"] = {
        1423, 1424, 1425, 1487, 1488, 1489, 1490, 1491,
        1492, 1493, 1494, 1495, 1496, 1500, 1501, 1502,
        1503, 1504, 1505
    },

    -- Event when the item is added on a tile (e.g; by a rune or a spell)

    ["onaddfield"] = {
        1423, 1424, 1425, 1487, 1488, 1489, 1490, 1491,
        1492, 1493, 1494, 1495, 1496, 1500, 1501, 1502,
        1503, 1504, 1505
    },
}

for eventtype, data in pairs(MagicFields) do
    for _, i in pairs(data) do
        local event = MoveEvent()
        event:id(i)
        event:type(eventtype)
        event:register()
    end
end

--[[

    {
        itemid = 2214,
        event = "defaultequip / defaultdeequip",
        level = 999,
        magiclevel = 999,
        premium = true,
        vocations = {
            -- Vocation name, show in description, last vocation
            {"sorcerer", true, false},
            {"knight", true, true},
        }
    }

]]

local Equipment = {
    ["necklace"] = {
        {
            itemid = 2161,
            event = "defaultequip",
        },
        {
            itemid = 2161,
            event = "defaultdeequip",
        },
        {
            itemid = 2170,
            event = "defaultequip",
        },
        {
            itemid = 2170,
            event = "defaultdeequip",
        },
        {
            itemid = 2172,
            event = "defaultequip",
        },
        {
            itemid = 2172,
            event = "defaultdeequip",
        },
        {
            itemid = 2173,
            event = "defaultequip",
        },
        {
            itemid = 2173,
            event = "defaultdeequip",
        },
        {
            itemid = 2197,
            event = "defaultequip",
        },
        {
            itemid = 2197,
            event = "defaultdeequip",
        },
        {
            itemid = 2198,
            event = "defaultequip",
        },
        {
            itemid = 2198,
            event = "defaultdeequip",
        },
        {
            itemid = 2199,
            event = "defaultequip",
        },
        {
            itemid = 2199,
            event = "defaultdeequip",
        },
        {
            itemid = 2200,
            event = "defaultequip",
        },
        {
            itemid = 2200,
            event = "defaultdeequip",
        },
        {
            itemid = 2201,
            event = "defaultequip",
        },
        {
            itemid = 2201,
            event = "defaultdeequip",
        },
    }, -- End necklace items

    ["head"] = {
        {
            itemid = 2502,
            event = "defaultequip",
        },
        {
            itemid = 2502,
            event = "defaultdeequip",
        },
        {
            itemid = 2664,
            event = "defaultequip",
        },
        {
            itemid = 2664,
            event = "defaultdeequip",
        },
    }, -- End head items

    ["armor"] = {
        {
            itemid = 2503,
            event = "defaultequip",
        },
        {
            itemid = 2503,
            event = "defaultdeequip",
        },
    }, -- End armor items

    ["legs"] = {
        {
            itemid = 2504,
            event = "defaultequip",
        },
        {
            itemid = 2504,
            event = "defaultdeequip",
        },
    }, -- End leg items

    ["feet"] = {
        {
            itemid = 2195,
            event = "defaultequip",
        },
        {
            itemid = 2195,
            event = "defaultdeequip",
        },
        {
            itemid = 2640,
            event = "defaultequip",
        },
        {
            itemid = 2640,
            event = "defaultdeequip",
        },
    }, -- End feet items

    ["ring"] = {
        {
            itemid = 2164,
            event = "defaultequip",
        },
        {
            itemid = 2164,
            event = "defaultdeequip",
        },
        {
            itemid = 2165,
            event = "defaultequip",
        },
        {
            itemid = 2166,
            event = "defaultequip",
        },
        {
            itemid = 2167,
            event = "defaultequip",
        },
        {
            itemid = 2168,
            event = "defaultequip",
        },
        {
            itemid = 2169,
            event = "defaultequip",
        },
        {
            itemid = 2202,
            event = "defaultequip",
        },
        {
            itemid = 2202,
            event = "defaultdeequip",
        },
        {
            itemid = 2203,
            event = "defaultequip",
        },
        {
            itemid = 2203,
            event = "defaultdeequip",
        },
        {
            itemid = 2204,
            event = "defaultequip",
        },
        {
            itemid = 2204,
            event = "defaultdeequip",
        },
        {
            itemid = 2205,
            event = "defaultequip",
        },
        {
            itemid = 2205,
            event = "defaultdeequip",
        },
        {
            itemid = 2206,
            event = "defaultequip",
        },
        {
            itemid = 2206,
            event = "defaultdeequip",
        },
        {
            itemid = 2207,
            event = "defaultequip",
        },
        {
            itemid = 2208,
            event = "defaultequip",
        },
        {
            itemid = 2209,
            event = "defaultequip",
        },
        {
            itemid = 2210,
            event = "defaultequip",
        },
        {
            itemid = 2210,
            event = "defaultdeequip",
        },
        {
            itemid = 2211,
            event = "defaultequip",
        },
        {
            itemid = 2211,
            event = "defaultdeequip",
        },
        {
            itemid = 2212,
            event = "defaultequip",
        },
        {
            itemid = 2212,
            event = "defaultdeequip",
        },
        {
            itemid = 2213,
            event = "defaultequip",
        },
        {
            itemid = 2214,
            event = "defaultequip",
        },
        {
            itemid = 2215,
            event = "defaultequip",
        },
        {
            itemid = 2215,
            event = "defaultdeequip",
        },
        {
            itemid = 2216,
            event = "defaultequip",
        },
        {
            itemid = 2216,
            event = "defaultdeequip",
        },
    }, -- End ring items
}

for slottype, data in pairs(Equipment) do
    for _, content in pairs(data) do
        local event = MoveEvent()
        event:id(content.itemid)
        event:slot(slottype)
        event:type(content.event)
        if content.event == "defaultequip" then
            registerEquipment(event, content)
        end
        event:register()
    end
end