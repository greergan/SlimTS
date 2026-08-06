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
	int start_col = message->GetStartColumn();
	auto maybe_source_line = message->GetSourceLine(context);

#ifdef ENABLE_LOGGING
	log::debug({"script_origin.ScriptId()", std::to_string(script_origin.ScriptId()),__FILE__, __LINE__});
	log::debug({"script_origin.ColumnOffset()", std::to_string(script_origin.ColumnOffset()),__FILE__, __LINE__});
	log::debug({"script_origin.LineOffset()", std::to_string(script_origin.LineOffset()),__FILE__, __LINE__});
	log::debug({"script_origin.ResourceName()", utilities::v8ValueToString(isolate, script_origin.ResourceName()),__FILE__, __LINE__});
	log::debug({"message->Get()", utilities::v8StringToString(isolate, message->Get()),__FILE__, __LINE__});
	log::debug({"message->GetScriptResourceName()", utilities::v8ValueToString(isolate, message->GetScriptResourceName()),__FILE__, __LINE__});
	log::debug({"message->GetLineNumber()", std::to_string(line_number), __FILE__, __LINE__});
	log::debug({"message->GetSourceLine() empty", maybe_source_line.IsEmpty() ? "true" : "false", __FILE__, __LINE__});
	log::debug({"message->ErrorLevel()", std::to_string(message->ErrorLevel()),__FILE__, __LINE__});
	log::debug({"message->GetStartColumn()", std::to_string(start_col),__FILE__, __LINE__});
	log::debug({"message->GetEndColumn()", std::to_string(message->GetEndColumn()),__FILE__, __LINE__});
	log::debug({"message->GetStartPosition()", std::to_string(message->GetStartPosition()),__FILE__, __LINE__});
	log::debug({"message->GetEndPosition()", std::to_string(message->GetEndPosition()),__FILE__, __LINE__});
	log::debug({"stack_trace empty", stack_trace.IsEmpty() ? "true" : "false", __FILE__, __LINE__});
#endif
	std::string exception_string;
	if(!script_origin.ResourceName()->IsUndefined()) {
#ifdef ENABLE_LOGGING
		log::debug({"script_origin.ResourceName() defined", utilities::v8ValueToString(isolate, script_origin.ResourceName()), __FILE__, __LINE__});
#endif
		exception_string += std::format("\n{}", utilities::v8ValueToString(isolate, script_origin.ResourceName()));
	}
	if(line_number > 0) {
#ifdef ENABLE_LOGGING
		log::debug({"line_number > 0", std::to_string(line_number), __FILE__, __LINE__});
#endif
		exception_string += std::format("\nLine number: {}\n", line_number);
	}
	if(try_catch->HasCaught()) {
#ifdef ENABLE_LOGGING
		log::debug({"try_catch->Exception()", utilities::v8ValueToString(isolate, try_catch->Exception()), __FILE__, __LINE__});
#endif
		exception_string += std::format("\n{}\n", utilities::v8ValueToString(isolate, try_catch->Exception()));
	}
	if(!maybe_source_line.IsEmpty()) {
#ifdef ENABLE_LOGGING
		log::debug({"message->GetSourceLine()", utilities::v8StringToString(isolate, maybe_source_line.ToLocalChecked()), __FILE__, __LINE__});
#endif
		exception_string += std::format("{}\n", utilities::v8StringToString(isolate, maybe_source_line.ToLocalChecked()));
	}
	if(start_col >= 0) {
#ifdef ENABLE_LOGGING
		log::debug({"start_col caret at", std::to_string(start_col), __FILE__, __LINE__});
#endif
		exception_string += std::string(start_col, ' ');
		exception_string += "^\n";
	}
	if(!stack_trace.IsEmpty()) {
#ifdef ENABLE_LOGGING
		log::debug({"stack_trace", utilities::v8ValueToString(isolate, stack_trace), __FILE__, __LINE__});
#endif
		exception_string += std::format("\nStackTrace:\n{}", utilities::v8ValueToString(isolate, stack_trace));
	}
#ifdef ENABLE_LOGGING
	log::trace({"slim::exception_handler::try_catch_handler()","ends",__FILE__, __LINE__});
#endif
	throw SlimException(exception_string.c_str());
}
