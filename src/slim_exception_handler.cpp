#include <sstream>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/exception.h>
#include <slim/exception_handler.h>
#include <slim/utilities.h>

namespace slim::common {}
namespace slim::exception_handler {
	using namespace slim::common;
	using namespace slim::utilities;
}
/* 	void PrintStackTrace(Isolate* isolate, Local<Value> error) {
		if(error->IsObject()) {
			Local<Object> error_obj = error->ToObject(isolate->GetCurrentContext()).ToLocalChecked();
			Local<String> message_key = String::NewFromUtf8Literal(isolate, "message");
			Local<Value> message_value;
			Local<String> stack_key = String::NewFromUtf8Literal(isolate, "stack");
			Local<Value> stack_value;
			if(error_obj->Get(isolate->GetCurrentContext(), StringToV8String(isolate, "stack")).ToLocal(&stack_value) && stack_value->IsString()) {
				cout << "\n" << v8ValueToString(isolate, stack_value) << "\n";
			}
			return;
		}
    	cout << "\n" << v8ValueToString(isolate, error) << "\n";
	}
	void OnPromiseRejected(PromiseRejectMessage message) {
		Isolate* isolate = message.GetPromise()->GetIsolate();
		Local<Value> value = message.GetValue();
		PrintStackTrace(isolate, value);
	} */

void slim::exception_handler::v8_try_catch_handler(v8::TryCatch* try_catch) {
#ifdef ENABLE_LOGGING
	log::trace({"slim::exception_handler::try_catch_handler()","begins",__FILE__, __LINE__});
#endif
	auto* isolate = try_catch->Message()->GetIsolate();
	auto context = isolate->GetCurrentContext();
	auto message = try_catch->Message();
	auto script_origin = message->GetScriptOrigin();
	auto maybe_stack_trace = try_catch->StackTrace(context);
	v8::Local<v8::Value> stack_trace;
	if(!maybe_stack_trace.IsEmpty()) {
		stack_trace = maybe_stack_trace.ToLocalChecked();
	}
	// Extract the line number safely (returns -1 if it fails to get the line number)
	int line_number = message->GetLineNumber(context).FromMaybe(-1);

#ifdef ENABLE_LOGGING
	log::debug({"script_origin.ScriptId()", std::to_string(script_origin.ScriptId()),__FILE__, __LINE__});
	log::debug({"script_origin.ColumnOffset()", std::to_string(script_origin.ColumnOffset()),__FILE__, __LINE__});
	log::debug({"script_origin.LineOffset()", std::to_string(script_origin.LineOffset()),__FILE__, __LINE__});
	log::debug({"script_origin.ResourceName()", utilities::v8ValueToString(isolate, script_origin.ResourceName()),__FILE__, __LINE__});
	log::debug({"message->Get()", utilities::v8StringToString(isolate, message->Get()),__FILE__, __LINE__});
	log::debug({"message->GetScriptResourceName()", utilities::v8ValueToString(isolate, message->GetScriptResourceName()),__FILE__, __LINE__});
	log::debug({"message->GetLineNumber()", std::to_string(line_number), __FILE__, __LINE__});
	log::debug({"message->GetSourceLine()", utilities::v8StringToString(isolate, message->GetSourceLine(context).ToLocalChecked()),__FILE__, __LINE__});
	log::debug({"message->ErrorLevel()", std::to_string(message->ErrorLevel()),__FILE__, __LINE__});
	log::debug({"message->GetStartColumn()", std::to_string(message->GetStartColumn()),__FILE__, __LINE__});
	log::debug({"message->GetEndColumn()", std::to_string(message->GetEndColumn()),__FILE__, __LINE__});
	log::debug({"message->GetStartPosition()", std::to_string(message->GetStartPosition()),__FILE__, __LINE__});
	log::debug({"message->GetEndPosition()", std::to_string(message->GetEndPosition()),__FILE__, __LINE__});
#endif
	std::string exception_string;
	if(!script_origin.ResourceName()->IsUndefined()) {
		exception_string += std::format("\n{}", utilities::v8ValueToString(isolate, script_origin.ResourceName()));
	}
	if(line_number > 0) {
		exception_string += std::format("\nLine number: {}\n", line_number);
	}
	if(try_catch->HasCaught()) {
		exception_string += std::format("\n{}\n", utilities::v8ValueToString(isolate, try_catch->Exception()));
	}
	exception_string += std::format("{}\n", utilities::v8ValueToString(isolate, message->GetSourceLine(context).ToLocalChecked()));
	exception_string += std::string(message->GetStartColumn(), ' ');
	exception_string += "^\n";
	if(!stack_trace.IsEmpty()) {
		exception_string += std::format("\nStackTrace:\n{}", utilities::v8ValueToString(isolate, stack_trace));
	}
#ifdef ENABLE_LOGGING
	log::trace({"slim::exception_handler::try_catch_handler()","ends",__FILE__, __LINE__});
#endif
	throw SlimException(exception_string.c_str());
}
