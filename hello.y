


local tb = {"name": fn(name) {
    print("welcome " + name)
},100 : 20}

print("tb 100 :" + tb[100])

tb["name"]("tomcat")

// 这里的前置++会结合到上一条表达式里面了，所以需要放成后置
tb[100]++



local test_tba = {100: 20}
++test_tba[100]
print(test_tba[100])

cal = 100 + 18 + 200 / 2 * 2 + 20 / 2
print("cal: " + cal)

fn test_args(...) {
    print("size: " + #args)
    for (i = 0; i < #args; ++i) {
        print(args[i])
    }
}

test_args(1, 2, 3)


test = 10
test = test >> 100
print(test)

local a = 100
-100

for (i = 0; i < 5; ++i) {
    print("cur: " + i)
}

local fn do_sth()
{
    local a1 = 90
    a = 50
    if (a == 50 && a1 == 90 && 1 > 0) {
        print("hello")
    } 
    else if (a == 100) {
        print("80")
    }
    else if (a > 100) {
        print("a1 = " + a1 + ", a = " + a)
    }
    

    local fn hello(...)
    {
        return "你好世界!!!" + "hello world"
    }

    return hello
}

fun = do_sth()



local k = nil, v = nil
for (k, v in tb) {
    print("key: " + k)
}






print("tb[100] :" + tb[100])

arr = [1, 2, 3, 4, 5]

for (i = 0; i < #arr; ++i) {
    print("normal for: " + arr[i])
}


for (i in arr) {
    print("for in: " + i)
}





