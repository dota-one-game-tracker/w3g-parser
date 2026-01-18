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

		// read data
		replayStream.seekg(header.size);
		replayStream.read(reinterpret_cast<char *>(&header.sizeOfCompressedDataBlock), sizeof(header.sizeOfCompressedDataBlock));
		::std::cout << "Size of compressed data blocks:\t" << header.sizeOfCompressedDataBlock << ::std::endl;

		replayStream.read(reinterpret_cast<char *>(&header.sizeOfDecompressedDataBlock), sizeof(header.sizeOfDecompressedDataBlock));
		::std::cout << "Size of decompressed data blocks:\t" << header.sizeOfDecompressedDataBlock << ::std::endl;

		::std::uint32_t unknown = 0;
		replayStream.read(reinterpret_cast<char *>(&unknown), sizeof(unknown));
		::std::cout << "Unknown:\t" << unknown << ::std::endl;

		auto in  = new unsigned char[header.sizeOfCompressedDataBlock]();
		auto out = new unsigned char[header.sizeOfDecompressedDataBlock]();

		z_stream strm{};
		int ret = inflateInit(&strm); // if this is RAW deflate use inflateInit2(&strm, -MAX_WBITS)
		if (ret != Z_OK) return;

		for (uint32_t i = 0; i < header.numberOfCompressedDataBlocks; ++i) {
		    replayStream.read(reinterpret_cast<char*>(in), header.sizeOfCompressedDataBlock);
			printf("block %u: %02X %02X %02X %02X\n", i,
       			in[0], in[1],
		        in[2], in[3]);
		    const auto bytesRead = replayStream.gcount();
		    if (bytesRead <= 0) break;

		    strm.next_in  = in;
		    strm.avail_in = static_cast<uInt>(bytesRead);

		    while (true) {
			strm.next_out  = out;
			strm.avail_out = header.sizeOfDecompressedDataBlock;

			int r = inflate(&strm, Z_NO_FLUSH);

			const size_t produced = header.sizeOfDecompressedDataBlock - strm.avail_out;
			if (produced > 0) {
			    // IMPORTANT: produced bytes, not avail_out
			    if (produced >= 4) printPlayerRecord(out + 4, produced - 4);
			}

			if (r == Z_STREAM_END) {
			    goto done; // whole stream finished
			}

			if (r == Z_OK) {
			    // keep going depending on buffers below
			} else if (r == Z_BUF_ERROR) {
			    // NOT necessarily fatal: it just couldn't make progress right now
			    if (strm.avail_out == 0) {
				// need more output space
				continue;
			    }
			    if (strm.avail_in == 0) {
				// need more input: go read next compressed block
				break;
			    }
			    // otherwise: no progress even though we had some space/input -> treat as error
			    std::cerr << "Z_BUF_ERROR with avail_in=" << strm.avail_in
				      << " avail_out=" << strm.avail_out << "\n";
			    goto done;
			} else {
			    std::cerr << "inflate error r=" << r
				      << " msg=" << (strm.msg ? strm.msg : "(null)") << "\n";
			    goto done;
			}

			// if output buffer filled, loop again with fresh output buffer
			if (strm.avail_out == 0) continue;

			// if we consumed all input from this block, read next block
			if (strm.avail_in == 0) break;
		    }
		}

		done:
		inflateEnd(&strm);
		delete[] in;
		delete[] out;
	

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
