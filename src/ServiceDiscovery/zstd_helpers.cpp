#include "zstd_helpers.h"
#include <cstring>
#include <iostream>

namespace ToolFramework {

std::pair<const char*, size_t>  ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, char* compress_buffer, size_t MAX_COMPRESSED_SIZE, std::string* compress_buffer2, size_t COMPRESS_THRESHOLD, int zstd_compression_level){
	std::string errmsg="";
	size_t bytes_to_send = msg_len;
	
	if(msg_len<COMPRESS_THRESHOLD){
		goto send_uncompressed;
	}
	
	bytes_to_send = ZSTD_compressBound(msg_len); // this can fail too!
	if(ZSTD_isError(bytes_to_send)){
		errmsg = std::string{"Warning: error calculating compressed size: "}+ZSTD_getErrorName(bytes_to_send);
		goto send_uncompressed;
	}
	
	if(compress_buffer2!=nullptr){
		compress_buffer2->resize(bytes_to_send);
		compress_buffer = (char*)compress_buffer2->data();
		MAX_COMPRESSED_SIZE = bytes_to_send;
	} else if(compress_buffer!=nullptr && bytes_to_send>MAX_COMPRESSED_SIZE){
		errmsg = "Warning: compressed size "+std::to_string(bytes_to_send)+" exceeds compression buffer size "+std::to_string(MAX_COMPRESSED_SIZE);
		goto send_uncompressed;
	}
	
	bytes_to_send = ZSTD_compressCCtx(zstd_cctx, (void*)compress_buffer, MAX_COMPRESSED_SIZE, msg, msg_len, zstd_compression_level);
	if(ZSTD_isError(bytes_to_send)){
		errmsg = std::string{"Warning: error compressing message "}+ZSTD_getErrorName(bytes_to_send);
		goto send_uncompressed;
	}
	
	return std::pair<const char*, size_t>{compress_buffer, bytes_to_send};
	
	send_uncompressed:
	if(!errmsg.empty())std::cerr << errmsg << std::endl; // FIXME is this appropriate?
	return std::pair<const char*, size_t>{msg, msg_len};
}


// accept compression buffer char array, return pointer and size of data to send
std::pair<const char*, size_t>  ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, char* compress_buffer, size_t MAX_COMPRESSED_SIZE, size_t COMPRESS_THRESHOLD, int zstd_compression_level){
	return ZstdCompress(zstd_cctx, msg, msg_len, compress_buffer, MAX_COMPRESSED_SIZE, 0, COMPRESS_THRESHOLD, zstd_compression_level);
}

// accept compression buffer string, return pointer and size of data to send
std::pair<const char*, size_t>  ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, std::string& compress_buffer, size_t COMPRESS_THRESHOLD, int zstd_compression_level){
	return ZstdCompress(zstd_cctx, msg, msg_len, 0, 0, &compress_buffer, COMPRESS_THRESHOLD, zstd_compression_level);
}

/*
// accept compression buffer char array, return zmq::message_t
zmq::message_t ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, char* compress_buffer, size_t MAX_COMPRESSED_SIZE, size_t COMPRESS_THRESHOLD, int zstd_compression_level){
	
	std::pair<const char*, size_t> output = ZstdCompress(zstd_cctx, msg, msg_len, compress_buffer, MAX_COMPRESSED_SIZE, nullptr, COMPRESS_THRESHOLD, zstd_compression_level);
	
	zmq::message_t zmsg(output.second);
	memcpy(zmsg.data(), output.first, output.second);
	return zmsg;
	
}

// accept compression buffer string, return zmq::message_t
zmq::message_t ZstdCompress(ZSTD_CCtx* zstd_cctx, const char* msg, size_t msg_len, std::string& compress_buffer, size_t COMPRESS_THRESHOLD, int zstd_compression_level){
	
	std::pair<const char*, size_t> output = ZstdCompress(zstd_cctx, msg, msg_len, 0, 0, compress_buffer, COMPRESS_THRESHOLD, zstd_compression_level);
	
	zmq::message_t zmsg(output.second);
	memcpy(zmsg.data(), output.first, output.second);
	return zmsg;
	
}
*/

//////////////////////////////////////////////////

bool ZstdDecompress(ZSTD_DCtx* zstd_dctx, const char* msg, size_t msg_len, std::string& decompress_buffer, size_t MAX_DECOMPRESSED_SIZE){
	if(msg_len>sizeof(ZSTD_MAGIC_BYTES) && std::memcmp(msg,ZSTD_MAGIC_BYTES,sizeof(ZSTD_MAGIC_BYTES))==0){
		size_t decompressed_bytes = ZSTD_getFrameContentSize(msg, msg_len);
		if(decompressed_bytes==ZSTD_CONTENTSIZE_UNKNOWN || decompressed_bytes==ZSTD_CONTENTSIZE_ERROR){
			// bad response
			decompress_buffer = std::string{"Received corrupt zstd message "}+ZSTD_getErrorName(decompressed_bytes);
			return false;
		}
		if(decompressed_bytes > MAX_DECOMPRESSED_SIZE){
			decompress_buffer = "Compressed message with oversized payload: "+std::to_string(decompressed_bytes)+" bytes";
			return false;
		}
		decompress_buffer.resize(decompressed_bytes);
		decompressed_bytes = ZSTD_decompressDCtx(zstd_dctx,(void*)decompress_buffer.data(),decompressed_bytes, msg, msg_len);
		if(ZSTD_isError(decompressed_bytes)){
			decompress_buffer = std::string{"zstd error decompressing response: "}+ZSTD_getErrorName(decompressed_bytes);
			return false;
		}
	} else {
		// message not compressed
		decompress_buffer.assign(msg, msg_len);
	}
	return true;
}

bool ZstdDecompress(ZSTD_DCtx* zstd_dctx, const std::string& msg, std::string& decompress_buffer, size_t MAX_DECOMPRESSED_SIZE){
	return ZstdDecompress(zstd_dctx, msg.data(), msg.length(), decompress_buffer, MAX_DECOMPRESSED_SIZE);
}

}
