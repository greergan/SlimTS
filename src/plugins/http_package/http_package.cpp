#include <algorithm>
#include <string>
#include <v8.h>
#include <slim/common/fetch.h>
#include <slim/common/http/headers.h>
#include <slim/common/http/parser.h>
#include <slim/common/http/request.h>
#include <slim/common/http/response.h>
#include <slim/common/log.h>
#include <slim/common/network/address/set.h>
#include <slim/common/web_file.h>
#include <slim/utilities.h>
namespace slim::package::http {
	using namespace slim;
	using namespace slim::common;
	void fetch(const v8::FunctionCallbackInfo<v8::Value>& args);
}
void slim::package::http::fetch(const v8::FunctionCallbackInfo<v8::Value>& args) {
	log::trace(log::Message("slim::package::http::fetch()", "begins",__FILE__,__LINE__));
	auto* isolate = args.GetIsolate();
	v8::HandleScope isolate_scope(isolate);
	if(args[0].IsEmpty()) {
		isolate->ThrowException(utilities::StringToV8String(isolate, "slim::package::http::fetch() expects a string argument or a request object"));
		return;
	}
	auto context = isolate->GetCurrentContext();
	auto maybe_resolver = v8::Promise::Resolver::New(context);
	if(maybe_resolver.IsEmpty()) {
		isolate->ThrowException(utilities::StringToV8String(isolate, "slim::package::http::fetch() failed to create promise"));
		return;
	}
	auto resolver = maybe_resolver.ToLocalChecked();
	args.GetReturnValue().Set(resolver->GetPromise());
	auto v8_response_object = v8::Object::New(isolate);
	if(args[0]->IsObject()) {

	}
	else {
		log::debug(log::Message("slim::package::http::fetch()", "fetching file",__FILE__,__LINE__));
 		auto file_to_fetch_string = utilities::v8ValueToString(isolate, args[0]);
		WebFile web_file = WebFile(slim::common::http::Request(file_to_fetch_string));
		log::trace(log::Message("slim::package::http::fetch()", "fetched, url => " + web_file.request().url() + ", file size => " + std::to_string(web_file.data()->size()),__FILE__,__LINE__));
		bool response_ok_bool = web_file.response().response_code() >= 200 && web_file.response().response_code() < 300;
		v8_response_object->Set(context, slim::utilities::StringToV8Name(isolate, "ok"), slim::utilities::BoolToV8Boolean(isolate, response_ok_bool));
		v8_response_object->Set(context, slim::utilities::StringToV8Name(isolate, "url"), slim::utilities::StringToV8String(isolate, web_file.request().url()));
		v8_response_object->Set(context, slim::utilities::StringToV8Name(isolate, "status"), slim::utilities::IntToV8Integer(isolate, web_file.response().response_code()));
		v8_response_object->Set(context, slim::utilities::StringToV8Name(isolate, "statusText"), slim::utilities::StringToV8String(isolate, web_file.response().response_code_text()));
		v8::Local<v8::Object> headers_object = v8::Object::New(isolate);
		for(auto& header_pair : web_file.response().headers().get()) {
			headers_object->Set(context, slim::utilities::StringToV8Name(isolate, header_pair.first), slim::utilities::StringToV8String(isolate, header_pair.second));
		}
		v8_response_object->Set(context, slim::utilities::StringToV8Name(isolate, "headers"), headers_object).ToChecked();
		v8::Local<v8::Array> data_array = v8::Array::New(isolate, web_file.data()->size());
		for(size_t i = 0; i < web_file.data()->size(); i++) {
			data_array->Set(context, static_cast<uint32_t>(i), v8::Integer::New(isolate, web_file.data()->at(i))).ToChecked();
		}
		v8_response_object->Set(context, slim::utilities::StringToV8Name(isolate, "data"), data_array).ToChecked();
		log::debug(log::Message("slim::package::http::fetch()", "fetched, url => " + web_file.request().url() + ", file size => " + std::to_string(web_file.data()->size()) ,__FILE__,__LINE__));
	}
	auto result_of_resolve = resolver->Resolve(context, v8_response_object);
	//log::trace(log::Message("slim::package::http::fetch()", "ends, url => " + web_file->request().url() + ", file size => " + std::to_string(web_file->data()->size()),__FILE__,__LINE__));
}
extern "C" void expose_plugin(v8::Isolate* isolate) {
	using namespace slim::common;
	v8::HandleScope isolate_scope(isolate);
	auto context = isolate->GetCurrentContext();
	log::trace(log::Message("slim::package::http::expose_plugin()", "begins",__FILE__,__LINE__));
	auto fetch_function_template = v8::FunctionTemplate::New(isolate, slim::package::http::fetch);
	auto fetch_function = fetch_function_template->GetFunction(context).ToLocalChecked();
	context->Global()->Set(context, slim::utilities::StringToV8Name(isolate, "fetch"), fetch_function).ToChecked();
	log::trace(log::Message("slim::package::http::expose_plugin()", "ends",__FILE__,__LINE__));
}
