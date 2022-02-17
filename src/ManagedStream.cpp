#include "ManagedStream.h"
#include <io.h>
#include <wtypes.h>
#include "zlib.h"

Stream::Stream()
{
	length = 0;
	position = 0;
	canread = false;
	canwrite = false;
	canseek = false;
	closed = false;
}

LRESULT Stream::Read(LPVOID buffer, int offset, int count) { return NOTIMPLEMENTED_ERROR; };
LRESULT Stream::Write(LPVOID buffer, int offset, int count) { return NOTIMPLEMENTED_ERROR; };
void Stream::Flush() { };
void Stream::Close() { };
LONG64 Stream::GetLength() { return NOTIMPLEMENTED_ERROR; };
LRESULT Stream::SetLength(LONG64 len) { return NOTIMPLEMENTED_ERROR; };
LONG64 Stream::GetPosition() { return NOTIMPLEMENTED_ERROR; };
LRESULT Stream::SetPosition(LONG64 pos) { return NOTIMPLEMENTED_ERROR; };

LRESULT Stream::Seek(LONG64 offset, SeekOrigin origin)
{
	if (canseek)
	{
		switch (origin)
		{
		case SeekOrigin::Begin:
			return SetPosition(offset);
		case SeekOrigin::Current:
			return SetPosition(offset + position);
		case SeekOrigin::End:
			return SetPosition(offset + length);
		}
		return ARGUMENT_ERROR;
	}
	else
		return NOTSUPPORT_ERROR;
}

bool Stream::CanRead()
{
	return canread;
}

bool Stream::CanWrite()
{
	return canwrite;
}

bool Stream::CanSeek()
{
	return canseek;
}

FileStream::FileStream(LPCSTR path, Mode mode)
{
	switch (mode)
	{
	case Mode::Create:
		fp = fopen(path, "wb+");
		canread = true;
		canwrite = true;
		canseek = true;
		position = _ftelli64(fp);
		length = _filelengthi64(_fileno(fp));
		break;
	case Mode::Open:
		fp = fopen(path, "rb+");
		canread = true;
		canwrite = true;
		canseek = true;
		position = _ftelli64(fp);
		length = _filelengthi64(_fileno(fp));
		break;
	case Mode::OpenOrCreate:
		if (_access(path, 0) == -1)
			fp = fopen(path, "wb+");
		else
			fp = fopen(path, "rb+");
		canread = true;
		canwrite = true;
		canseek = true;
		position = _ftelli64(fp);
		length = _filelengthi64(_fileno(fp));
		break;
	case Mode::Truncate:
		fp = fopen(path, "wb");
		canread = false;
		canwrite = true;
		canseek = true;
		position = _ftelli64(fp);
		length = _filelengthi64(_fileno(fp));
		break;
	case Mode::Append:
		fp = fopen(path, "ab");
		canread = false;
		canwrite = true;
		canseek = false;
		position = _ftelli64(fp);
		length = _filelengthi64(_fileno(fp));
		break;
	}
	if (!fp)
		closed = true;
}

FileStream::~FileStream()
{
	Close();
}

LRESULT FileStream::Read(LPVOID buffer, int offset, int count)
{
	if (!fp)
		return STREAMCLOSED_ERROR;
	if (length == position)
		return ENDOFSTREAM_ERROR;
	if (canread)
	{
		if (count < 0)
			return ARGUMENT_ERROR;
		position += fread((BYTE*)buffer + offset, 1, count, fp);
		return SUCCEED_OPERATE;
	}
	else
		return ACCESS_ERROR;
}

LRESULT FileStream::Write(LPVOID buffer, int offset, int count)
{
	if (!fp)
		return STREAMCLOSED_ERROR;
	if (canwrite)
	{
		if (count < 0)
			return ARGUMENT_ERROR;
		position += fwrite((BYTE*)buffer + offset, 1, count, fp);
		if (length < position)
			length = position;
		return SUCCEED_OPERATE;
	}
	else
		return ACCESS_ERROR;
}

