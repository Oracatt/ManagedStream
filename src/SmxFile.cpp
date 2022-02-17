#include "SmxFile.h"

void UnicodeToAnsi(BYTE* buffer)
{
    int ansiLen = WideCharToMultiByte(CP_ACP, NULL, (LPWSTR)buffer, wcslen((LPWSTR)buffer), NULL, 0, NULL, NULL);
    char* szAnsi = new char[ansiLen + 1];
    WideCharToMultiByte(CP_ACP, NULL, (LPWSTR)buffer, wcslen((LPWSTR)buffer), szAnsi, ansiLen, NULL, NULL);
    szAnsi[ansiLen] = '\0';
    memcpy(buffer, szAnsi, ansiLen + 1);
    delete[] szAnsi;
}

LPSRCDIR FindDirectoryByName(LPSRCDIR father, string filename)
{
    LPSRCDIR temp;
    for (temp = father->firstsondir; temp != NULL; temp = temp->nextdir)
    {
        if (!_strcmpi(Path::GetFileName(temp->dirname).c_str(), filename.c_str()))
            return temp;
    }
    return NULL;
}

LPSRCDIR FindDirectoryByName(LPSMXFILE file, string path)
{
    char* buffer;
    char* pathcstr = new char[path.length() + 1];
    char* part = 0;
    memcpy(pathcstr, path.c_str(), path.length());
    pathcstr[path.length()] = 0;
    part = strtok_s(pathcstr, "\\", &buffer);
    part = strtok_s(NULL, "\\", &buffer);
    LPSRCDIR sd = file->maindir;
    while (part)
    {
        sd = FindDirectoryByName(sd, part);
        part = strtok_s(NULL, "\\", &buffer);
    }
    delete[] pathcstr;
    return sd;
}

LPSRCINFO SearchDirectory(LPSRCDIR dir, string filename)
{
    if (!dir)
        return NULL;
    LPSRCINFO temp;
    for (temp = dir->firstsonfile; temp != NULL; temp = temp->nextfile)
    {
        if (!_strcmpi(Path::GetFileName(temp->filename).c_str(), filename.c_str()))
            return temp;
    }
    return NULL;
}

void SetDirectoryRelation(LPSRCDIR father, LPSRCDIR son)
{
    son->fatherdir = father;
    LPSRCDIR infotemp = father->firstsondir;
    if (infotemp == NULL)
    {
        father->firstsondir = son;
    }
    else
    {
        while (true)
        {
            if (infotemp->nextdir == NULL)
                break;
            infotemp = infotemp->nextdir;
        }
        infotemp->nextdir = son;
    }
}

void SetFileRelation(LPSRCDIR father, LPSRCINFO son)
{
    son->fatherdir = father;
    LPSRCINFO infotemp = father->firstsonfile;
    if (infotemp == NULL)
    {
        father->firstsonfile = son;
    }
    else
    {
        while (true)
        {
            if (infotemp->nextfile == NULL)
                break;
            infotemp = infotemp->nextfile;
        }
        infotemp->nextfile = son;
    }
}

LPSRCDIR FindDirectoryInStack(LPSMXFILE file, string fullname)
{
    if (!file->openfilestack.empty())
    {
        for (int i = file->openfilestack.size() - 1; i >= 0; i--)
        {
            LPSRCDIR dir = file->openfilestack.top();
            if (!_strcmpi(dir->dirname.c_str(), fullname.c_str()))
                return dir;
            file->openfilestack.pop();
        }
    }
    LPSRCDIR sd = FindDirectoryByName(file, fullname);
    if (sd != NULL)
        file->openfilestack.push(sd);
    return sd;
}

void DirectoryToSource(LPSMXFILE file, LPSRCDIR dirinfo)
{
    LPSRCDIR sd = FindDirectoryInStack(file, Path::GetDirectoryName(dirinfo->dirname));
    if (sd == NULL)
        sd = file->maindir;
    LPSRCDIR dir(new SourceDirectory);
    dir->dirname = dirinfo->dirname;
    dir->namelength = (short)dir->dirname.length();
    SetDirectoryRelation(sd, dir);
    file->openfilestack.push(dir);
}

void FileToSource(LPSMXFILE file, LPSRCINFO fileinfo)
{
    LPSRCDIR sd = FindDirectoryInStack(file, Path::GetDirectoryName(fileinfo->filename));
    if (sd == NULL)
        sd = file->maindir;
    SetFileRelation(sd, fileinfo);
}

