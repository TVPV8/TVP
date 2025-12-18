-- This script converts a switch/lever with the action ID 2081 into a lever that cannot be moved
-- that will turn back into its original ID after decaying (puzzle related)

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	if item:getId() == 1945 then 
		item:transform(4384, 1)
		item:decay()
	else
		player:sendTextMessage(MESSAGE_STATUS_SMALL, "It doesn't move.")
	end
	return true
end

action:aid(actionIds.puzzleSwitch)
action:register()