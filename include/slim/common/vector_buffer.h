#ifndef SLIM__COMMON__VECTOR__BUFFER__H
#define SLIM__COMMON__VECTOR__BUFFER__H
#include <cstdint>
#include <streambuf>
#include <vector>
namespace slim::common {
	struct char_vector_buffer : std::streambuf {
		char_vector_buffer();
		char_vector_buffer(std::shared_ptr<std::vector<uint8_t>> vector_data_pointer);
	};
}
#endif