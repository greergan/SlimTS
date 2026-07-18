#include <iostream>

#include <v8.h>
#include <slim/builtins.h>
#include <slim/builtins/dummy_console_provider.h>
#include <slim/common/log.h>
#include <slim/common/memory/mapper.h>
#include <slim/plugin.hpp>
#include <slim/plugin/loader.h>
/* look at source file product/google/v8/src/init/bootstrapper.cc */
namespace {
	using namespace slim::common;
	using namespace slim::utilities;
}
//void slim::builtins::initialize(v8::Isolate* isolate) {
void slim::builtins::initialize(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate>& globalObjectTemplate) {
	log::trace(log::Message("slim::builtins::initialize()","begins",__FILE__, __LINE__));
	//v8::HandleScope scope(isolate);
	log::trace(log::Message("slim::builtins::initialize()","after handle scope",__FILE__, __LINE__));
	slim::dummy_console::expose_plugin(isolate);
	log::trace(log::Message("slim::builtins::initialize()","after expose_plugin",__FILE__, __LINE__));
	slim::plugin::plugin slim_objects(isolate, "slim");
	log::trace(log::Message("slim::builtins::initialize()","after slim",__FILE__, __LINE__));
	slim_objects.add_function("load", slim::plugin::loader::load);
	log::trace(log::Message("slim::builtins::initialize()","after load",__FILE__, __LINE__));
	slim_objects.expose_plugin();
	log::trace(log::Message("slim::builtins::initialize()","after expose slim_objects",__FILE__, __LINE__));
	log::trace(log::Message("slim::builtins::initialize()","ends",__FILE__, __LINE__));
}
