a = 2
local b = 10
fn do_sth(){
    
    local fn hello( h, d )
    {
        print("parameter: " + h + d)
        print("hello yuan script!")
    }

    local fn say()
    {
        hello(100, 20)
    }
    
    say()

    c = 10
    for (b = 0; b < 10; b++) {
        print("cur: " + b)
        if (b > 5) {
            break
        }
    }
}

fun = do_sth

fun()


