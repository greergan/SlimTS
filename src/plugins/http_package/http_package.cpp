#include <algorithm>
#include <format>
#include <string>
#include <vector>

#include <v8.h>

#include <slim/common/http_url.h>
#include <slim/common/log.h>
#include <slim/common/web_file.h>

#include <slim/exception_handler.h>
#include <slim/http_response_codes.hpp>

#include <slim/utilities.h>

#include <slim/SlimValue.hpp>


/* 
void ResponseText(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    auto* state = static_cast<ResponseState*>(
        args.This()->GetAlignedPointerFromInternalField(0)
    );

    // Create Promise
    v8::Local<v8::Promise::Resolver> resolver =
        v8::Promise::Resolver::New(context).ToLocalChecked();

    v8::Local<v8::Promise> promise = resolver->GetPromise();

    // Capture data for async resolution
    std::string body = state->body;

    // Schedule microtask (WHATWG-style async resolution)
    isolate->EnqueueMicrotask(
        [resolver = v8::Global<v8::Promise::Resolver>(isolate, resolver),
         body](void* data) mutable {

            v8::Isolate* isolate = static_cast<v8::Isolate*>(data);
            v8::HandleScope handle_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();

            v8::Local<v8::Promise::Resolver> local_resolver =
                resolver.Get(isolate);

            local_resolver->Resolve(context,
                v8::String::NewFromUtf8(isolate, body.c_str())
                    .ToLocalChecked()
            ).Check();
        },
        isolate
    );

    args.GetReturnValue().Set(promise);
} */



namespace slim::package::http {
	using namespace slim;
	using namespace slim::common;

	void fetch(const v8::FunctionCallbackInfo<v8::Value>& args);

	v8::Local<v8::Object> create_object_storage(v8::Isolate* _isolate, const int _count);
	void response_error(const v8::FunctionCallbackInfo<v8::Value>& args);
	void create_error_function(v8::Isolate* _isolate, v8::Object& _response);
}

v8::Local<v8::Object> slim::package::http::create_object_storage(v8::Isolate* _isolate, const int _count) {
    if(_count <= 0) {
        return v8::Object::New(_isolate);
    }
	auto context = _isolate->GetCurrentContext();
	v8::Local<v8::ObjectTemplate> object_template = v8::ObjectTemplate::New(_isolate);
    object_template->SetInternalFieldCount(_count); 
    auto maybe_obj = object_template->NewInstance(context);
    v8::Local<v8::Object> obj;

    if(!maybe_obj.ToLocal(&obj)) {
		return v8::Object::New(_isolate);
    }

    for(int i = 0; i < _count; i++) {
		obj->SetInternalField(i, v8::Undefined(_isolate));
    }

	return obj;
}


void slim::package::http::response_error(const v8::FunctionCallbackInfo<v8::Value>& args) {


	//std::vector<std::string>& original_vec = *reinterpret_cast<std::vector<std::string>*>(ptr);
	//void* ptr = obj->GetAlignedPointerFromInternalField(1);  

}

void slim::package::http::create_error_function(v8::Isolate* _isolate, v8::Object& _response) {
	auto context = _isolate->GetCurrentContext();

	auto function_template = v8::FunctionTemplate::New(_isolate, slim::package::http::response_error);
	log::debug(log::Message(__func__, "created function template for => error",__FILE__,__LINE__));

	auto function = function_template->GetFunction(context).ToLocalChecked();
	log::debug(log::Message(__func__, "created function from fetch template for => error",__FILE__,__LINE__));

	context->Global()->Set(context, slim::utilities::StringToV8Name(_isolate, "error"), function).ToChecked();
	log::debug(log::Message(__func__, "added global context function => error()",__FILE__,__LINE__));
}

void define_readonly_property(v8::Isolate* _isolate, v8::Local<v8::Object>& _object, std::string _key, std::string_view _value) {
	_object->DefineOwnProperty(_isolate->GetCurrentContext(), slim::utilities::StringToV8Name(_isolate, _key), slim::utilities::StringViewToV8String(_isolate, _value), v8::ReadOnly);
}



