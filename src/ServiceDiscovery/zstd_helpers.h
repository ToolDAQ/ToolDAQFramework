#ifndef ZSTD_HELPERS
#define ZSTD_HELPERS

#include "zmq.hpp"
#include "zstd.h"
#include <string>
#include <utility>

namespace ToolFramework {

// we could avoid the need for all these splits if we supported c++17 std::string_view 🙃️

// compression - implementation
std::pair<const char*, size_t>  ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, char* compress_buffer, size_t MAX_COMPRESSED_SIZE, std::string* compress_buffer2, size_t COMPRESS_THRESHOLD, int zstd_compression_level); // don't compress anything less than 50B by default

// compression wrapper - char array compression buffer
std::pair<const char*, size_t>  ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, char* compress_buffer, size_t MAX_COMPRESSED_SIZE, size_t COMPRESS_THRESHOLD=50, int zstd_compression_level=1);

// compression wrapper - string compression buffer
std::pair<const char*, size_t>  ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, std::string& compress_buffer, size_t COMPRESS_THRESHOLD=50, int zstd_compression_level=1);

/*
zmq::message_t ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, char* compress_buffer, size_t MAX_COMPRESSED_SIZE, size_t COMPRESS_THRESHOLD, int zstd_compression_level);

zmq::message_t ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, std::string& compress_buffer, size_t COMPRESS_THRESHOLD, int zstd_compression_level);
*/

// decompression
bool ZstdDecompress(ZSTD_DCtx* zstd_dctx, const char* msg, size_t msg_len, std::string& decompress_buffer, size_t MAX_DECOMPRESSED_SIZE=104857600);  // don't decompress anything that will expand to > 100MB by default

bool ZstdDecompress(ZSTD_DCtx* zstd_dctx, const std::string& msg, std::string& decompress_buffer, size_t MAX_DECOMPRESSED_SIZE=104857600);  // don't decompress anything that will expand to > 100MB by default

}

#endif
