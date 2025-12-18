local ALLOW_AUTO_STACK = true
local config = {
	[ITEM_GOLD_COIN] = {changeTo = ITEM_PLATINUM_COIN},
	[ITEM_PLATINUM_COIN] = {changeBack = ITEM_GOLD_COIN, changeTo = ITEM_CRYSTAL_COIN},
	[ITEM_CRYSTAL_COIN] = {changeBack = ITEM_PLATINUM_COIN},
}

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	local coin = config[item:getId()]
	if coin.changeTo and item.type == 100 then
		item:remove()
		if not ALLOW_AUTO_STACK then
			player:addItem(coin.changeTo, 1)
		else
			local gold = Game.createItem(coin.changeTo, 1)
			if player:addItemEx(gold) ~= RETURNVALUE_NOERROR then
				Tile(player:getPosition()):addItemEx(gold)
			end
		end
	elseif coin.changeBack then
		item:remove(1)

		if not ALLOW_AUTO_STACK then
			player:addItem(coin.changeBack, 100)
		else
			local gold = Game.createItem(coin.changeBack, 100)
			if player:addItemEx(gold) ~= RETURNVALUE_NOERROR then
				Tile(player:getPosition()):addItemEx(gold)
			end
		end
	else
		return false
	end
	return true
end

action:id(ITEM_GOLD_COIN)
action:id(ITEM_PLATINUM_COIN)
action:id(ITEM_CRYSTAL_COIN)
action:register()
