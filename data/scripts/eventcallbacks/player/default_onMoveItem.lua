local ec = EventCallback

ec.onMoveItem = function(self, item, count, fromPosition, toPosition, fromCylinder, toCylinder)
	if toPosition.x ~= CONTAINER_POSITION then
		return true
	end

	return true
end

ec:register()