void FileStream::Flush()
{
	if (fp)
		fflush(fp);
}

void FileStream::Close()
{
	if (!closed)
	{
		closed = true;
		fclose(fp);
		fp = 0;
	}
}

LONG64 FileStream::GetLength()
{
	if (!fp)
		return STREAMCLOSED_ERROR;
	if (canseek)
		return length;
	else
		return NOTSUPPORT_ERROR;
}

LRESULT FileStream::SetLength(LONG64 len)
{
	if (!fp)
		return STREAMCLOSED_ERROR;
	if (canwrite)
	{
		Flush();
		if (!_chsize_s(_fileno(fp), len))
		{
			position = _ftelli64(fp);
			length = _filelengthi64(_fileno(fp));
			return SUCCEED_OPERATE;
		}
		else
		{
			switch (errno)
			{
			case EACCES:
				return ACCESS_ERROR;
			case EINVAL:
				return ARGUMENT_ERROR;
			default:
				return IO_ERROR;
			}
		}
	}
	else
		return NOTSUPPORT_ERROR;
}

LONG64 FileStream::GetPosition()
{
	if (!fp)
		return STREAMCLOSED_ERROR;
	if (canseek)
		return position;
	else
		return NOTSUPPORT_ERROR;
}

LRESULT FileStream::SetPosition(LONG64 pos)
{
	if (!fp)
		return STREAMCLOSED_ERROR;
	if (canseek)
	{
		if (pos<0 || pos>length)
			return ARGUMENT_ERROR;
		Flush();
		position = pos;
		_fseeki64(fp, position, SEEK_SET);
		return SUCCEED_OPERATE;
	}
	else
		return NOTSUPPORT_ERROR;
}

MemoryStream::MemoryStream()
{
	_ptr = new BYTE[16];
	canread = true;
	canwrite = true;
	canseek = true;
	capacity = 16;
}

MemoryStream::MemoryStream(LONG64 cap)
{
	_ptr = new BYTE[cap];
	canread = true;
	canwrite = true;
	canseek = true;
	capacity = cap;
}

MemoryStream::MemoryStream(LPVOID buffer, LONG64 size)
{
	_ptr = new BYTE[size];
	memcpy(_ptr, buffer, size);
	canread = true;
	canwrite = true;
	canseek = true;
	capacity = size;
	length = size;
}

MemoryStream::~MemoryStream()
{
	Close();
}

LRESULT MemoryStream::Read(LPVOID buffer, int offset, int count)
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	if (length == position)
		return ENDOFSTREAM_ERROR;
	if (canread)
	{
		if (count < 0)
			return ARGUMENT_ERROR;
		if (position + count > length)
		{
			memcpy((BYTE*)buffer + offset, _ptr + position, length - position);
			position = length;
		}
		else
		{
			memcpy((BYTE*)buffer + offset, _ptr + position, count);
			position += count;
		}
		return SUCCEED_OPERATE;
	}
	else
		return ACCESS_ERROR;
}

LRESULT MemoryStream::Write(LPVOID buffer, int offset, int count)
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	if (canwrite)
	{
		if (count < 0)
			return ARGUMENT_ERROR;
		while (position + count > capacity)
		{
			if (CapacityAutoGrow() != SUCCEED_OPERATE)
				return OUTOFMEMORY_ERROR;
		}
		memcpy(_ptr + position, (BYTE*)buffer + offset, count);
		position += count;
		if (length < position)
			length = position;
		return SUCCEED_OPERATE;
	}
	else
		return ACCESS_ERROR;
}

void MemoryStream::Flush() {}

void MemoryStream::Close()
{
	if (!closed)
	{
		closed = true;
		if (_ptr)
			delete _ptr;
		_ptr = 0;
	}
}

LONG64 MemoryStream::GetLength()
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	if (canseek)
		return length;
	else
		return NOTSUPPORT_ERROR;
}

