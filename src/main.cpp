#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>
#include <zlib.h>

namespace baje {
struct ReplayHeader {
	::std::uint32_t size;
	::std::uint32_t version;
	::std::uint32_t numberOfCompressedDataBlocks;
	::std::uint16_t sizeOfCompressedDataBlock;
	::std::uint16_t sizeOfDecompressedDataBlock;
};

enum PlayerRecordType : ::std::uint8_t { host = 0x00, additionalPlayer = 0x16 };

struct PlayerRecord {
	PlayerRecordType type;
	::std::uint8_t playerId;
	::std::string playerName;
	::std::string a;
};

struct Cursor {
	const std::uint8_t* p;
	const std::uint8_t* end;

	bool read_u8(std::uint8_t& v) {
		if (p >= end)
			return false;
		v = *p++;
		return true;
	}

	bool read_u32(::std::uint32_t& v) {
		if (p + sizeof(::std::uint32_t) >= end) {
			return false;
		}
		::std::memcpy(&v, p, sizeof(::std::uint32_t));
		p = p + sizeof(::std::uint32_t);
		return true;
	}

	::std::uint8_t peek() { return *p; }

	bool skip(size_t n) {
		if (static_cast<size_t>(end - p) < n)
			return false;
		p += n;
		return true;
	}

	std::string read_cstr() {
		std::string s;
		while (p < end && *p != 0)
			s.push_back(static_cast<char>(*p++));
		if (p < end && *p == 0)
			++p; // consume null
		return s;
	}
};
} // namespace baje

static uint16_t read_u16_le(::std::istream& s) {
	uint8_t b[2];
	s.read(reinterpret_cast<char*>(b), 2);
	if (!s)
		throw std::runtime_error("read_u16_le failed");
	return static_cast<uint16_t>(b[0] | (static_cast<uint16_t>(b[1]) << 8));
}

static uint32_t read_u32_le(::std::istream& s) {
	uint8_t b[4];
	s.read(reinterpret_cast<char*>(b), 4);
	if (!s)
		throw std::runtime_error("read_u32_le failed");
	return static_cast<uint32_t>(
		b[0] | (static_cast<uint32_t>(b[1]) << 8) |
		(static_cast<uint32_t>(b[2]) << 16) |
		(static_cast<uint32_t>(b[3]) << 24)
	);
}

static inline baje::PlayerRecord handlePlayerRecord(baje::Cursor& cursor) {
	baje::PlayerRecord playerRecord{};
	::std::uint8_t type = 0;
	::std::uint8_t id = 0;
	cursor.read_u8(type);
	playerRecord.type = (baje::PlayerRecordType)type;
	cursor.read_u8(id);
	playerRecord.playerId = id;
	playerRecord.playerName = cursor.read_cstr();
	::std::uint8_t additionalData = 0;
	cursor.read_u8(additionalData);
	if (additionalData == 0x01) {
		cursor.skip(1);
	} else {
		cursor.skip(8);
	}
	return playerRecord;
}

static inline ::std::uint32_t handlePlayerCount(baje::Cursor& cursor) {
	::std::uint32_t playerCount;
	cursor.read_u32(playerCount);
	::std::cout << "Player count " << playerCount << ::std::endl;
	return playerCount;
}

static inline void handleGameType(baje::Cursor& cursor) {
	::std::uint32_t gameType = 0;
	cursor.read_u32(gameType);
	::std::cout << "Game type " << gameType << ::std::endl;
}

static inline void handleLanguageId(baje::Cursor& cursor) {
	::std::uint32_t languageId = 0;
	cursor.read_u32(languageId);
	::std::cout << "Language id 0x" << ::std::hex << languageId
		    << ::std::endl;
}

static inline void handleGameSettings(baje::Cursor& cursor) {
	::std::cout << cursor.read_cstr() << ::std::endl;
}

static inline void handleGameName(baje::Cursor& cursor) {
	::std::cout << cursor.read_cstr() << ::std::endl;
}

static inline void
handleFirstDecompressedBlock(const std::uint8_t* data, size_t len) {
	baje::Cursor cursor{data, data + len};

	::std::vector<baje::PlayerRecord> players{};
	cursor.skip(4); // doc says hostname begins at 6th byte of first block
			// in your interpretation
	players.emplace_back(handlePlayerRecord(cursor));
	handleGameName(cursor);
	cursor.skip(1);
	handleGameSettings(cursor);
	auto playerCount = handlePlayerCount(cursor);
	handleGameType(cursor);
	handleLanguageId(cursor);

	for (::std::uint32_t i = 1; i < playerCount; i++) {
		players.emplace_back(handlePlayerRecord(cursor));
		cursor.skip(4);
	}
	for (const auto& p : players) {
		::std::cout << p.playerName << ", id=0x" << ::std::hex
			    << (int)p.playerId << ::std::endl;
	}
}

static inline void readCompressedActions(
	::std::ifstream& replayStream,
	baje::ReplayHeader& header
) {

	bool error = false;
	for (size_t i = 0; i < header.numberOfCompressedDataBlocks; ++i) {
		header.sizeOfCompressedDataBlock = read_u16_le(replayStream);
		header.sizeOfDecompressedDataBlock = read_u16_le(replayStream);
		auto unknown = read_u32_le(replayStream);
		(void)unknown;
		unsigned char* in =
			new unsigned char[header.sizeOfCompressedDataBlock];
		unsigned char* out =
			new unsigned char[header.sizeOfDecompressedDataBlock];
		replayStream.read(
			reinterpret_cast<char*>(in),
			header.sizeOfCompressedDataBlock
		);

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

		if (i == 0) {
			::std::cout << "out[5] = 0x" << ::std::hex << out[5]
				    << ::std::endl;
			handleFirstDecompressedBlock(out, produced);
		}
		delete[] in;
		delete[] out;
		inflateEnd(&strm);
		if (error) {
			break;
		}
	}
}

static inline void collectDataFromReplayFile(const ::std::string& filePath) {
	::std::ifstream replayStream{filePath, ::std::ios_base::binary};
	if (replayStream.is_open()) {
		baje::ReplayHeader header{};
		// read haeder size
		replayStream.seekg(0x001c);
		replayStream.read(
			reinterpret_cast<char*>(&header.size),
			sizeof(header.size)
		);
		::std::cout << "0x" << ::std::hex << header.size << ::std::endl;

		// read replay haeder version
		replayStream.seekg(0x0024);
		replayStream.read(
			reinterpret_cast<char*>(&header.version),
			sizeof(header.version)
		);
		::std::cout << "Replay header version:\t0x" << header.version
			    << ::std::endl;

		replayStream.seekg(0x002c);
		replayStream.read(
			reinterpret_cast<char*>(
				&header.numberOfCompressedDataBlocks
			),
			sizeof(header.numberOfCompressedDataBlocks)
		);
		::std::cout << "Number of compressed data blocks:\t"
			    << ::std::dec << header.numberOfCompressedDataBlocks
			    << ::std::endl;
		replayStream.seekg(header.size);
		readCompressedActions(replayStream, header);

	} else {
		::std::cerr << "Could not open file " << ::std::endl;
	}
}

int main(int argc, char** argv) {
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
