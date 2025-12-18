local moveevent = MoveEvent()

local condition = Condition(CONDITION_POISON)
condition:setParameter(CONDITION_PARAM_CYCLE, 1000)
condition:setParameter(CONDITION_PARAM_COUNT, 3)
condition:setParameter(CONDITION_PARAM_MAX_COUNT, 3)

function moveevent.onStepIn(creature, item, position, fromPosition)
	if creature:isPlayer() then
		creature:addCondition(condition)
		creature:getPlayer():setStorageValue(270, 1)
		Game.sendMagicEffect({x = 33362, y = 32811, z = 14}, 9)
	end
end

moveevent:aid(3015)
moveevent:register()