#ifndef _OBJECT_H
#define _OBJECT_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>

class Checksum{
	private:
		std::array<std::uint_fast32_t, 256> createLookupTable() noexcept;
		template <typename InputIterator>
		std::uint_fast32_t crc(InputIterator first, InputIterator last);
	public:
		std::string calculateCRC(std::string);
};

#endif