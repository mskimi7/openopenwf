#pragma once

#include <vector>

class BinaryWriteStream {
private:
	std::vector<unsigned char> buffer;

public:
	inline const std::vector<unsigned char>& GetBuffer() const { return buffer; }

	inline void WriteBytes(const void* buf, size_t len) { this->buffer.insert(this->buffer.end(), (const unsigned char*)buf, (const unsigned char*)buf + len); }

	template <typename T>
	void Write(T val);

	BinaryWriteStream(BinaryWriteStream const&) = delete;
	BinaryWriteStream& operator=(BinaryWriteStream const&) = delete;
	inline BinaryWriteStream() {}
};

template<> inline void BinaryWriteStream::Write(char val) { return WriteBytes(&val, sizeof(char)); }
template<> inline void BinaryWriteStream::Write(unsigned char val) { return WriteBytes(&val, sizeof(unsigned char)); }
template<> inline void BinaryWriteStream::Write(short val) { return WriteBytes(&val, sizeof(short)); }
template<> inline void BinaryWriteStream::Write(unsigned short val) { return WriteBytes(&val, sizeof(unsigned short)); }
template<> inline void BinaryWriteStream::Write(int val) { return WriteBytes(&val, sizeof(int)); }
template<> inline void BinaryWriteStream::Write(unsigned int val) { return WriteBytes(&val, sizeof(unsigned int)); }
template<> inline void BinaryWriteStream::Write(float val) { return WriteBytes(&val, sizeof(float)); }
template<> inline void BinaryWriteStream::Write(double val) { return WriteBytes(&val, sizeof(double)); }
template<> inline void BinaryWriteStream::Write(long long val) { return WriteBytes(&val, sizeof(long long)); }
template<> inline void BinaryWriteStream::Write(unsigned long long val) { return WriteBytes(&val, sizeof(unsigned long long)); }
