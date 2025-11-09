#pragma once

#include <vector>

class BinaryReadStream {
private:
	unsigned char* data = nullptr;
	size_t dataLength = 0;
	size_t currentPos = 0;

	template <typename T>
	T ReadPrimitive()
	{
		T value;
		ReadBytes(&value, sizeof(T));

		return value;
	}

public:
	inline bool EndOfDataReached() const { return currentPos >= dataLength; }
	inline size_t GetCurrentPos() const { return currentPos; };
	inline size_t GetRemainingBytes() const { return EndOfDataReached() ? 0 : dataLength - currentPos; }

	inline void ReadBytes(void* buf, size_t len)
	{
		if (len > GetRemainingBytes())
			memset(buf, 0, len);

		len = std::min(len, GetRemainingBytes());
		memcpy(buf, this->data + this->currentPos, len);
		this->currentPos += len;
	}

	template <typename T>
	T Read();

	inline BinaryReadStream(unsigned char* data_, size_t dataLength_) : data(data_), dataLength(dataLength_) {}
	BinaryReadStream(BinaryReadStream const&) = delete;
	BinaryReadStream& operator=(BinaryReadStream const&) = delete;
};

template<> inline char BinaryReadStream::Read() { return ReadPrimitive<char>(); }
template<> inline unsigned char BinaryReadStream::Read() { return ReadPrimitive<unsigned char>(); }
template<> inline short BinaryReadStream::Read() { return ReadPrimitive<short>(); }
template<> inline unsigned short BinaryReadStream::Read() { return ReadPrimitive<unsigned short>(); }
template<> inline int BinaryReadStream::Read() { return ReadPrimitive<int>(); }
template<> inline unsigned int BinaryReadStream::Read() { return ReadPrimitive<unsigned int>(); }
template<> inline long long BinaryReadStream::Read() { return ReadPrimitive<long long>(); }
template<> inline unsigned long long BinaryReadStream::Read() { return ReadPrimitive<unsigned long long>(); }
