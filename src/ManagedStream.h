#ifndef MANAGEDSTREAM_H
#define MANAGEDSTREAM_H

#include <iostream>
#include "Path.h"
#include <basetsd.h> 
#include <wtypes.h>

using namespace std;

#define SUCCEED_OPERATE 0
#define ARGUMENT_ERROR -1
#define ACCESS_ERROR -2
#define NOTSUPPORT_ERROR -3
#define IO_ERROR -4
#define STREAMCLOSED_ERROR -5
#define ENDOFSTREAM_ERROR -6
#define OUTOFMEMORY_ERROR -7
#define NOTIMPLEMENTED_ERROR -8

#define GZIPBUFFERSIZE (1<<12)

class Stream
{
public:
	enum class SeekOrigin
	{
		Begin,
		Current,
		End
	};
	Stream();
	virtual LRESULT Read(LPVOID buffer, int offset, int count) = 0;
	virtual LRESULT Write(LPVOID buffer, int offset, int count) = 0;
	virtual void Flush() = 0;
	virtual void Close() = 0;
	virtual LONG64 GetLength() = 0;
	virtual LRESULT SetLength(LONG64 len) = 0;
	virtual LONG64 GetPosition() = 0;
	virtual LRESULT SetPosition(LONG64 pos) = 0;
	virtual LRESULT Seek(LONG64 offset, SeekOrigin origin);
	virtual bool CanRead();
	virtual bool CanWrite();
	virtual bool CanSeek();
protected:
	LONG64 length;
	LONG64 position;
	bool canread;
	bool canwrite;
	bool canseek;
	bool closed;
};

class FileStream :public Stream
{
public:
	enum class Mode
	{
		Create,
		Open,
		OpenOrCreate,
		Truncate,
		Append
	};
	FileStream(LPCSTR path, Mode mode);
	~FileStream();
	LRESULT Read(LPVOID buffer, int offset, int count) override;
	LRESULT Write(LPVOID buffer, int offset, int count) override;
	void Flush() override;
	void Close() override;
	LONG64 GetLength() override;
	LRESULT SetLength(LONG64 len) override;
	LONG64 GetPosition() override;
	LRESULT SetPosition(LONG64 pos) override;
private:
	FILE* fp;
};

class MemoryStream :public Stream
{
public:
	MemoryStream();
	MemoryStream(LONG64 capacity);
	MemoryStream(LPVOID buffer, LONG64 size);
	~MemoryStream();
	LRESULT Read(LPVOID buffer, int offset, int count) override;
	LRESULT Write(LPVOID buffer, int offset, int count) override;
	void Flush() override;
	void Close() override;
	LONG64 GetLength() override;
	LRESULT SetLength(LONG64 len) override;
	LONG64 GetPosition() override;
	LRESULT SetPosition(LONG64 pos) override;
	LONG64 GetCapacity();
	LRESULT SetCapacity(LONG64 cap);
	LPVOID GetBuffer();
	LRESULT ToArray(LPVOID buffer, LONG64 buffersize, LONG64& outputcount);
protected:
	BYTE* _ptr;
	LONG64 capacity;
	LRESULT CapacityAutoGrow();
};

class GZipStream :public Stream
{
public:
	enum class CompressionMode
	{
		Decompress,
		Compress
	};
	GZipStream(Stream* stream, CompressionMode mode);
	~GZipStream();
	LRESULT Read(LPVOID buffer, int offset, int count) override;
	LRESULT Write(LPVOID buffer, int offset, int count) override;
	void Flush() override;
	void Close() override;
	LONG64 GetLength() override;
	LRESULT SetLength(LONG64 len) override;
	LONG64 GetPosition() override;
	LRESULT SetPosition(LONG64 pos) override;
private:
	BYTE outbuffer[GZIPBUFFERSIZE];
	int remain;
	Stream* inputstream;
	void* cstream;
};

#endif