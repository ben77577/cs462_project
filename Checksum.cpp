#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <iomanip>
#include "Checksum.hpp"
#include <iostream>
 
//Create a lookup table for the checksum
std::array<std::uint_fast32_t, 256> Checksum::createLookupTable() noexcept{
	auto const reversed_polynomial = std::uint_fast32_t{0xEDB88320uL};
 
	//calculate checksum for a value then increment value starting from zero
	struct byte_checksum{
		std::uint_fast32_t operator()() noexcept{
			auto checksum = static_cast<std::uint_fast32_t>(n++);
			for (auto i = 0; i < 8; ++i)
				checksum = (checksum >> 1) ^ ((checksum & 0x1u) ? reversed_polynomial : 0);
 
			return checksum;
		}
		unsigned n = 0;
	};
	auto table = std::array<std::uint_fast32_t, 256>{};
	std::generate(table.begin(), table.end(), byte_checksum{});
 
	return table;
}
 
//Calculate CRC
template <typename InputIterator>
std::uint_fast32_t Checksum::crc(InputIterator first, InputIterator last){
	// Generate lookup table on first use
	static auto const table = createLookupTable();
 
	//Calculate to checksum and make sure it's 32 bits
	return std::uint_fast32_t{0xFFFFFFFFuL} & ~std::accumulate(first, last, ~std::uint_fast32_t{0} & std::uint_fast32_t{0xFFFFFFFFuL}, 
		[](std::uint_fast32_t checksum, std::uint_fast8_t value) 
		{ return table[(checksum ^ value) & 0xFFu] ^ (checksum >> 8); });
}

//Get the checksum for the given string (return hex string)
std::string Checksum::calculateCRC(std::string message){
	std::cout<<"Inside crc buffer looks like: " << message << "\n";
	auto const s = message;
	std::stringstream stream;
	stream << std::hex << Checksum::crc(s.begin(), s.end());
	std::string result(stream.str());
	return result;
}
 
 