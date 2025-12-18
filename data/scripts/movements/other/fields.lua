local itemTable = {
    [1506] = {initDamage = 300},
    [1507] = {initDamage = 300},
}

local event = MoveEvent()

function event.onStepIn(creature, item, position, fromPosition)
    local entry = itemTable[item:getId()]
    if not entry then
        return true
    end

    doTargetCombat(0, creature, COMBAT_FIREDAMAGE, -entry.initDamage, -entry.initDamage, CONST_ME_HITBYFIRE)

end

for id, _ in pairs(itemTable) do
    event:id(id)
end

event:register()