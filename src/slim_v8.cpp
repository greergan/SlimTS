#include <format>
#include <mutex>
#include <string>
#include <unordered_map>
#include <v8.h>
#include <libplatform/libplatform.h>
#include <slim/common/log.h>
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
}
v8::Isolate* slim::v_8::new_isolate(const std::string& _label) {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	std::lock_guard isolate_lock(isolates_mutex);
	v8::Isolate* isolate;
	if(!isolates.contains(_label)) {
		auto pair = isolates.emplace(_label, v8::Isolate::New(create_params));
		if(pair.second) {
			isolate = pair.first->second;
			log::debug(log::Message(__func__,std::format("created and stored v8::Isolate for => {}", _label),__FILE__, __LINE__));
		}
		else {
			log::debug(log::Message(__func__,std::format("unable to created and store v8::Isolate for => {}", _label),__FILE__, __LINE__));
		}
	}
	else {
		log::error(log::Message(__func__,std::format("unable to create v8::Isolate for existing label => {}", _label),__FILE__, __LINE__));
		log::error(log::Message(__func__,"returning bad v8::Isolate*",__FILE__, __LINE__));
	}
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
	return isolate;
}
void slim::v_8::initialize(std::vector<std::string>& _v8_command_line_arguments) {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
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
	log::debug(log::Message(__func__,"called InitializeICUDefaultLocation",__FILE__, __LINE__));

	platform = v8::platform::NewDefaultPlatform();
	log::debug(log::Message(__func__,"created platform",__FILE__, __LINE__));

	v8::V8::InitializePlatform(platform.get());
	log::debug(log::Message(__func__,"initialized platform",__FILE__, __LINE__));

	v8::V8::Initialize();
	log::debug(log::Message(__func__,"initialized V8",__FILE__, __LINE__));

//log::debug(log::Message(__func__,"setting command line flags",__FILE__, __LINE__));
//v8::V8::SetFlagsFromCommandLine(arg_count, args.data(), false);
//log::debug(log::Message(__func__,"set command line flags",__FILE__, __LINE__));

	create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	log::debug(log::Message(__func__,"created new default allocator on create_params",__FILE__, __LINE__));
	v8_is_initialized = true;
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
}
void slim::v_8::tear_down() {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	if(v8_is_initialized) {
		for(const auto& isolate : isolates) {
			log::debug(log::Message(__func__,std::format("isolate => {} => IsInUse => {}",
				isolate.first, utilities::to_string(isolate.second->IsInUse())),__FILE__, __LINE__));
			if(isolate.second->IsInUse()) {
				isolate.second->TerminateExecution();
				isolate.second->Exit();
				isolate.second->Dispose();
				log::debug(log::Message(__func__,std::format("disposed => {} isolate", isolate.first),__FILE__, __LINE__));
			}
		}
		v8::V8::Dispose();
		log::debug(log::Message(__func__,"disposed => V8",__FILE__, __LINE__));
		v8::V8::DisposePlatform();
		log::debug(log::Message(__func__,"disposed => platform",__FILE__, __LINE__));
		delete create_params.array_buffer_allocator;
		log::debug(log::Message(__func__,"deleted => array_buffer_allocator",__FILE__, __LINE__));
		v8_is_initialized = false;
	}
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
}
