-- t_actor : component name matching this lua file
-- t_move ( actor , vec( 0, 0, 0) )
-- t_rotation( actor , vec(0, 0, 0) )
-- t_kill( actor ) 
-- t_new_actor( "name" )
-- t_print("message")

function init() --init lua
end

function update(delta) --Update every frame
	t_rotation("actor0", 1 * delta, 0, 0)
end

function destroy() --When Destroy
end
