



// 测试文件操作
start = os.now()
ls = os.listfile("D:/job/logic/module/activity", false, nil, true)

for (i = 0; i < #ls; ++i) {
    print(ls[i])
}

end = os.now()
print("spend time: " + (end - start))

fd = os.open("d:/1.txt", os.input)
print("fd: " + fd)
if (!fd) {
    print("no such file or directory!")
    return
}

line = os.readline(fd)
while (line)
{
    print(line)
    line = os.readline(fd)
}

os.close(fd)


local fn quicksort(arr, l, r) {
    if (l < r) {
        local key = arr[(l + r) / 2]
        i = l, j = r
        while (i <= j) {
            
            while (arr[i] < key) {
                i++
            }
            
            while (arr[j] > key) {
                j--
            }
            if (i <= j) {
                // 这里左边要是表达式才能正确修改数组的值，否则只是拷贝，不会被修改
                local t = arr[i]
                arr[i] = arr[j]
                arr[j] = t
                i++
                j--
            }
        }


        if (l < j) {
            quicksort(arr, l, j)
        }

        if (r > j) {
            quicksort(arr, i, r)
        }
    }
}

local arr1 = [3, 1, 4, 2, 6, 5]

quicksort(arr1, 0, #arr1 - 1)

str = "["
for (i = 0; i < #arr1; ++i) {
    str = str + arr1[i]
    if (i < #arr1 - 1) {
        str = str + ", "
    }
}
str = str + "]"
print(str)


local fn fib(n) {
    if (n == 0) {
        return 1
    }
    if (n == 1) {
        return 1
    }
    return fib(n - 1) + fib(n - 2)
}
print(fib)

print("fib result: " + fib(5))

fn test_args(...) {
    print("size: " + #args)
    for (i = 0; i < #args; ++i) {
        print("args: " + args[i])
    }
    print(gArgs)
}
test_args(1, 2, 3)

local mtb = require("module", "hello args in module")

mtb.say_hello()
mtb.say_hello_world()


local tb = {"name": fn(name) {
    print("welcome " + name)
},100 : 20}

// 这里不管加不加 local 默认都是 local
local fn tb:func()
{
    print("module")
}

print("tb 100 :" + tb[100])

tb["name"]("tomcat")

tb.func()

// 这里的前置++会结合到上一条表达式里面了，所以需要放成后置
tb[100]++



local test_tba = {100: 20}
++test_tba[100]
print(test_tba[100])

cal = 100 + 18 + 200 / 2 * 2 + 20 / 2
print("cal: " + cal)




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


// require 只执行一遍文件，可以所有的全局变量，return的表可以当作模块使用，可以return任何类型的数据，执行到return就会返回掉，不执行下面的指令 
// do_file 执行文件
// local aa = require("文件", ..., args);   aa.doas() doas 会去 aa 变量里面找key



return tb