LRESULT MemoryStream::SetLength(LONG64 len)
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	if (canwrite)
	{
		LONG64 oldlen = length;
		LONG64 oldpos = position;
		length = len;
		if (position > length)
			position = length;
		if (length > capacity)
		{
			BYTE* _newptr = new BYTE[length];
			if (!_newptr)
			{
				length = oldlen;
				position = oldpos;
				return OUTOFMEMORY_ERROR;
			}
			else
			{
				memcpy(_newptr, _ptr, capacity);
				capacity = length;
				if (_ptr)
					delete _ptr;
				_ptr = _newptr;
			}
		}
		return SUCCEED_OPERATE;
	}
	else
		return NOTSUPPORT_ERROR;
}

LONG64 MemoryStream::GetPosition()
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	if (canseek)
		return position;
	else
		return NOTSUPPORT_ERROR;
}

LRESULT MemoryStream::SetPosition(LONG64 pos)
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	if (canseek)
	{
		if (pos<0 || pos>length)
			return ARGUMENT_ERROR;
		position = pos;
		return SUCCEED_OPERATE;
	}
	else
		return NOTSUPPORT_ERROR;
}

LONG64 MemoryStream::GetCapacity()
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	return capacity;
}

LRESULT MemoryStream::SetCapacity(LONG64 cap)
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	if (canwrite)
	{
		if (length > cap)
		{
			BYTE* _newptr = new BYTE[cap];
			if (!_newptr)
				return OUTOFMEMORY_ERROR;
			else
			{
				memcpy(_newptr, _ptr, capacity < cap ? capacity : cap);
				capacity = cap;
				length = cap;
				if (position > length)
					position = length;
				if (_ptr)
					delete _ptr;
				_ptr = _newptr;
			}
		}
		else
		{
			BYTE* _newptr = new BYTE[cap];
			if (!_newptr)
				return OUTOFMEMORY_ERROR;
			else
			{
				memcpy(_newptr, _ptr, capacity < cap ? capacity : cap);
				capacity = cap;
				if (_ptr)
					delete _ptr;
				_ptr = _newptr;
			}
		}
		return SUCCEED_OPERATE;
	}
	else
		return NOTSUPPORT_ERROR;
}

LPVOID MemoryStream::GetBuffer()
{
	return _ptr;
}

LRESULT MemoryStream::ToArray(LPVOID buffer, LONG64 buffersize, LONG64& outputcount)
{
	if (!_ptr)
		return STREAMCLOSED_ERROR;
	memcpy(buffer, _ptr, outputcount = (buffersize < length ? buffersize : length));
	return SUCCEED_OPERATE;
}

LRESULT MemoryStream::CapacityAutoGrow()
{
	BYTE* _newptr = new BYTE[capacity * 1.5];
	if (!_newptr)
		return OUTOFMEMORY_ERROR;
	else
	{
		memcpy(_newptr, _ptr, capacity);
		capacity *= 1.5;
		if (_ptr)
			delete _ptr;
		_ptr = _newptr;
	}
	return SUCCEED_OPERATE;
}

GZipStream::GZipStream(Stream* stream, CompressionMode mode)
{
	remain = 0;
	inputstream = stream;
	cstream = new z_stream;
	z_stream* c_stream = (z_stream*)cstream;
	if (mode == CompressionMode::Compress)
	{
		canread = false;
		canwrite = true;
		canseek = false;
		const int windowBits = 15;
		const int GZIP_ENCODING = 16;
		c_stream->zalloc = (alloc_func)0;
		c_stream->zfree = (free_func)0;
		c_stream->opaque = (voidpf)0;
		deflateInit2(c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY);
	}
	else
	{
		canread = true;
		canwrite = false;
		canseek = false;
		c_stream->zalloc = (alloc_func)0;
		c_stream->zfree = (free_func)0;
		c_stream->opaque = (voidpf)0;
		inflateInit2(c_stream, MAX_WBITS + 16);
	}
}