void ReleaseSourceDir(LPSRCDIR dir)
{
    dir->fatherdir = 0;
    LPSRCINFO tempinfo;
    for (tempinfo = dir->firstsonfile; tempinfo != NULL; tempinfo = tempinfo->nextfile)
    {
        tempinfo->fatherdir = 0;
    }
    dir->firstsonfile = 0;
    LPSRCDIR temp;
    for (temp = dir->firstsondir; temp != NULL; temp = temp->nextdir)
    {
        ReleaseSourceDir(temp);
    }
    dir->firstsondir = 0;
}

LPSMXFILE OpenSmxFile(LPCSTR filename)
{
    LPSMXFILE file = new SmxFile;
    file->maindir = LPSRCDIR(new SourceDirectory);
    file->fs = new FileStream(filename, FileStream::Mode::Open);
    BYTE buffer[4096];
    while (file->fs->GetPosition() < file->fs->GetLength())
    {
        file->fs->Read(buffer, 0, 1);
        if (buffer[0] == 1) //ÊÇÎÄ¼þ¼Ð
        {
            LPSRCDIR dirinfo(new SourceDirectory);
            file->fs->Read(buffer, 0, 2);
            dirinfo->namelength = *((short*)buffer);
            file->fs->Read(buffer, 0, dirinfo->namelength);
            buffer[dirinfo->namelength] = 0;
            buffer[dirinfo->namelength + 1] = 0;
            UnicodeToAnsi(buffer);
            dirinfo->dirname = (LPSTR)buffer;
            DirectoryToSource(file, dirinfo);
        }
        else
        {
            LPSRCINFO fileinfo(new SourceInfo);
            fileinfo->zipped = (buffer[0] == 0);
            file->fs->Read(buffer, 0, 2);
            fileinfo->namelength = *((short*)buffer);
            file->fs->Read(buffer, 0, fileinfo->namelength);
            buffer[fileinfo->namelength] = 0;
            buffer[fileinfo->namelength + 1] = 0;
            UnicodeToAnsi(buffer);
            fileinfo->filename = (LPSTR)buffer;
            file->fs->Read(buffer, 0, 8);
            fileinfo->bytesunzip = *((LONG64*)buffer);
            file->fs->Read(buffer, 0, 8);
            fileinfo->byteszipped = *((LONG64*)buffer);
            file->fs->Read(buffer, 0, 8);
            fileinfo->startpos = *((LONG64*)buffer);
            file->fs->Read(buffer, 0, 2);
            fileinfo->year = *((short*)buffer);
            file->fs->Read(buffer, 0, 1);
            fileinfo->month = buffer[0];
            file->fs->Read(buffer, 0, 1);
            fileinfo->day = buffer[0];
            file->fs->Seek(fileinfo->byteszipped, Stream::SeekOrigin::Current);
            FileToSource(file, fileinfo);
        }
    }
    return file;
}

void CloseSmxFile(LPSMXFILE file)
{
    ReleaseSourceDir(file->maindir);
    file->maindir = 0;
    file->fs->Close();
    delete file->fs;
    delete file;
}

LPSRCINFO LoadSourceFileByPath(LPSMXFILE file, string path)
{
    char* buffer;
    char* pathcstr = new char[path.length() + 1];
    char* part = 0;
    memcpy(pathcstr, path.c_str(), path.length());
    pathcstr[path.length()] = 0;
    part = strtok_s(pathcstr, "\\", &buffer);
    char* oldpart = 0;
    LPSRCDIR sd = file->maindir;
    while (part)
    {
        if (oldpart)
        {
            sd = FindDirectoryByName(sd, oldpart);
        }
        oldpart = part;
        part = strtok_s(NULL, "\\", &buffer);
    }
    LPSRCINFO target = SearchDirectory(sd, oldpart);
    delete[] pathcstr;
    return target;
}

void ReadSourceFile(LPSMXFILE file, LPSRCINFO src, void* buffer)
{
    file->fs->SetPosition(src->startpos);
    if (src->zipped)
    {
        GZipStream gz(file->fs, GZipStream::CompressionMode::Decompress);
        gz.Read((BYTE*)buffer, 0, src->bytesunzip);
        gz.Close();
    }
    else
        file->fs->Read(buffer, 0, src->bytesunzip);
}