#ifndef _Checksum_H
#define _Checksum_H

class Checksum{
	private:
		std::array<std::uint_fast32_t, 256> createLookupTable() noexcept;
		template <typename InputIterator>
		std::uint_fast32_t crc(InputIterator first, InputIterator last);
	public:
		std::string calculateCRC(std::string);
};

#endif