local moveevent = MoveEvent()

function moveevent.onStepIn(creature, item, position, fromPosition)
	if Game.isItemInPosition({x = 32876, y = 32584, z = 10},4997) and Game.isItemInPosition ({x = 32823, y = 32525, z = 10},4997) and Game.isItemInPosition ({x = 32792, y = 32527, z = 10},4997) and Game.isItemInPosition ({x = 32744, y = 32586, z = 10}, 4997) then 
		doRelocate(item:getPosition(),{x = 32884, y = 32632, z = 11})
		item:getPosition():sendMagicEffect(21)
		Game.sendMagicEffect({x = 32884, y = 32632, z = 11}, 21)
	else
		doRelocate(item:getPosition(),{x = 32853, y = 32543, z = 10})
		item:getPosition():sendMagicEffect(21)
		Game.sendMagicEffect({x = 32853, y = 32543, z = 10}, 21)
		creature:say("Spectral guardians ward you off.", TALKTYPE_MONSTER_SAY)
	end
end

moveevent:aid(3085)
moveevent:register()

local moveevent = MoveEvent()

function moveevent.onAddItem(item, tileitem, position)
	doRelocate(item:getPosition(),{x = 32853, y = 32543, z = 10})
	item:getPosition():sendMagicEffect(21)
	Game.sendMagicEffect({x = 32853, y = 32543, z = 10}, 21)
	Game.sendMagicEffect(position, 21)
	local tile = Tile({x = 32853, y = 32543, z = 10})
	if tile and tile:getTopCreature() then
		tile:getTopCreature():say("Spectral guardians ward you off.", TALKTYPE_MONSTER_SAY)
	end
end

moveevent:aid(3085)
moveevent:tileItem(true)
moveevent:register()