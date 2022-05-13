local tb = {}


local fn tb:say_hello()
{
    print("module say hello")
}

local fn tb:say_hello_world()
{
    print("hello world in module.y")
}

print(gArgs[0])

return tb
