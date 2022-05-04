a = 2
local b = 10
fn do_sth(){
    c = 10
    for (b = 0; b < 10; b++) {
        print("cur: " + b)
        if (b > 5) {
            break
        }
    }

    local fn hello()
    {
        print("hello yuan script!")
    }

    local fn say()
    {
        hello()
    }
    
    say()
}

fun = do_sth

fun()


