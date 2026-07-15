#include <format>
#include <mutex>
#include <string>
#include <unordered_map>
#include <slim/common/memory_mapper.h>
#include <slim/common/log.h>
namespace slim::common::memory_mapper {
	using namespace slim::common;
	using lock_guard = std::lock_guard<std::mutex>;
	std::mutex map_mutex;
	std::mutex maps_mutex;
	std::unordered_map<std::string, std::mutex> maps_mutexs;
}
[[maybe_unused]] bool slim::common::memory_mapper::attach(const std::string& _map_name, map_pointer _map) {
	if(_map_name.empty()) {
		log::error(log::Message(__func__, "map name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::attach() map name is empty exception");
	}
	if(_map == nullptr) {
		log::error(log::Message(__func__, "map == nullptr", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::attach() nullptr exception => " + _map_name);
	}
	try {
		lock_guard map_lock(map_mutex);
		maps[_map_name] = _map;
		lock_guard maps_lock(maps_mutex);
		maps_mutexs[_map_name];
	}
	catch(const std::bad_alloc& e) {
		log::error(log::Message(__func__, "std::bad_alloc => " + std::string(e.what()) + " => map => " + _map_name, __FILE__, __LINE__));
		return false;
	}
	return true;
}
[[maybe_unused]] bool slim::common::memory_mapper::create(const std::string& _map_name) {
	log::trace(log::Message(__func__, "begins => " + _map_name, __FILE__, __LINE__));
	if(_map_name.empty()) {
		log::error(log::Message(__func__, "map name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::create() map name is empty exception");
	}
	try {
		lock_guard map_lock(map_mutex);
		if(!maps.contains(_map_name)) {
			maps[_map_name] = std::make_shared<map_container>();
			lock_guard maps_lock(maps_mutex);
			maps_mutexs[_map_name];
		}
	}
	catch(const std::bad_alloc& e) {
		log::error(log::Message(__func__, "std::bad_alloc => unable to create map => " + _map_name, __FILE__, __LINE__));
		return false;
	}
	return true;
}
void slim::common::memory_mapper::erase(const std::string& _map_name) {
	log::trace(log::Message(__func__, "begins => " + _map_name, __FILE__, __LINE__));
	if(_map_name.empty()) {
		log::error(log::Message(__func__, "map name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::erase() map name is empty exception");
	}
/* 	if(map_exists(_map_name)) {
		write_lock map_lock(map_mutex);
		auto items_erased = maps.erase(_map_name);
		
		log::debug(log::Message("slim::common::memory_mapper::erase()", "map => " + _map_name + " => erased", __FILE__, __LINE__));
		if(items_erased != 1) {
			log::debug(log::Message("slim::common::memory_mapper::erase()", "map => "
				+ _map_name + " => erased => " + std::to_string(items_erased) + " items => expected 1", __FILE__, __LINE__));
		}
		std::lock_guard<std::mutex> maps_lock(maps_mutex);
		auto mutexs_erased = maps_mutexs.erase(_map_name);
		log::debug(log::Message("slim::common::memory_mapper::erase()", "map mutex => " + _map_name + " => erased", __FILE__, __LINE__));
		if(mutexs_erased != 1) {
			log::debug(log::Message("slim::common::memory_mapper::erase()", "map mutex => "
				+ _map_name + " => erased => " + std::to_string(mutexs_erased) + " items => expected 1", __FILE__, __LINE__));
		}
	} */
	log::trace(log::Message("slim::common::memory_mapper::erase()", "ends => " + _map_name, __FILE__, __LINE__));
}
[[maybe_unused]] bool slim::common::memory_mapper::map_exists(const std::string& _map_name) {
	if(_map_name.empty()) {
		log::error(log::Message(__func__, "map name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::exists(_map_name) map name is empty exception");
	}
	lock_guard map_lock(map_mutex);
	return maps.contains(_map_name);
}
[[maybe_unused]] bool slim::common::memory_mapper::variable_exists(const std::string& _map_name, const std::string& _variable_name) {
	if(_map_name.empty()) {
		log::error(log::Message(__func__, "map name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::variable_exists() map name is empty exception");
	}
	if(_variable_name.empty()) {
		log::error(log::Message(__func__, "variable name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::variable_exists() variable name is empty exception");
	}
	bool answer = false;
	lock_guard map_lock(map_mutex);
	if(maps.contains(_map_name)) {
		lock_guard maps_lock(maps_mutexs[_map_name]);
		answer = maps[_map_name].get()->contains(_variable_name);
	}
	return answer;
}

[[maybe_unused]] const std::vector<std::string> slim::common::memory_mapper::list_keys(const std::string& _map_name) {
	if(_map_name.empty()) {
		log::error(log::Message("slim::common::memory_mapper::list_keys()", "map name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::list_keys() map name is empty exception");
	}
	std::vector<std::string> keys_vector;
	if(map_exists(_map_name)) {
		std::lock_guard<std::mutex> lock(maps_mutexs[_map_name]);
		for(auto [key,value] : *maps[_map_name]) {
			keys_vector.emplace_back(key);
		}
	}
	return keys_vector;
}

[[maybe_unused]] std::shared_ptr<std::string> slim::common::memory_mapper::read(const std::string& _map_name, const std::string& _variable_name) {
	std::shared_ptr<std::string> content_pointer = nullptr;
	if(map_exists(_map_name)) {
		std::lock_guard<std::mutex> lock(maps_mutexs[_map_name]);
		auto content_iterator = maps[_map_name].get()->find(_variable_name);
		if(content_iterator != maps[_map_name].get()->end()) {
			if(std::holds_alternative<std::string>(content_iterator->second)) {
				content_pointer = std::make_shared<std::string>(std::get<std::string>(content_iterator->second));
			}
			else if(std::holds_alternative<std::shared_ptr<std::string>>(content_iterator->second)) {
				content_pointer = std::get<std::shared_ptr<std::string>>(content_iterator->second);
			}
			else {
				log::error(log::Message(__func__, std::format("map => {} => variable => {} is not of type string or string pointer", _map_name, _variable_name), __FILE__, __LINE__));
			}
		}
	}
	return content_pointer;
}

[[maybe_unused]] bool slim::common::memory_mapper::read_bool(const std::string& _map_name, const std::string& _variable_name) {
	bool value = false;
	lock_guard map_lock(map_mutex);
	if(maps.contains(_map_name)) {
		lock_guard maps_lock(maps_mutexs[_map_name]);
		auto content_iterator = maps[_map_name].get()->find(_variable_name);
		if(content_iterator != maps[_map_name].get()->end()) {
			if(std::holds_alternative<bool>(content_iterator->second)) {
				value = std::get<bool>(content_iterator->second);
			}
			else {
				log::error(log::Message(__func__, std::format("map => {} => variable => {} is not of type bool", _map_name, _variable_name), __FILE__, __LINE__));
			}
		}
	}
	return value;
}

[[maybe_unused]] std::string slim::common::memory_mapper::read_string(const std::string& _map_name, const std::string& _variable_name) {
	std::string content_string;
	lock_guard map_lock(map_mutex);
	if(maps.contains(_map_name)) {
		lock_guard maps_lock(maps_mutexs[_map_name]);
		auto content_iterator = maps[_map_name].get()->find(_variable_name);
		if(content_iterator != maps[_map_name].get()->end()) {
			if(std::holds_alternative<std::string>(content_iterator->second)) {
				content_string = std::get<std::string>(content_iterator->second);
			}
			else if(std::holds_alternative<std::shared_ptr<std::string>>(content_iterator->second)) {
				content_string = *std::get<std::shared_ptr<std::string>>(content_iterator->second).get();
			}
			else {
				log::error(log::Message(__func__, std::format("map => {} => variable => {} is not of type string or string pointer", _map_name, _variable_name), __FILE__, __LINE__));
			}
		}
	}
	return content_string;
}
[[maybe_unused]] std::string_view slim::common::memory_mapper::read_string_view(const std::string& _map_name, const std::string& _variable_name) {
	std::string_view content_string_view;
	lock_guard map_lock(map_mutex);
	if(maps.contains(_map_name)) {
		lock_guard maps_lock(maps_mutexs[_map_name]);
		auto content_iterator = maps[_map_name].get()->find(_variable_name);
		if(content_iterator != maps[_map_name].get()->end()) {
			if(std::holds_alternative<std::string>(content_iterator->second)) {
				content_string_view = std::get<std::string>(content_iterator->second);
			}
			else if(std::holds_alternative<std::shared_ptr<std::string>>(content_iterator->second)) {
				content_string_view = *std::get<std::shared_ptr<std::string>>(content_iterator->second).get();
			}
			else {
				log::error(log::Message(__func__, std::format("map => {} => variable => {} is not of type string or string pointer", _map_name, _variable_name), __FILE__, __LINE__));
			}
		}
	}
	return content_string_view;
}

[[maybe_unused]] bool slim::common::memory_mapper::write(const std::string& _map_name, const std::string& _variable_name, content_variant _content) {
	if(_map_name.empty()) {
		log::error(log::Message(__func__, "map name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::write() map name is empty exception");
	}
	if(_variable_name.empty()) {
		log::error(log::Message(__func__, "variable name is empty", __FILE__, __LINE__));
		throw("slim::common::memory_mapper::write() variable name is empty exception");
	}

	bool completed = false;
	try {
		lock_guard map_lock(map_mutex);
		if(!maps.contains(_map_name)) {
			maps[_map_name] = std::make_shared<map_container>();
		}
		lock_guard maps_lock(maps_mutexs[_map_name]);
		auto map = maps[_map_name].get();
		if(std::holds_alternative<bool>(_content)) {
			completed = map->insert_or_assign(_variable_name, std::get<bool>(_content)).second;
		}
		else if(std::holds_alternative<std::string>(_content)) {
			completed = map->insert_or_assign(_variable_name, std::get<std::string>(_content)).second;;
		}
		else if(std::holds_alternative<std::shared_ptr<std::string>>(_content)) {
			completed = map->insert_or_assign(_variable_name, std::get<std::shared_ptr<std::string>>(_content)).second;;
		}
		else {
			log::error(log::Message(__func__, "unknown data type in write request for map name => " + _map_name + " variable name => " + _variable_name,__FILE__,__LINE__));
			throw("slim::common::memory_mapper::write() => unknown data type in write request for map name => " + _map_name + " variable name => " + _variable_name);
		}
	}
	catch(const std::bad_alloc& e) {
		log::error(log::Message(__func__, "std::bad_alloc => " + std::string(e.what()) + " => map => " + _map_name, __FILE__, __LINE__));
		return false;
	}
	return completed;
}