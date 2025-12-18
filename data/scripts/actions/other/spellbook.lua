local spellbook = Action()

function spellbook.onUse(player, item, fromPosition, target, toPosition, isHotkey)
	local text = ""
	local tml = {}

	for _, spell in ipairs(player:getInstantSpells()) do
		if spell.level >= 0 or spell.mlevel >= 0 then
			if spell.manapercent > 0 then
				spell.mana = spell.manapercent .. "%"
			end
			if spell.mlevel >= 0 then
				tml[#tml + 1] = spell
			end
		end
	end

	table.sort(
		tml,
		function(a, b)
			return a.mlevel < b.mlevel
		end
	)
	local prevmLevel = -1
	for i, spell in ipairs(tml) do
		local line = ""
		if prevmLevel ~= spell.mlevel then
			if i ~= 1 then
				line = "\n"
			end
			line = line .. spell.mlevel .. ". Level Spells\n"
			prevmLevel = spell.mlevel
		end
		text = text .. line .. "  " .. spell.words .. " - " .. spell.name .. " : " .. spell.mana .. "\n"
	end

	player:showTextDialog(item:getId(), text)
	return true
end

spellbook:id(2175)
spellbook:register()