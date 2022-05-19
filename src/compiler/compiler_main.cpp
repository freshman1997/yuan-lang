#include <direct.h>

#include "yuan.h"
#include "utils.h"

int main(int argc, char **argv) 
{
    char* cwd = _getcwd(NULL, 0);
    if (!cwd) {
        cout << "unexpected!" << endl;
        return 1;
    }
    string _cwd = cwd;
    setcwd(_cwd);

    for (int i = 1; i < argc; ++i) {
        if (is_valid_path((getcwd() + "/" + argv[i]).c_str()) && is_file((getcwd() + "/" + argv[i]).c_str())) {
            compile((getcwd() + "/" + string(argv[i])).c_str());
        }
        else {
            m_mkdir((getcwd() + "/" + string(argv[i]) + "/target").c_str());
            setcwd(getcwd() + "/" + string(argv[i]) + "/target");
            compile_dir((string(cwd) + "/" + argv[i]).c_str());
        }
    }
    return 0;
}