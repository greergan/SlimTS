#include <cstdint>
#include <memory>
#include <streambuf>
#include <vector>
#include <slim/common/vector_buffer.h>
slim::common::char_vector_buffer::char_vector_buffer() {}
slim::common::char_vector_buffer::char_vector_buffer(std::shared_ptr<std::vector<uint8_t>> vector_data_pointer) {
	setg((char*)vector_data_pointer.get()->data(), (char*)vector_data_pointer.get()->data(), (char*)vector_data_pointer.get()->size());
}