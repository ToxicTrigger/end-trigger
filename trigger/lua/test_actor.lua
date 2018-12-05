function init()
    t_print("Lua Test Component inited!")
end

count = 0
function update(delta)
    if count == 12 then
    elseif count == 11 then
        t_new_actor("Trigger Object")
        count = count + 1
    elseif count == 10 then
        t_print("count is 10! update will be stop!")
        count = count + 1
    else
        count = count + 1
        t_print(count)
    end
end

function destroy()
    t_print("Trigger Object destroyed")
end