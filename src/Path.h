#ifndef PATH_H
#define PATH_H

#include <iostream>

using namespace std;

class Path
{
public:
    static string Combine(string* paths, int count);
    static string Combine(string path1, string path2);
    static string GetDirectoryName(string path);
    static string GetExtension(string path);
    static string GetFileName(string path);
    static string GetFileNameWithoutExtension(string path);
};

#endif