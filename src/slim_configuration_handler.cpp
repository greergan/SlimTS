#include <atomic>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <unordered_set>
#include <v8.h>
#include <slim/path.h>
#include <slim/configuration/strings.h>
#include <slim/configuration_handler.h>
#include <slim/common/fetch.h>
#include <slim/common/log.h>
#include <slim/common/memory_mapper.h>
#include <slim/common/utilities.h>
#include <slim/common/web_file.h>
#include <slim/utilities.h>
#include <slim/slim_v8.h>
namespace slim::configuration_handler {
	using namespace slim::common;
	v8::Isolate* isolate;
	std::atomic<bool> log_startup_debug = false;
	std::atomic<bool> log_startup_trace = false;
	std::atomic<bool> system_logging_enabled = false;
	std::string default_configuration_file_name = "./slim_configuration.json";
	std::string default_configuration_file = std::filesystem::current_path().string() + std::filesystem::path::preferred_separator + default_configuration_file_name;
	std::string logging_consumers_map = "logging_consumers";
	std::string logging_consumer_files_map = "logging_consumer_files";
	std::string logging_consumer_file_functions_map = "logging_consumer_file_functions";
	static bool process_logging_object(v8::Local<v8::Object>& _v8_logging_object);
}
[[maybe_unused]]
bool slim::configuration_handler::can_log(std::string_view _consumer, std::string_view _log_level, std::string_view _file, std::string_view _function) {
	if(log_startup_debug || log_startup_trace) {
		return ((log_startup_debug && _log_level == "debug") || (log_startup_trace && _log_level == "trace"));
	}

	if(!system_logging_enabled) {
		return false;
	}

	if(memory_mapper::read_bool(logging_consumers_map, std::format("{}/{}", _consumer, _log_level))) {
		if(memory_mapper::read_bool(logging_consumers_map, std::format("{}/{}", _consumer, "continue"))) {
			if(memory_mapper::read_bool(logging_consumer_files_map, std::format("{}/{}/{}", _consumer, _file, _log_level))) {
				if(memory_mapper::read_bool(logging_consumer_files_map, std::format("{}/{}/{}", _consumer, _file, "continue"))) {
					if(memory_mapper::read_bool(logging_consumer_file_functions_map, std::format("{}/{}/{}/{}", _consumer, _file, _function, _log_level))) {
						return true;
					}
					return false;
				}
				return true;
			}
			return false;
		}
		return true;
	}
	return false;
}
void slim::configuration_handler::disable_startup_logging() {
	log_startup_debug = false;
	log_startup_trace = false;
}
void slim::configuration_handler::load() {
	log::trace(log::Message(__func__,"begin",__FILE__, __LINE__));
	bool turn_system_logging_on = false;

	isolate = slim::v_8::new_isolate("configuration_loader");
	if(!isolate->IsDead()) {
		{ // top of isolate scope
			v8::Isolate::Scope isolate_scope(isolate);
			log::debug(log::Message(__func__,"created isolate scope",__FILE__, __LINE__));

			v8::HandleScope handle_scope(isolate);
			log::debug(log::Message(__func__,"created handle scope",__FILE__, __LINE__));

			auto context = v8::Context::New(isolate);
			log::debug(log::Message(__func__,"created new context",__FILE__, __LINE__));

			v8::Context::Scope context_scope(context);
			log::debug(log::Message(__func__,"created context_scope",__FILE__, __LINE__));

			log::debug(log::Message(__func__,"finding default configuration file => " + default_configuration_file,__FILE__, __LINE__));
			std::string configuration_file = default_configuration_file;

			if(std::filesystem::exists(configuration_file)) {
				auto file_url_string = "file://" + std::filesystem::canonical(configuration_file).string();
				log::debug(log::Message(__func__, std::format("configuration file found => {}", file_url_string),__FILE__, __LINE__));	

				auto web_file = slim::common::WebFile(file_url_string);
				log::debug(log::Message(__func__, std::format("loaded configuration file => {} => size => {}", web_file.request().url().url().value_or("not set"), std::to_string(web_file.size())),__FILE__, __LINE__));

				if(web_file.size() > 0) {
					log::debug(log::Message(__func__, std::format("configuration contents => {}", web_file.to_string()),__FILE__, __LINE__));

					auto imported_configuration_v8_maybe_value = v8::JSON::Parse(context, slim::utilities::StringToV8String(isolate, web_file.to_string()));
					log::debug(log::Message(__func__, "configuration contents parsed",__FILE__, __LINE__));

					if(!imported_configuration_v8_maybe_value.IsEmpty()) {
						v8::Local<v8::Value> imported_configuration_value;
						if(imported_configuration_v8_maybe_value.ToLocal(&imported_configuration_value)) {
							if(!imported_configuration_value.IsEmpty()) {
								log::debug(log::Message(__func__, "converting Local<v8::Value> to Local<v8::Object>",__FILE__, __LINE__));

								auto configuration_object = slim::utilities::GetObject(isolate, imported_configuration_value);
								log::debug(log::Message(__func__, "converted Local<v8::Value> to Local<v8::Object>",__FILE__, __LINE__));

								if(!configuration_object.IsEmpty()) {
									if(configuration_object->Has(context, slim::utilities::StringToV8Value(isolate, "logging")).FromMaybe(false)) {
										auto logging_v8_object = slim::utilities::GetObject(isolate, "logging", configuration_object);
										turn_system_logging_on = process_logging_object(logging_v8_object);
									}
									else {
										log::debug(log::Message(__func__, std::format("configuration section not found => {} ", "logging"),__FILE__, __LINE__));
									}
								}
								else {
									log::error(log::Message(__func__, "configuration seems to be empty => should never be reached",__FILE__, __LINE__));
								}
							}
							else {
								log::error(log::Message(__func__, "configuration seems to be empty => should never be reached",__FILE__, __LINE__));
							}
						}
						else {
							log::error(log::Message(__func__, "MaybeLocal<v8::Value> was not converted to Local<v8::Value>",__FILE__, __LINE__));
						}
					}
					else {
						log::debug(log::Message(__func__, "configuration object is empty",__FILE__, __LINE__));
					}
				}
			}
			else {
				log::debug(log::Message(__func__, std::format("configuration file not found => {}", default_configuration_file),__FILE__, __LINE__));	
			}
		} // bottom of isolate scope
		isolate->Dispose();
		log::debug(log::Message(__func__, "isolate disposed",__FILE__, __LINE__));
	}
	else {
		log::error(log::Message(__func__, "received in-valid isolate",__FILE__, __LINE__));
	}

	log::trace(log::Message(__func__, "ends",__FILE__, __LINE__));
	system_logging_enabled = turn_system_logging_on;
}
void slim::configuration_handler::log_startup_tasks_debug(bool _bool) {
	log_startup_debug = _bool;
}
void slim::configuration_handler::log_startup_tasks_trace(bool _bool) {
	log_startup_trace = _bool;
}
static bool slim::configuration_handler::process_logging_object(v8::Local<v8::Object>& _v8_logging_object) {
	log::trace(log::Message(__func__, "begins",__FILE__, __LINE__));
	std::unordered_set<std::string> configuration_scope_variables = {"print_color", "debug", "error", "trace", "continue"};
	bool turn_system_logging_on = false;
	if(!_v8_logging_object.IsEmpty()) {
		log::debug(log::Message(__func__, std::format("configuration section found => {} ", "logging"),__FILE__, __LINE__));

		turn_system_logging_on = slim::utilities::V8ValueToBool(isolate, slim::utilities::GetValue(isolate, "system_logging_enabled", _v8_logging_object));
		log::debug(log::Message(__func__, std::format("system logging enabled => {}", slim::common::utilities::to_string(turn_system_logging_on)),__FILE__, __LINE__));

		auto v8_logging_consumers_object = slim::utilities::GetObject(isolate, "consumers", _v8_logging_object);
		log::debug(log::Message(__func__, "created logging/consumers object",__FILE__, __LINE__));

		if(!v8_logging_consumers_object.IsEmpty() && v8_logging_consumers_object->IsArray()) {
			log::debug(log::Message(__func__, std::format("configuration section found => {} => size => {}", "consumers", v8_logging_consumers_object.As<v8::Array>()->Length()),__FILE__, __LINE__));

			for(int consumers_array_index = 0; consumers_array_index < v8_logging_consumers_object.As<v8::Array>()->Length(); consumers_array_index++) {
				auto consumer_object = slim::utilities::GetObject(isolate, v8_logging_consumers_object.As<v8::Array>(), consumers_array_index);
				std::string consumer_name = slim::utilities::v8ValueToString(isolate, slim::utilities::GetValue(isolate, "name", consumer_object));

				if(!consumer_object.IsEmpty()) {
					log::debug(log::Message(__func__, std::format("configuration section found => {} ", consumer_name),__FILE__, __LINE__));

					for(const auto& scope_variable : configuration_scope_variables) {
						const auto value = slim::utilities::V8ValueToBool(isolate, slim::utilities::GetValue(isolate, scope_variable, consumer_object));
						std::string key = std::format("{}/{}", consumer_name, scope_variable);
						memory_mapper::write(logging_consumers_map, key, value);
						log::debug(log::Message(__func__, std::format("wrote {} => {}", key, slim::common::utilities::to_string(value))  ,__FILE__, __LINE__));
					}

					auto files_object = slim::utilities::GetObject(isolate, "file_list", consumer_object);

					if(!files_object.IsEmpty() && files_object->IsArray()) {
						log::debug(log::Message(__func__, std::format("configuration section found => {} => size => {}", "file_list", files_object.As<v8::Array>()->Length()),__FILE__, __LINE__));

						for(int files_array_index = 0; files_array_index < files_object.As<v8::Array>()->Length(); files_array_index++) {
							auto file_object = slim::utilities::GetObject(isolate, files_object.As<v8::Array>(), files_array_index);
							std::string file_name = slim::utilities::v8ValueToString(isolate, slim::utilities::GetValue(isolate, "name", file_object));

							for(const auto& scope_variable : configuration_scope_variables) {
								const auto value = slim::utilities::V8ValueToBool(isolate, slim::utilities::GetValue(isolate, scope_variable, file_object));
								std::string key = std::format("{}/{}/{}", consumer_name, file_name, scope_variable);
								memory_mapper::write(logging_consumer_files_map, key, value);
								log::debug(log::Message(__func__, std::format("wrote {} => {}", key, slim::common::utilities::to_string(value))  ,__FILE__, __LINE__));
							}

							auto functions_object = slim::utilities::GetObject(isolate, "function_list", file_object);
							
							if(!functions_object.IsEmpty() && functions_object->IsArray()) {
								log::debug(log::Message(__func__, std::format("configuration section found => {} => size => {}", "function_list", functions_object.As<v8::Array>()->Length()),__FILE__, __LINE__));

								for(int functions_array_index = 0; functions_array_index < functions_object.As<v8::Array>()->Length(); functions_array_index++) {
									auto function_object = slim::utilities::GetObject(isolate, functions_object.As<v8::Array>(), functions_array_index);
									std::string function_name = slim::utilities::v8ValueToString(isolate, slim::utilities::GetValue(isolate, "name", function_object));

									for(const auto& scope_variable : configuration_scope_variables) {
										const auto value = slim::utilities::V8ValueToBool(isolate, slim::utilities::GetValue(isolate, scope_variable, function_object));
										std::string key = std::format("{}/{}/{}/{}", consumer_name, file_name, function_name, scope_variable);
										memory_mapper::write(logging_consumer_file_functions_map, key, value);
										log::debug(log::Message(__func__, std::format("wrote {} => {}", key, slim::common::utilities::to_string(value))  ,__FILE__, __LINE__));
									}
								}
							}
							else {
								log::debug(log::Message(__func__, std::format("configuration section not populated => {}", "function_list"),__FILE__, __LINE__));
							}
						}
					}
					else {
						log::debug(log::Message(__func__, std::format("configuration section not populated => {}", "file_list"),__FILE__, __LINE__));
					}
				}
				else {
					log::debug(log::Message(__func__, std::format("configuration section not populated => {}", consumer_name),__FILE__, __LINE__));
				}
			}
		}
		else {
			log::debug(log::Message(__func__, std::format("configuration section not populated => {}", "consumers"),__FILE__, __LINE__));
		}
	}
	else {
		log::debug(log::Message(__func__, std::format("configuration section not populated => {}", "logging"),__FILE__, __LINE__));
	}
	log::debug(log::Message(__func__, std::format("configuration section => {} => has been parsed", "logging"),__FILE__, __LINE__));
	log::trace(log::Message(__func__, "ends",__FILE__, __LINE__));
	return turn_system_logging_on;
}
void slim::configuration_handler::set_startup_logging(int argc, char* argv[]) {
	std::vector<std::string> args(argv + 1, argv + argc);
    for(const auto& arg : args) {
        if(arg == "-v") {
            slim::configuration_handler::log_startup_tasks_trace(true);
			log::trace(log::Message(__func__, "trace message output begins",__FILE__, __LINE__));
        }
        else if(arg == "-vv") {
            slim::configuration_handler::log_startup_tasks_trace(true);
			log::trace(log::Message(__func__, "trace message output begins",__FILE__, __LINE__));
            slim::configuration_handler::log_startup_tasks_debug(true);
			log::trace(log::Message(__func__, "debug message output begins",__FILE__, __LINE__));
        }
    }
	log::trace(log::Message(__func__, "ends",__FILE__, __LINE__));
}