void slim::package::http::fetch(const v8::FunctionCallbackInfo<v8::Value>& args) {
	log::trace(log::Message(__func__, "begins",__FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("received => {} arguments", std::to_string(args.Length())),__FILE__,__LINE__));
	auto* isolate = args.GetIsolate();
	v8::HandleScope isolate_scope(isolate);

	v8::TryCatch try_catch(isolate);
	auto context = isolate->GetCurrentContext();
	auto resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
	args.GetReturnValue().Set(resolver->GetPromise());

	auto errors_object = v8::Object::New(isolate);

	std::vector<std::string> errors = {};
	auto response = create_object_storage(isolate, 2);
	log::debug(log::Message(__func__, "checking received => arguments => count",__FILE__,__LINE__));
	if(args.Length() == 0) {
		log::debug(log::Message(__func__, "expects [string|Request|URL]",__FILE__,__LINE__));
		define_readonly_property(isolate, errors_object, "invalid_argument(s)", "expects [string|Request|URL]");
		define_readonly_property(isolate, response, "statusText", httpcodes::to_string(404));
		response->DefineOwnProperty(context, slim::utilities::StringToV8Name(isolate, "status"), slim::utilities::IntToV8Integer(isolate, 404), v8::ReadOnly).Check();
		response->DefineOwnProperty(context, slim::utilities::StringToV8Name(isolate, "url"), v8::Undefined(isolate), v8::ReadOnly).Check();
	}
	else {
		v8::Local<v8::Value> arg = args[0];
		log::debug(log::Message(__func__, "converted => argument => 0",__FILE__,__LINE__));
		log::debug(log::Message(__func__, std::format("argument => 0 => IsEmpty()", std::to_string(arg.IsEmpty())),__FILE__,__LINE__));

		if(!arg.IsEmpty()) {
			log::debug(log::Message(__func__, "argument => 0 => ",__FILE__,__LINE__));

			auto file = utilities::v8ValueToString(isolate, arg);
			log::debug(log::Message(__func__, std::format("checking for URL().can_parse => {} arguments", file),__FILE__,__LINE__));
			auto can_parse = slim::common::http::URL().can_parse(file);

			if(!can_parse) {
				define_readonly_property(isolate, errors_object, "parse_error", "URL [string|Request|URL] not parsable");
				if(can_parse.has_map("errors")) {
					for(auto const& [key, error] : can_parse.get_map("errors").get()) {
						define_readonly_property(isolate, errors_object, key, error.get<std::string>());
					}
				}
				log::debug(log::Message(__func__, std::format("URL [string|Request|URL] not parsable", file),__FILE__,__LINE__));
			}
			else {
throw("crap");
				log::debug(log::Message(__func__, std::format("URL [string] parsable", file),__FILE__,__LINE__));
				auto url = slim::common::http::URL(file);
				log::debug(log::Message(__func__, std::format("URL path => {}", url.pathname().value()),__FILE__,__LINE__));
				log::debug(log::Message(__func__, std::format("fetching => {}", url.url().value()),__FILE__,__LINE__));
				WebFile web_file = WebFile(slim::common::http::Request(file));
				log::debug(log::Message(__func__, std::format("fetched  => {} => size => {}", 
					web_file.request().url().url().value_or("not set"),std::to_string(web_file.data()->size())),__FILE__,__LINE__));

				bool response_ok_bool = web_file.response().response_code() >= 200 && web_file.response().response_code() < 300;
				response->Set(context, slim::utilities::StringToV8Name(isolate, "ok"), slim::utilities::BoolToV8Boolean(isolate, response_ok_bool));
				response->Set(context, slim::utilities::StringToV8Name(isolate, "url"), slim::utilities::StringToV8String(isolate, web_file.request().url().url().value_or("not set")));
				response->Set(context, slim::utilities::StringToV8Name(isolate, "status"), slim::utilities::IntToV8Integer(isolate, web_file.response().response_code()));
				response->Set(context, slim::utilities::StringToV8Name(isolate, "statusText"), slim::utilities::StringToV8String(isolate, web_file.response().response_code_text()));

	/* 		v8::Local<v8::Object> headers_object = v8::Object::New(isolate);
			for(auto& header_pair : web_file.response().headers().get()) {
				headers_object->Set(context, slim::utilities::StringToV8Name(isolate, header_pair.first), slim::utilities::StringToV8String(isolate, header_pair.second));
			}
			response->Set(context, slim::utilities::StringToV8Name(isolate, "headers"), headers_object).ToChecked(); */


			// SetAlignedPointerInInternalField =>  args.This()->SetAlignedPointerInInternalField(0, state);

				v8::Local<v8::Array> data_array = v8::Array::New(isolate, web_file.data()->size());
				for(size_t i = 0; i < web_file.data()->size(); i++) {
					data_array->Set(context, static_cast<uint8_t>(i), v8::Integer::New(isolate, web_file.data()->at(i))).ToChecked();
				}

		// the following is supposed to be a promise return so needs to be shuffled into its own function code
		//response->Set(context, slim::utilities::StringToV8Name(isolate, "bytes"), data_array).ToChecked();
		// the following is supposed to be a promise return so needs to be shuffled into its own function code
				response->Set(context, slim::utilities::StringToV8Name(isolate, "text"), slim::utilities::StringToV8String(isolate, web_file.to_string())).ToChecked();
				log::debug(log::Message(__func__, "fetched, url => " + web_file.request().url().url().value_or("not set") + ", file size => " + std::to_string(web_file.data()->size()) ,__FILE__,__LINE__));	
			}
		}
		log::debug(log::Message(__func__, "processing completed",__FILE__,__LINE__));
	}
/* 	else {
		log::debug(log::Message(__func__, "found !IsString()",__FILE__,__LINE__));
		errors.push_back("expects [string|Request|URL]");
		define_readonly_property(isolate, response, "statusText", httpcodes::to_string(404));
		response->DefineOwnProperty(context, slim::utilities::StringToV8Name(isolate, "status"), slim::utilities::IntToV8Integer(isolate, 404), v8::ReadOnly).Check();
		response->DefineOwnProperty(context, slim::utilities::StringToV8Name(isolate, "url"), v8::Undefined(isolate), v8::ReadOnly).Check();
		log::debug(log::Message(__func__, "expects [string|Request|URL]",__FILE__,__LINE__));
	} */

	response->Set(context, slim::utilities::StringToV8Name(isolate, "errors"), errors_object).ToChecked();

	auto result_of_resolve = resolver->Resolve(context, response);
	if(try_catch.HasCaught()) {
		log::error(log::Message(__func__, "begins",__FILE__,__LINE__));
		slim::exception_handler::v8_try_catch_handler(&try_catch);
	}

	log::trace(log::Message(__func__, "ends",__FILE__,__LINE__));
}
extern "C" void expose_plugin(v8::Isolate* isolate) {
	using namespace slim::common;
	log::trace(log::Message(__func__, "begins",__FILE__,__LINE__));

	v8::HandleScope isolate_scope(isolate);
	auto context = isolate->GetCurrentContext();
	
	auto fetch_function_template = v8::FunctionTemplate::New(isolate, slim::package::http::fetch);
	log::debug(log::Message(__func__, "created function template for => fetch",__FILE__,__LINE__));

	auto fetch_function = fetch_function_template->GetFunction(context).ToLocalChecked();
	log::debug(log::Message(__func__, "created function from fetch template for => fetch",__FILE__,__LINE__));

	context->Global()->Set(context, slim::utilities::StringToV8Name(isolate, "fetch"), fetch_function).ToChecked();
	log::debug(log::Message(__func__, "added global context function => fetch()",__FILE__,__LINE__));

	log::trace(log::Message(__func__, "ends",__FILE__,__LINE__));
}
