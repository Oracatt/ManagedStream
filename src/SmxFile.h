#ifndef SMXFILE_H
#define SMXFILE_H

#include "ManagedStream.h"
#include <stack>

struct SourceInfo;

struct SourceDirectory
{
	shared_ptr<SourceDirectory> nextdir = 0;
	shared_ptr<SourceDirectory> fatherdir = 0;
	shared_ptr<SourceDirectory> firstsondir = 0;
	shared_ptr<SourceInfo> firstsonfile = 0;
	short namelength = 0;
	string dirname = "";
};

struct SourceInfo
{
	bool zipped;
	short namelength = 0;
	string filename = ""; //º¬smxÂ·¾¶
	LONG64 bytesunzip;
	LONG64 byteszipped;
	LONG64 startpos;
	short year;
	BYTE month;
	BYTE day;
	shared_ptr<SourceInfo> nextfile = 0;
	shared_ptr<SourceDirectory> fatherdir = 0;
};

typedef shared_ptr<SourceDirectory> LPSRCDIR;
typedef shared_ptr<SourceInfo> LPSRCINFO;

typedef struct
{
	FileStream* fs = 0;
	LPSRCDIR maindir = 0;
	stack<LPSRCDIR> openfilestack;
}SmxFile, * LPSMXFILE;

LPSMXFILE OpenSmxFile(LPCSTR filename);
void CloseSmxFile(LPSMXFILE file);
LPSRCINFO LoadSourceFileByPath(LPSMXFILE file, string path);
void ReadSourceFile(LPSMXFILE file, LPSRCINFO src, void* buffer);
#endif // !SMXFILE_H
