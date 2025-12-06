#include <mutex>
#include <unordered_map>
#include <slim/common/memory_mapper.h>
#include <slim/common/log.h>
namespace slim::common::memory_mapper {
	using namespace slim::common;
	std::mutex write_mutex;
	std::mutex read_mutex;
}
void slim::common::memory_mapper::attach(std::string map_name_string, map_pointer map) {
	log::trace(log::Message("slim::common::memory_mapper::attach()", "begins => " + map_name_string, __FILE__, __LINE__));
	if(map == nullptr) {
		log::error(log::Message("slim::common::memory_mapper::create()", "map == nullptr", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::attach() nullptr exception");
	}
	try {	
		if(!exists(map_name_string)) {
			maps[map_name_string] = map;
			log::debug(log::Message("slim::common::memory_mapper::attach()", "attached => " + map_name_string, __FILE__, __LINE__));
			log::debug(log::Message("slim::common::memory_mapper::attach()", "map size => " + std::to_string(map->size()), __FILE__, __LINE__));
		}
		else {
			log::debug(log::Message("slim::common::memory_mapper::attach()", "did not attach to existing map => " + map_name_string, __FILE__, __LINE__));
		}
	}
	catch(const std::bad_alloc& e) {
		throw(e);
	}
	log::trace(log::Message("slim::common::memory_mapper::attach()", "ends => " + map_name_string, __FILE__, __LINE__));
}
void slim::common::memory_mapper::create(std::string map_name_string) {
	log::trace(log::Message("slim::common::memory_mapper::create()", "begins => " + map_name_string, __FILE__, __LINE__));
	try {
		if(!exists(map_name_string)) {
			std::unique_lock<std::mutex> lock(write_mutex);
			maps[map_name_string] = std::make_shared<map_container>();
			log::debug(log::Message("slim::common::memory_mapper::create()", "created => " + map_name_string, __FILE__, __LINE__));
		}
	}
	catch(const std::bad_alloc& e) {
		throw(e);
	}
	log::trace(log::Message("slim::common::memory_mapper::create()", std::string("ends => " + map_name_string).c_str(), __FILE__, __LINE__));
}
void slim::common::memory_mapper::erase(std::string map_name_string) {
	log::trace(log::Message("slim::common::memory_mapper::erase()", "begins =>" + map_name_string, __FILE__, __LINE__));
	if(exists(map_name_string)) {
		std::unique_lock<std::mutex> lock(write_mutex);
		auto items_erased = maps.erase(map_name_string);
		if(items_erased != 1) {
			log::warn(log::Message("slim::common::memory_mapper::erase()", "erased =>" + std::to_string(items_erased) + " items from => " + map_name_string + ", should be 1", __FILE__, __LINE__));
		}
		log::debug(log::Message("slim::common::memory_mapper::erase()", "erased map name =>" + map_name_string, __FILE__, __LINE__));
	}
	log::trace(log::Message("slim::common::memory_mapper::erase()", "ends =>" + map_name_string, __FILE__, __LINE__));
}
bool slim::common::memory_mapper::exists(std::string map_name_string) {
	log::trace(log::Message("slim::common::memory_mapper::exists()", "begins => " + map_name_string, __FILE__, __LINE__));
	int map_size = 0;
	std::unique_lock<std::mutex> lock(read_mutex);
	bool answer = maps.count(map_name_string) > 0 ? true : false;
	if(answer) {
		map_size = maps.find(map_name_string)->second.get()->size();
	}
	std::string answer_message_string = answer ? "true" : "false";
	log::debug(log::Message("slim::common::memory_mapper::exists()", map_name_string + " map exists => "
		+ answer_message_string + ", map size => " + std::to_string(map_size), __FILE__, __LINE__));
	log::trace(log::Message("slim::common::memory_mapper::exists()", "ends map name => " + map_name_string + ", exists => " + answer_message_string, __FILE__, __LINE__));
	return answer;
}
bool slim::common::memory_mapper::exists(std::string map_name_string, std::string file_name_string) {
	log::trace(log::Message("slim::common::memory_mapper::exists()", "begins, map name => " + map_name_string + " file name => " + file_name_string, __FILE__, __LINE__));
	bool answer = false;
	if(exists(map_name_string)) {
		std::unique_lock<std::mutex> lock(read_mutex);
		auto map = *maps[map_name_string].get();
		answer = map.count(file_name_string) > 0 ? true : false;
		std::string answer_message_string = answer == true ? "true" : "false";
		log::debug(log::Message("slim::common::memory_mapper::exists()", "map name => " + map_name_string + " file name => " + file_name_string
			+ " exists => " + answer_message_string, __FILE__, __LINE__));
	}
	log::trace(log::Message("slim::common::memory_mapper::exists()", "ends, map name => " + map_name_string + " file name => " + file_name_string, __FILE__, __LINE__));
	return answer;
}
const std::vector<std::string> slim::common::memory_mapper::list_keys(std::string map_name_string) {
	log::trace(log::Message("slim::common::memory_mapper::list_keys()", std::string("begins => " + map_name_string).c_str(), __FILE__, __LINE__));
	std::vector<std::string> keys_vector;
	if(exists(map_name_string)) {
		for(auto [key,value] : *maps[map_name_string]) {
			keys_vector.emplace_back(key);
		}
	}
	return keys_vector;
	log::trace(log::Message("slim::common::memory_mapper::list_keys()", std::string("ends => " + map_name_string).c_str(), __FILE__, __LINE__));
}
std::shared_ptr<std::string> slim::common::memory_mapper::read(std::string map_name_string, std::string file_name_string) {
	log::trace(log::Message("slim::common::memory_mapper::read()", "begins file => " + file_name_string + " from map => " + map_name_string, __FILE__, __LINE__));
	std::shared_ptr<std::string> content_pointer;
	std::string file_found_string = "false";
	if(exists(map_name_string)) {
		std::unique_lock<std::mutex> lock(read_mutex);
		auto content_iterator = maps[map_name_string].get()->find(file_name_string);
		if(content_iterator != maps[map_name_string].get()->end()) {
			if(std::holds_alternative<std::string>(content_iterator->second)) {
				content_pointer = std::make_shared<std::string>(std::get<std::string>(content_iterator->second));
			}
			else if(std::holds_alternative<std::shared_ptr<std::string>>(content_iterator->second)) {
				content_pointer = std::get<std::shared_ptr<std::string>>(content_iterator->second);
			}
			file_found_string = "true";
			log::debug(log::Message("slim::common::memory_mapper::read()", "read "
				+ std::to_string(content_pointer.get()->size()) + " bytes from map name => " + map_name_string + ", file name => " + file_name_string, __FILE__, __LINE__));
		}
	}
	log::trace(log::Message("slim::common::memory_mapper::read()", "ends file => " + file_name_string + " from map => " + map_name_string + " found => " + file_found_string, __FILE__, __LINE__));
	return content_pointer;
}
std::string slim::common::memory_mapper::read_string(std::string map_name_string, std::string variable_name_string) {
	log::trace(log::Message("slim::common::memory_mapper::read()", "begins for map name => " + map_name_string + " variable name => " + variable_name_string, __FILE__, __LINE__));
	std::string content_string;
	if(exists(map_name_string)) {
		std::unique_lock<std::mutex> lock(read_mutex);
		auto content_iterator = maps[map_name_string].get()->find(variable_name_string);
		if(content_iterator != maps[map_name_string].get()->end()) {
			if(std::holds_alternative<std::string>(content_iterator->second)) {
				content_string = std::get<std::string>(content_iterator->second);
			}
			else if(std::holds_alternative<std::shared_ptr<std::string>>(content_iterator->second)) {
				content_string = *std::get<std::shared_ptr<std::string>>(content_iterator->second).get();
			}
		}
		else {
			log::debug(log::Message("slim::common::memory_mapper::read()", "file not found => " + variable_name_string + " in map => " + map_name_string, __FILE__, __LINE__));
		}
	}
	log::debug(log::Message("slim::common::memory_mapper::read()", "read "
		+ std::to_string(content_string.length()) + " bytes from map name => " + map_name_string + " variable name => " + variable_name_string, __FILE__, __LINE__));
	log::trace(log::Message("slim::common::memory_mapper::read()", "ends for map name => " + map_name_string + " variable name => " + variable_name_string, __FILE__, __LINE__));
	return content_string;
}
void slim::common::memory_mapper::write(std::string map_name_string, std::string file_name_string, content_variant content_string_or_pointer) {
	log::trace(log::Message("slim::common::memory_mapper::write()", "begins for map name => " + map_name_string + " file name => " + file_name_string,__FILE__,__LINE__));
	int write_size = 0;
	try {
		if(!exists(map_name_string)) {
			create(map_name_string);
		}
		auto map = maps[map_name_string].get();
		log::debug(log::Message("slim::common::memory_mapper::write()", "preparing to write to newly created map => " + map_name_string,__FILE__,__LINE__));
		if(std::holds_alternative<std::string>(content_string_or_pointer)) {
			log::debug(log::Message("slim::common::memory_mapper::write()", "handling write case => std::string for map name => " + map_name_string,__FILE__,__LINE__));
			auto content_string = std::get<std::string>(content_string_or_pointer);
			std::unique_lock<std::mutex> lock(write_mutex);
			if(content_string.length() < 16) {
				(*map)[file_name_string] = content_string;
			}
			else {
				(*map)[file_name_string] = std::make_shared<std::string>(content_string);
			}
			write_size = content_string.length();
		}
		else if(std::holds_alternative<std::shared_ptr<std::string>>(content_string_or_pointer)) {
			auto content_pointer = std::get<std::shared_ptr<std::string>>(content_string_or_pointer);
			write_size = content_pointer->size();
			log::debug(log::Message("slim::common::memory_mapper::write()", "handling write case => std::shared_ptr<std::string> for map name => " + map_name_string,__FILE__,__LINE__));
			std::unique_lock<std::mutex> lock(write_mutex);
			(*map)[file_name_string] = content_pointer;
		}
		else {
			log::error(log::Message("slim::common::memory_mapper::write()", "unknown data type in write request for map name => " + map_name_string + " file name => " + file_name_string,__FILE__,__LINE__));
		}
	}
	catch(const std::bad_alloc& e) {
		log::error(log::Message("slim::common::memory_mapper::write()", e.what(), __FILE__, __LINE__));
		throw(e);
	}
	log::debug(log::Message("slim::common::memory_mapper::write()", "wrote " + std::to_string(write_size) 
		+ " bytes to map => " + map_name_string + " file name => " + file_name_string,__FILE__,__LINE__));
	log::trace(log::Message("slim::common::memory_mapper::write()", "ends for map name => " + map_name_string + " file name => " + file_name_string,__FILE__,__LINE__));
}