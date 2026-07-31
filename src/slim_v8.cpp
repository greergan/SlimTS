#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <v8.h>
#include <libplatform/libplatform.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/common/utilities.h>
#include <slim/path.h>
#include <slim/slim_v8.h>
namespace slim::v_8 {
	using namespace slim::common;
	v8::Isolate::CreateParams create_params;
	std::unique_ptr<v8::Platform> platform;
	static std::unordered_map<std::string, v8::Isolate*> isolates;
	std::mutex isolates_mutex;
	bool v8_is_initialized = false;
	static std::unordered_map<v8::Isolate*, std::vector<std::function<void()>>> cleanup_hooks;
}

void slim::v_8::dispose_isolate(std::string_view label) {
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
#endif
	std::lock_guard isolate_lock(isolates_mutex);
	auto it = isolates.find(std::string(label));
	if(it != isolates.end()) {
		it->second->Dispose();
		isolates.erase(it);
#ifdef ENABLE_LOGGING
		log::debug(log::Message(__func__,std::format("disposed isolate => {}", label),__FILE__, __LINE__));
#endif
	}
	else {
#ifdef ENABLE_LOGGING
		log::error(log::Message(__func__,std::format("isolate not found => {}", label),__FILE__, __LINE__));
#endif
	}
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
}

v8::Platform* slim::v_8::get_platform() {
    return platform.get();
}

void slim::v_8::initialize(std::vector<std::string>& _v8_command_line_arguments) {
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
#endif
	int* arg_count = (int*)0; //_v8_command_line_arguments.size();
	std::vector<char*> args;
	args.push_back(nullptr);
/* 	char** args = new char*[arg_count + 1];
	for(int index = 0; index < arg_count; index++) {
		args[index] = strdup(_v8_command_line_arguments[index].c_str());
	}
	result[arg_count] = nullptr; */
	//char* args = _v8_command_line_arguments.data();
/* 	std::vector<char*> args;
	args.reserve(arg_count + 1);
	for(auto& arg : _v8_command_line_arguments) {
		args.push_back(*arg);
    }
	args.push_back(nullptr); */

	v8::V8::InitializeICUDefaultLocation(slim::path::getExecutablePath().c_str());
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__,"called InitializeICUDefaultLocation",__FILE__, __LINE__));
#endif

	platform = v8::platform::NewDefaultPlatform(0, // thread_pool_size = 0, no background threads
        v8::platform::IdleTaskSupport::kDisabled, v8::platform::InProcessStackDumping::kDisabled);
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__,"created platform",__FILE__, __LINE__));
#endif

	v8::V8::InitializePlatform(platform.get());
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__,"initialized platform",__FILE__, __LINE__));
#endif

	v8::V8::Initialize();
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__,"initialized V8",__FILE__, __LINE__));
#endif

//log::debug(log::Message(__func__,"setting command line flags",__FILE__, __LINE__));
//v8::V8::SetFlagsFromCommandLine(arg_count, args.data(), false);
//log::debug(log::Message(__func__,"set command line flags",__FILE__, __LINE__));

	create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
#ifdef ENABLE_LOGGING
	log::debug(log::Message(__func__,"created new default allocator on create_params",__FILE__, __LINE__));
#endif
	v8_is_initialized = true;
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
}

v8::Isolate* slim::v_8::new_isolate(std::string label) {
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
#endif
	std::lock_guard isolate_lock(isolates_mutex);
	v8::Isolate* isolate;
	if(!isolates.contains(label)) {
		auto pair = isolates.emplace(label, v8::Isolate::New(create_params));
		if(pair.second) {
			isolate = pair.first->second;
#ifdef ENABLE_LOGGING
			log::debug(log::Message(__func__,std::format("created and stored v8::Isolate for => {}", label),__FILE__, __LINE__));
#endif
		}
		else {
#ifdef ENABLE_LOGGING
			log::debug(log::Message(__func__,std::format("unable to created and store v8::Isolate for => {}", label),__FILE__, __LINE__));
#endif
		}
	}
	else {
#ifdef ENABLE_LOGGING
		log::error(log::Message(__func__,std::format("unable to create v8::Isolate for existing label => {}", label),__FILE__, __LINE__));
#endif
#ifdef ENABLE_LOGGING
		log::error(log::Message(__func__,"returning bad v8::Isolate*",__FILE__, __LINE__));
#endif
	}
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
	return isolate;
}

void slim::v_8::register_cleanup(v8::Isolate* isolate, std::function<void()> fn) {
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
	std::lock_guard lock(isolates_mutex);
	cleanup_hooks[isolate].push_back(std::move(fn));
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}

void slim::v_8::run_cleanup(v8::Isolate* isolate) {
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
	std::vector<std::function<void()>> hooks;
	{
		std::lock_guard lock(isolates_mutex);
		auto it = cleanup_hooks.find(isolate);
		if (it != cleanup_hooks.end()) {
			hooks = std::move(it->second);
			cleanup_hooks.erase(it);
		}
	}
	for (auto& fn : hooks) {
		fn();
	}
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}

void slim::v_8::tear_down() {
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
#endif
	if(v8_is_initialized) {
		for(const auto& isolate : isolates) {
#ifdef ENABLE_LOGGING
			log::debug(log::Message(__func__,std::format("isolate => {} => IsInUse => {}", isolate.first, isolate.second->IsInUse()),
			    __FILE__, __LINE__));
#endif
			if(isolate.second->IsInUse()) {
				isolate.second->TerminateExecution();
				isolate.second->Exit();
				isolate.second->Dispose();
#ifdef ENABLE_LOGGING
				log::debug(log::Message(__func__,std::format("disposed => {} isolate", isolate.first),__FILE__, __LINE__));
#endif
			}
		}
		v8::V8::Dispose();
#ifdef ENABLE_LOGGING
		log::debug(log::Message(__func__,"disposed => V8",__FILE__, __LINE__));
#endif
		v8::V8::DisposePlatform();
#ifdef ENABLE_LOGGING
		log::debug(log::Message(__func__,"disposed => platform",__FILE__, __LINE__));
#endif
		delete create_params.array_buffer_allocator;
#ifdef ENABLE_LOGGING
		log::debug(log::Message(__func__,"deleted => array_buffer_allocator",__FILE__, __LINE__));
#endif
		v8_is_initialized = false;
	}
#ifdef ENABLE_LOGGING
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
#endif
}
