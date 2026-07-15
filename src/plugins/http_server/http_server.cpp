#include <v8.h>
#include <slim/slim.h>
#include <slim/common/log.h>
#include <slim/common/network/listener.h>
#include <slim/exception_handler.h>
#include <slim/plugin.hpp>
#include <slim/utilities.h>
using namespace slim;
using namespace slim::common;

namespace slim::plugin::http_server {
	static void listen(const v8::FunctionCallbackInfo<v8::Value>& args);
	static void serve(const v8::FunctionCallbackInfo<v8::Value>& args);
}

static void slim::plugin::http_server::listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
	log::trace(log::Message(__func__, "begins",__FILE__,__LINE__));
	auto* isolate = args.GetIsolate();
	auto context = isolate->GetCurrentContext();
	v8::HandleScope handle_scope(isolate);
	v8::Context::Scope context_scope(context);
	bool listening = false;

	v8::TryCatch try_catch(isolate);
	if(try_catch.HasCaught()) {
		log::error(log::Message(__func__, utilities::v8ValueToString(isolate, try_catch.Exception()),__FILE__, __LINE__));
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}

	log::trace(log::Message(__func__, "ends",__FILE__,__LINE__));
	args.GetReturnValue().Set(utilities::BoolToV8Boolean(isolate, listening));
}

static void slim::plugin::http_server::serve(const v8::FunctionCallbackInfo<v8::Value>& args) {
	log::trace(log::Message(__func__, "begins",__FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends",__FILE__,__LINE__));
}

extern "C" void expose_plugin(v8::Isolate* isolate) {
	log::trace(log::Message(__func__, "begins",__FILE__,__LINE__));
	v8::HandleScope handle_scope(isolate);
	log::trace(log::Message(__func__, "begins",__FILE__,__LINE__));
	plugin::plugin http_server_plugin(isolate, "http_server");
	http_server_plugin.add_function("listen", plugin::http_server::listen);
	http_server_plugin.add_function("serve", plugin::http_server::serve);
	http_server_plugin.expose_plugin();
	log::trace(log::Message(__func__, "ends",__FILE__,__LINE__));
}