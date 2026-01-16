#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <zlib.h>
#include <vector>

namespace baje {
struct ReplayData {};
}

static inline void collectDataFromReplayFile(const ::std::string &filePath) {
	::std::ifstream replayStream{filePath, ::std::ios_base::binary};
	if (replayStream.is_open()) {
		// read haeder size
		replayStream.seekg(0x001c);
		::std::uint32_t headerSize = 0;
		replayStream.read(reinterpret_cast<char *>(&headerSize), sizeof(::std::uint32_t));
		::std::cout << "0x" << ::std::hex << headerSize << ::std::endl;

		// read replay haeder version
		replayStream.seekg(0x0024);
		::std::uint32_t replayHeaderVersion = 0;
		replayStream.read(reinterpret_cast<char *>(&replayHeaderVersion), sizeof(replayHeaderVersion));
		::std::cout << "Replay header version:\t0x" << replayHeaderVersion << ::std::endl;


		replayStream.seekg(0x002c);
		::std::uint32_t numberOfCompressedDataBlocks = 0;
		replayStream.read(reinterpret_cast<char *>(&numberOfCompressedDataBlocks), sizeof(numberOfCompressedDataBlocks));
		::std::cout << "Number of compressed data blocks:\t" << ::std::dec << numberOfCompressedDataBlocks << ::std::endl;

		// read data
		replayStream.seekg(headerSize);
		::std::uint16_t sizeOfCompressedDataBlocks = 0;
		replayStream.read(reinterpret_cast<char *>(&sizeOfCompressedDataBlocks), sizeof(sizeOfCompressedDataBlocks));
		::std::cout << "Size of compressed data blocks:\t" << sizeOfCompressedDataBlocks << ::std::endl;

		replayStream.seekg(6, ::std::ios_base::cur);
		auto dataForOneCompressedBlock = new char[sizeOfCompressedDataBlocks]();
		for (::std::uint32_t i = 0; i < numberOfCompressedDataBlocks; ++i) {
			replayStream.read(dataForOneCompressedBlock, sizeOfCompressedDataBlocks);
			 z_stream strm{};
		}
	} else {
		::std::cerr << "Could not open file " << ::std::endl;
	}
}

int main (int argc, char **argv) {
	if (argc == 1) {
		::std::cerr << "Provide at least one path to a file" << ::std::endl;
		return EXIT_FAILURE;
	}
	for (int i = 1; i < argc; ++i) {
		collectDataFromReplayFile(argv[i]);
	}
	return EXIT_SUCCESS;
}
