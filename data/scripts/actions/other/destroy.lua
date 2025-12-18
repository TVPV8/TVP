local items = {
	{fromid=2376, toid=2404},
	{fromid=2406, toid=2415},
	{fromid=2417, toid=2419},
	{fromid=2421, toid=2441},
	{fromid=2443, toid=2453},
}

local action = Action()

function action.onUse(player, item, fromPosition, target, toPosition)
	return destroyItem(player, target, toPosition)
end

for _, data in ipairs(items) do
	for i=data.fromid, data.toid, 1 do
		action:id(i)
	end
end

action:register()
