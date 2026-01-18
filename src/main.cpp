#include "zconf.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <zlib.h>
#include <vector>

namespace baje {
struct ReplayHeader {
	::std::uint32_t size;
	::std::uint32_t version;
	::std::uint32_t numberOfCompressedDataBlocks;
	::std::uint16_t sizeOfCompressedDataBlock;
	::std::uint16_t sizeOfDecompressedDataBlock;
};
}

void printPlayerRecord(unsigned char *data, size_t length) {
	size_t count = 0;
::std::cout << "Record id:\t0x" << ::std::hex << data[count++] << ::std::endl;
::std::cout << "Player id:\t0x" << ::std::hex << data[count++] << ::std::endl;
::std::cout << "Player name\t";
		for (; data[count] != 0 && count < length; ++count) {
			::std::cout << data[count];
		}
	::std::cout << ::std::endl;
}

static inline void readCompressedActions(::std::ifstream &replayStream, baje::ReplayHeader &header) {
	char unknown[4] = {0};
	z_stream strm;
	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	auto ret = inflateInit(&strm);
	if (ret != Z_OK) {
		return;
	}
	bool error = false;
	for (size_t i = 0; i < header.numberOfCompressedDataBlocks; ++i) {
		replayStream.read(reinterpret_cast<char *>(&header.sizeOfCompressedDataBlock), sizeof(header.sizeOfCompressedDataBlock));
		replayStream.read(reinterpret_cast<char *>(&header.sizeOfDecompressedDataBlock), sizeof(header.sizeOfDecompressedDataBlock));
		replayStream.read(unknown, sizeof(unknown));
		unsigned char *in = new unsigned char[header.sizeOfCompressedDataBlock];
		unsigned char *out = new unsigned char[header.sizeOfDecompressedDataBlock];
		replayStream.read(reinterpret_cast<char *>(in), header.sizeOfCompressedDataBlock);

		strm.avail_in = header.sizeOfCompressedDataBlock;
		strm.next_in = in;


		while(true) {
			strm.avail_out = header.sizeOfDecompressedDataBlock;
			strm.next_out = out;
			ret = inflate(&strm, Z_SYNC_FLUSH);

			if  (ret != Z_OK) {
				error = true;
				::std::cout << "Inflating error " << ret << " (" << strm.msg << ")" << ::std::endl;
				goto done;
			}
			if (ret == Z_STREAM_END) {
			    std::cout << "stream end reached at block " << i << "\n";
			    goto done;
			}
			auto produced = header.sizeOfDecompressedDataBlock - strm.avail_out;
			printPlayerRecord(out + 4, produced - 4);
		}

	done:
		delete [] in;
		delete [] out;
		inflateEnd(&strm);
		if (error) {
			break;
		}
	}
}

static inline void collectDataFromReplayFile(const ::std::string &filePath) {
	::std::ifstream replayStream{filePath, ::std::ios_base::binary};
	if (replayStream.is_open()) {
		baje::ReplayHeader header{};
		// read haeder size
		replayStream.seekg(0x001c);
		replayStream.read(reinterpret_cast<char *>(&header.size), sizeof(header.size));
		::std::cout << "0x" << ::std::hex << header.size << ::std::endl;

		// read replay haeder version
		replayStream.seekg(0x0024);
		replayStream.read(reinterpret_cast<char *>(&header.version), sizeof(header.version));
		::std::cout << "Replay header version:\t0x" << header.version << ::std::endl;


		replayStream.seekg(0x002c);
		replayStream.read(reinterpret_cast<char *>(&header.numberOfCompressedDataBlocks), sizeof(header.numberOfCompressedDataBlocks));
		::std::cout << "Number of compressed data blocks:\t" << ::std::dec << header.numberOfCompressedDataBlocks << ::std::endl;	
		readCompressedActions(replayStream, header);

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
