#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <zlib.h>

namespace baje
{
struct ReplayHeader {
	::std::uint32_t size;
	::std::uint32_t version;
	::std::uint32_t numberOfCompressedDataBlocks;
	::std::uint16_t sizeOfCompressedDataBlock;
	::std::uint16_t sizeOfDecompressedDataBlock;
};
} // namespace baje

static uint16_t read_u16_le(std::istream &s)
{
	uint8_t b[2];
	s.read(reinterpret_cast<char *>(b), 2);
	if (!s)
		throw std::runtime_error("read_u16_le failed");
	return static_cast<uint16_t>(b[0] | (static_cast<uint16_t>(b[1]) << 8));
}

static uint32_t read_u32_le(std::istream &s)
{
	uint8_t b[4];
	s.read(reinterpret_cast<char *>(b), 4);
	if (!s)
		throw std::runtime_error("read_u32_le failed");
	return static_cast<uint32_t>(b[0] | (static_cast<uint32_t>(b[1]) << 8) |
				     (static_cast<uint32_t>(b[2]) << 16) |
				     (static_cast<uint32_t>(b[3]) << 24));
}

void printPlayerRecord(unsigned char *data, size_t length)
{
	size_t count = 0;
	::std::cout << "Record id:\t0x" << ::std::hex << data[count++]
		    << ::std::endl;
	::std::cout << "Player id:\t0x" << ::std::hex << data[count++]
		    << ::std::endl;
	::std::cout << "Player name\t";
	for (; data[count] != 0 && count < length; ++count) {
		::std::cout << data[count];
	}
	::std::cout << ::std::endl;
}

static inline void readCompressedActions(::std::ifstream &replayStream,
					 baje::ReplayHeader &header)
{

	bool error = false;
	for (size_t i = 0; i < header.numberOfCompressedDataBlocks; ++i) {
		header.sizeOfCompressedDataBlock = read_u16_le(replayStream);
		header.sizeOfDecompressedDataBlock = read_u16_le(replayStream);
		auto unknown = read_u32_le(replayStream);
		(void) unknown;
		::std::cout << ::std::dec << header.sizeOfCompressedDataBlock
			    << ", " << header.sizeOfDecompressedDataBlock
			    << ::std::endl;
		unsigned char *in =
			new unsigned char[header.sizeOfCompressedDataBlock];
		unsigned char *out =
			new unsigned char[header.sizeOfDecompressedDataBlock];
		replayStream.read(reinterpret_cast<char *>(in),
				  header.sizeOfCompressedDataBlock);

		z_stream strm;
		/* allocate inflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		auto ret = inflateInit(&strm);
		if (ret != Z_OK) {
			error = true;
		}
		strm.avail_in = header.sizeOfCompressedDataBlock;
		strm.next_in = in;
		int r;
		do {
			strm.avail_out = header.sizeOfDecompressedDataBlock;
			strm.next_out = out;
			r = inflate(&strm, Z_SYNC_FLUSH);

			if (r == Z_NEED_DICT)
				r = Z_DATA_ERROR;

			if (r != Z_OK && r != Z_STREAM_END) {
				std::cerr << "Block " << i
					  << ": inflate error r=" << r
					  << " msg="
					  << (strm.msg ? strm.msg : "(null)")
					  << "\n";
				error = true;
				break;
			}

			// If input consumed but not ended, we might need more
			// input (shouldn't happen if n is correct)
			if (r != Z_STREAM_END && strm.avail_in == 0) {
				// Some formats rely on Z_SYNC_FLUSH and don't
				// end with STREAM_END; accept exact outSize
				break;
			}

		} while (r != Z_STREAM_END);

		auto produced =
			header.sizeOfDecompressedDataBlock - strm.avail_out;
		printPlayerRecord(out + 4, produced - 4);

		delete[] in;
		delete[] out;
		inflateEnd(&strm);
		if (error) {
			break;
		}
	}
}

static inline void collectDataFromReplayFile(const ::std::string &filePath)
{
	::std::ifstream replayStream{filePath, ::std::ios_base::binary};
	if (replayStream.is_open()) {
		baje::ReplayHeader header{};
		// read haeder size
		replayStream.seekg(0x001c);
		replayStream.read(reinterpret_cast<char *>(&header.size),
				  sizeof(header.size));
		::std::cout << "0x" << ::std::hex << header.size << ::std::endl;

		// read replay haeder version
		replayStream.seekg(0x0024);
		replayStream.read(reinterpret_cast<char *>(&header.version),
				  sizeof(header.version));
		::std::cout << "Replay header version:\t0x" << header.version
			    << ::std::endl;

		replayStream.seekg(0x002c);
		replayStream.read(reinterpret_cast<char *>(
					  &header.numberOfCompressedDataBlocks),
				  sizeof(header.numberOfCompressedDataBlocks));
		::std::cout << "Number of compressed data blocks:\t"
			    << ::std::dec << header.numberOfCompressedDataBlocks
			    << ::std::endl;
		replayStream.seekg(header.size);
		readCompressedActions(replayStream, header);

	} else {
		::std::cerr << "Could not open file " << ::std::endl;
	}
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		::std::cerr << "Provide at least one path to a file"
			    << ::std::endl;
		return EXIT_FAILURE;
	}
	for (int i = 1; i < argc; ++i) {
		collectDataFromReplayFile(argv[i]);
	}
	return EXIT_SUCCESS;
}
