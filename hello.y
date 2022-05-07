

fn test_args(...) {
    print("size: " + #args)
    for (i = 0; i < #args; ++i) {
        print(args[i])
    }
}

test_args(1, 2, 3)


test = 10
test >>= 100
print(test)

local a = 100
-100

for (i = 0; i < 5; ++i) {
    print("cur: " + i)
}

local fn do_sth()
{
    if (a > 100) {
        print("hello")
    } 
    else if (a == 100) {
        print("80")
    }
    else if (a == 50) {
        a = 50
    }
    

    local fn hello(...)
    {
        return "你好世界!!!" + "hello world"
    }

    return hello
}

fun = do_sth()



local tb = {"name": fn(name) {
    print("welcome " + name)
},100 : 20}

tb["name"](fun())


arr = [1, 2, "hello world", tb, fn(){
    print("array inner method")
}]

arr[4]()




