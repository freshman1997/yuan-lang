
a = false

print(100)

arr = [1, 2, 3]

 local tb = {12 : 12, "name": "tomcat"}
 tb[12]++



a = 2
local b = 10
fn do_sth(){
    
    local fn hello( h, d )
    {
        print("parameter: " + h + d)
        print("hello yuan script!")
    }

    local fn get()
    {
        return 200 * 2 + 300 * 3 / 2
    }

    local fn say()
    {
        hello(100, 20)
        return 10 + get()
    }
    say()
    c = 10
    for (b = 0; b < 10; b++) {
        print("cur: " + b)
        a = a + say()
        print("now a: " + a)
        if (b > 5) {
            break
        }
    }
    while (a--){
        print("while a: " + a)
    }
}

fun = do_sth

fun()