GZipStream::~GZipStream()
{
	Close();
}

LRESULT GZipStream::Read(LPVOID buffer, int offset, int count)
{
	if (!cstream || !inputstream)
		return STREAMCLOSED_ERROR;
	if (canread)
	{
		if (count < 0)
			return ARGUMENT_ERROR;
		z_stream* c_stream = (z_stream*)cstream;
		c_stream->next_out = (Bytef*)buffer;
		c_stream->avail_out = count;
		int outread;
		while (c_stream->avail_out != 0)
		{
			if (remain == 0)
			{
				if (inputstream->GetLength() == inputstream->GetPosition())
					return ENDOFSTREAM_ERROR;
				if (inputstream->GetPosition() + GZIPBUFFERSIZE > inputstream->GetLength())
					outread = inputstream->GetLength() - inputstream->GetPosition();
				else
					outread = GZIPBUFFERSIZE;
				inputstream->Read(outbuffer, 0, outread);
				remain = outread;
				c_stream->next_in = (Bytef*)outbuffer;
				c_stream->avail_in = outread;
				if (inflate(c_stream, Z_NO_FLUSH) < 0)
					return ARGUMENT_ERROR;
				remain = c_stream->avail_in;
			}
			else
			{
				if (inflate(c_stream, Z_NO_FLUSH) < 0)
					return ARGUMENT_ERROR;
				remain = c_stream->avail_in;
			}
		}
		return SUCCEED_OPERATE;
	}
	else
		return ACCESS_ERROR;
}

LRESULT GZipStream::Write(LPVOID buffer, int offset, int count)
{
	if (!cstream || !inputstream)
		return STREAMCLOSED_ERROR;
	if (canwrite)
	{
		if (count < 0)
			return ARGUMENT_ERROR;
		z_stream* c_stream = (z_stream*)cstream;
		c_stream->next_in = (Bytef*)buffer + offset;
		c_stream->avail_in = count;
		while (c_stream->avail_in != 0)
		{
			c_stream->next_out = (Bytef*)outbuffer;
			c_stream->avail_out = GZIPBUFFERSIZE;
			if (deflate(c_stream, Z_NO_FLUSH) < 0)
				return ARGUMENT_ERROR;
			if (c_stream->avail_out < GZIPBUFFERSIZE)
				inputstream->Write(outbuffer, 0, GZIPBUFFERSIZE - c_stream->avail_out);
		}
		return SUCCEED_OPERATE;
	}
	else
		return ACCESS_ERROR;
}

void GZipStream::Flush() {}

void GZipStream::Close()
{
	if (!closed)
	{
		closed = true;
		if (canwrite)
		{
			z_stream* c_stream = (z_stream*)cstream;
			c_stream->next_in = 0;
			c_stream->avail_in = 0;
			int ret;
			while (true)
			{
				c_stream->next_out = (Bytef*)outbuffer;
				c_stream->avail_out = GZIPBUFFERSIZE;
				ret = deflate(c_stream, Z_FINISH);
				inputstream->Write(outbuffer, 0, GZIPBUFFERSIZE - c_stream->avail_out);
				if (ret == Z_STREAM_END)
					break;
			}
			deflateEnd(c_stream);
			delete c_stream;
		}
		else
		{
			z_stream* c_stream = (z_stream*)cstream;
			inflateEnd(c_stream);
			delete c_stream;
		}
	}
}

LONG64 GZipStream::GetLength()
{
	return NOTSUPPORT_ERROR;
}

LRESULT GZipStream::SetLength(LONG64 len)
{
	return NOTSUPPORT_ERROR;
}

LONG64 GZipStream::GetPosition()
{
	return NOTSUPPORT_ERROR;
}

LRESULT GZipStream::SetPosition(LONG64 pos)
{
	return NOTSUPPORT_ERROR;
}