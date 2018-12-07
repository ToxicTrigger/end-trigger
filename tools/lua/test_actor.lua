-- t_actor : component name matching this lua file
-- t_move ( actor ,  0, 0, 0 )
-- t_position( actor , 0, 0, 0)
-- t_set_scale( actor , 0, 0, 0)
-- t_rotation( actor , 0, 0, 0 )
-- t_set_rotation( actor , 0, 0, 0)
-- t_kill( actor ) 
-- t_new_actor( "name" ) -> return "name"
-- t_print("message")



function init() --init lua
	actor = t_new_actor("tt")
	msg = string.format("%s are mewoo", actor)
	t_print(msg)
end

function update(delta) --Update every frame
	t_rotation("tt", 0.2 * delta, -0.2 * delta, 1 * delta)
end

function destroy() --When Destroy
end
