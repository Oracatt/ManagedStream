#include "Path.h"

string Path::Combine(string* paths, int count)
{
	string ret = paths[0];
	for (int i = 1; i < count; i++)
		ret = Combine(ret, paths[i]);
	return ret;
}

string Path::Combine(string path1, string path2)
{
	size_t startpos;
	while ((startpos = path1.find("\\\\")) != string::npos)
		path1.replace(startpos, 1, "");
	while ((startpos = path2.find("\\\\")) != string::npos)
		path2.replace(startpos, 1, "");
	if (path1.length() == 0)
		return path2;
	if (path2.length() == 0)
		return path1;
	if (path1[path1.length() - 1] != '\\')
		path1.append("\\");
	if (path2[0] == '\\')
		path1.append(path2.c_str() + 1);
	else
		path1.append(path2.c_str());
	return path1;
}

string Path::GetDirectoryName(string path)
{
	string ret = "";
	ret.reserve(path.length() + 1);
	char* buffer;
	char* pathcstr = new char[path.length() + 1];
	char* part = 0;
	memcpy(pathcstr, path.c_str(), path.length());
	pathcstr[path.length()] = 0;
	part = strtok_s(pathcstr, "\\", &buffer);
	char* oldpart = 0;
	bool head = true;
	while (part)
	{
		if (oldpart)
		{
			if (!head)
				ret.append("\\");
			else
				head = false;
			ret.append(oldpart);
		}
		oldpart = part;
		part = strtok_s(0, "\\", &buffer);
	}
	delete[] pathcstr;
	return ret;
}

string Path::GetExtension(string path)
{
	string ret = ".";
	char* buffer;
	char* pathcstr = new char[path.length() + 1];
	char* part = 0;
	memcpy(pathcstr, path.c_str(), path.length());
	pathcstr[path.length()] = 0;
	part = strtok_s(pathcstr, ".", &buffer);
	char* oldpart = 0;
	while (part)
	{
		oldpart = part;
		part = strtok_s(0, ".", &buffer);
	}
	if (oldpart)
	{
		ret.append(oldpart);
		delete[] pathcstr;
		return ret;
	}
	else
	{
		delete[] pathcstr;
		return "";
	}
}

string Path::GetFileName(string path)
{
	string ret = "";
	char* buffer;
	char* pathcstr = new char[path.length() + 1];
	char* part = 0;
	memcpy(pathcstr, path.c_str(), path.length());
	pathcstr[path.length()] = 0;
	part = strtok_s(pathcstr, "\\", &buffer);
	char* oldpart = 0;
	while (part)
	{
		oldpart = part;
		part = strtok_s(0, "\\", &buffer);
	}
	if (oldpart)
		ret = oldpart;
	delete[] pathcstr;
	return ret;
}

string Path::GetFileNameWithoutExtension(string path)
{
	path = GetFileName(path);
	string ret = "";
	ret.reserve(path.length() + 1);
	char* buffer;
	char* pathcstr = new char[path.length() + 1];
	char* part = 0;
	memcpy(pathcstr, path.c_str(), path.length());
	pathcstr[path.length()] = 0;
	part = strtok_s(pathcstr, ".", &buffer);
	char* oldpart = 0;
	bool head = true;
	while (part)
	{
		if (oldpart)
		{
			if (!head)
				ret.append(".");
			else
				head = false;
			ret.append(oldpart);
		}
		oldpart = part;
		part = strtok_s(0, ".", &buffer);
	}
	delete[] pathcstr;
	return ret;
}