#pragma once
#ifndef DS_DATA_BUFFER_H
#define DS_DATA_BUFFER_H
#include <string>
#include "read_write_buffer.h"
#include "ds/query/recycle_array.h"

namespace ds {

/*
	* brief
	*   add functions must be matched be read function.
	*/
class DataBuffer
{
public:
	DataBuffer(unsigned initialStreamSize = 0);
	unsigned size();
	void seekBegin();
	void clear();

	template <typename T>
	bool canRead()
	{
		unsigned size = sizeof(T);
		unsigned currentPosition = mStream.getReadPosition();

		mStream.setReadPosition(ReadWriteBuffer::End);
		unsigned length = mStream.getReadPosition();
		mStream.setReadPosition(currentPosition);

		if(size > (length - currentPosition))
			return false;
		return true;
	}

	// function to add raw data no size added.
	void addRaw(const char *b, unsigned size);
	// function to read raw data no size will be read.
	bool readRaw(char *b, unsigned size);

	// will write size when writing data.
	void add(const char *b, unsigned size);
	template <typename T>
	void add(const T &t)
	{
		mStream.write((const char *)(&t), sizeof(t));
	}

    
	void add(const char *cs);
	void add(const wchar_t *cs);

	// will read size from buffer and only read if size is available.
	bool read(char *b, unsigned size);
	template <typename T>
	T read()
	{
		T t;
		mStream.read((char *)(&t), sizeof(t));
		return t;
	}

	template <typename T>
	void rewindRead()
	{
		unsigned currentPosition = mStream.getReadPosition();

		if(sizeof(T) > currentPosition)
			return;

		mStream.rewindRead(sizeof(T));
	}

	template <typename T>
	void rewindAdd()
	{
		unsigned currentPosition = mStream.getWritePosition();

		if(sizeof(T) > currentPosition)
			return;

		mStream.rewindWrite(sizeof(T));
	}
private:
	ReadWriteBuffer		mStream;
	RecycleArray<char>   mStringBuffer;
	RecycleArray<char>   mWStringBuffer;
};
    
    
    template <>
    std::string DataBuffer::read<std::string>();
    template <>
    std::wstring DataBuffer::read<std::wstring>();
    
    template <>
    std::string DataBuffer::read<std::string>();
    template <>
    std::wstring DataBuffer::read<std::wstring>();

template <>
std::string DataBuffer::read<std::string>()
{
	unsigned size = read<unsigned>();
	unsigned currentPosition = mStream.getReadPosition();

	mStream.setReadPosition(ReadWriteBuffer::End);
	unsigned length = mStream.getReadPosition();
	mStream.setReadPosition(currentPosition);

	if(size > (length - currentPosition)) {
		add(size);
		return std::string();
	}

	mStringBuffer.setSize(size);

	mStream.read(mStringBuffer.data(), size);
	return std::string(mStringBuffer.data(), size);
}

template <>
std::wstring DataBuffer::read<std::wstring>()
{
	unsigned size = read<unsigned>();
	unsigned currentPosition = mStream.getReadPosition();

	mStream.setReadPosition(ReadWriteBuffer::End);
	unsigned length = mStream.getReadPosition();
	mStream.setReadPosition(currentPosition);

	if(size > (length - currentPosition)) {
		add(size);
		return std::wstring();
	}

	mWStringBuffer.setSize(size);

	mStream.read((char *)mWStringBuffer.data(), size);
	return std::wstring((const wchar_t *)mWStringBuffer.data(), size / 2);
}

}

#endif//DS_DATA_BUFFER_H
