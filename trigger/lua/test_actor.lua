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
	t_set_scale("actor0", 1, 1, 1)
	t_set_rotation("actor0", 0, 0, 0)
	t_print("init done")
end

function update(delta) --Update every frame
end

function destroy() --When Destroy
end
