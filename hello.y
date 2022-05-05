local a = 100

fn do_sth()
{
    if (a > 100) {
        print("hello")
    } 
    else if (a != 100) {
        print("80")
    }
    else if (a == 50) {
        a = 50
    }
    

    local fn hello()
    {
        print("hello world!!!")
    }

    return hello
}

fun = do_sth()

fun()
