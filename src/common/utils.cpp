#include "utils.h"
#include "lex.h"
#include "types.h"

#include <direct.h>
#include <io.h>
#include <fstream>
#include <cstdarg>
#include <Windows.h>

static void verror_at(const char *filename, char *input, int line_no, char *loc, char *fmt, va_list ap) {
    // Find a line containing `loc`.
	  line_no += 1;
    char *line = loc;
    while (input < line && line[-1] != '\n')
      line--;

    char *end = loc;
    while (*end && *end != '\n')
      end++;

    // Print out the line.
    int indent = fprintf(stderr, "%s:%d ", filename, line_no);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // Show the error message.
    int pos = int((loc - line)) + indent;

    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void error_tok(const Token &tok, const char *filename, char * content, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verror_at(filename, content, tok.row, tok.from, fmt, ap);
    exit(1);
}

bool m_mkdir(const char *dir) 
{
    return _mkdir(dir) == 0;
}

static string get_path_dir(string path)
{
    size_t idx = path.find_last_of('\\');
    if (idx == string::npos) idx = path.find_last_of('/');
    if (idx != string::npos) {
        path.erase(idx);
        return path;
    }
    return "";
}

bool m_mkdirs(const char *dir)
{
    if (_access(dir, 00) == 0) return false;
    string path = dir;
    vector<string> dirs;
    dirs.push_back(path);
    while (true) {
        path = get_path_dir(path);
        if (path.empty() || _access(path.c_str(), 00) == 0) break;
        dirs.push_back(path);
    }
    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
        _mkdir(it->c_str());
    }
    return true;
}

bool is_valid_path(const char *path)
{
    return _access(path, 00) == 0;
}

bool is_file(const char *path)
{
    if (_access(path, 00) == 0) {
        WIN32_FIND_DATAA FindFileData;
        FindFirstFileA(path, &FindFileData);
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    cout << "utils unexpected!" << endl;
    exit(0);
    return false;
}

int get_prefix_index(const char *s1, const char *s2)
{
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) break;
        i++;
    }
    return i;
}

static string _cwd;

void to_cwd(const char *exe)
{
    int len = strlen(exe) - 1;
    while (!(exe[len] == '/' || exe[len] == '\\')){
       --len;
    }
    for (int i = 0; i < len; i++) {
        _cwd.push_back(exe[i]);
    }
}

void setcwd(const string &str)
{
    _cwd = str;
}

string getcwd()
{
    return _cwd;
}

static void find(const char* lpPath, std::vector<string> &fileList, const char *ext, bool r, bool full) 
{ 
    char szFind[MAX_PATH]; 
    WIN32_FIND_DATA FindFileData; 

    strcpy(szFind, lpPath); 
    strcat(szFind,"\\*.*"); 
    
    HANDLE hFind=::FindFirstFile(szFind,&FindFileData); 
    if(INVALID_HANDLE_VALUE == hFind)  return; 
    
    while(true) 
    { 
        if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
        { 
            if(FindFileData.cFileName[0] != '.') 
            { 
                if (r) {
                    char szFile[MAX_PATH]; 
                    strcpy(szFile, lpPath); 
                    strcat(szFile, "\\"); 
                    strcat(szFile, (char* )(FindFileData.cFileName)); 
                    find(szFile, fileList, ext, r, full); 
                }
            } 
        } 
        else
        { 
            if (full) {
                fileList.push_back(lpPath);
                fileList.back().append("\\").append(FindFileData.cFileName); 
            }
            else fileList.push_back(FindFileData.cFileName); 
            if (ext) {
                int i = strlen(ext) - 1, j = fileList.back().size() - 1, sz = i + 1;
                if (i >= j) fileList.pop_back();
                else {
                    while (i) {
                        if (ext[i] != fileList.back()[j]) break;
                        --i;
                        --j;
                    }
                    if (i != 0) fileList.pop_back();
                }
            }
        } 
        if (!FindNextFile(hFind, &FindFileData))  break; 
    } 
    FindClose(hFind); 
} 

vector<string> * get_dir_files(const char *dir, bool r, bool full)
{
    vector<string> *ret = new vector<string>;
    find(dir, *ret, NULL, r, full);
    return ret;
}

vector<string> * get_dir_files(const char *dir, const char *ext, bool r, bool full)
{
    vector<string> *ret = new vector<string>;
    find(dir, *ret, ext, r, full);
    return ret;
}

