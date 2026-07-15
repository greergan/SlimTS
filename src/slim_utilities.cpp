#include <cstring>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <v8.h>
#include <slim/common/log.h>
#include <slim/utilities.h>

#include <iostream>
/* 
template <encoding encoding>
uint32_t WriteOneByteString(const char* src,
                            uint32_t src_len,
                            char* dst,
                            uint32_t dst_len) {
  if (dst_len == 0) {
    return 0;
  }

  if (encoding == UTF8) {
    return simdutf::convert_latin1_to_utf8_safe(src, src_len, dst, dst_len);
  } else if (encoding == LATIN1 || encoding == ASCII) {
    const auto size = std::min(src_len, dst_len);
    memcpy(dst, src, size);
    return size;
  } else {
    // TODO(ronag): Add support for more encoding.
    UNREACHABLE();
  }
}
*/
std::string slim::utilities::print_property_type(v8::Isolate* isolate, v8::Local<v8::Value> property_name, v8::Local<v8::Value> property_value) {
	auto local_string = "property name => " + v8ValueToString(isolate, property_name) + " typeof => ";
	if(property_value->IsString()) {
		local_string += "string";
	}
	else if(property_value->IsFunction()) {
		local_string += "function";
	}
	else if(property_value->IsFunctionTemplate()) {
		local_string += "function template";
	}
	else {
		local_string += "unknown";
	}
	return local_string;
}
void slim::utilities::print_v8_array_buffer(v8::Isolate* isolate, const v8::Local<v8::ArrayBuffer>& array_buffer) {
	slim::common::log::debug(slim::common::log::Message("print_v8_array_buffer size => ", std::to_string(array_buffer->ByteLength()).c_str(), "", 0));
	auto backing_store = array_buffer->GetBackingStore();
	unsigned char* array_buffer_data_pointer = static_cast<unsigned char*>(backing_store->Data());
	for(long count = 0; count < array_buffer->ByteLength(); count++) {
/*  		v8::MaybeLocal<v8::Value> value = array_buffer->Get(isolate->GetCurrentContext(), count);
		std::cout << value.ToLocalChecked()->IsString() << std::endl;
		std::string value_string = slim::utilities::v8ValueToString(isolate, value.ToLocalChecked()); */
		std::cout << array_buffer_data_pointer[count];
		//std::cout << value_string << std::endl;
		//char ch = (char)array_buffer_data_pointer[count];
		//slim::common::log::debug(slim::common::log::Message("print_v8_array_buffer => ", &ch, "", 0));
	}
	std::cout << std::endl;
}
void slim::utilities::print_v8_object_keys(v8::Isolate* isolate, const v8::Local<v8::Object>& object_value) {
	auto property_names_array = object_value->GetOwnPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
	for(int array_index = 0; array_index < property_names_array->Length(); array_index++) {
		auto v8_property_name = property_names_array->Get(isolate->GetCurrentContext(), array_index).ToLocalChecked();
		std::string property_name_string = slim::utilities::v8ValueToString(isolate, v8_property_name);
		slim::common::log::debug(slim::common::log::Message("print_object_keys => ", property_name_string.c_str(), "", 0));
	}
}
int slim::utilities::ArrayCount(v8::Local<v8::Value> value) {
	if(value->IsArray()) {
		return v8::Handle<v8::Array>::Cast(value)->Length();
	}
	return 0;
}
template <typename Thing>
v8::Local<v8::Object> slim::utilities::GetObject(v8::Isolate* isolate, Thing thingy) {
	return (thingy->IsObject()) ? thingy->ToObject(isolate->GetCurrentContext()).ToLocalChecked() : v8::Object::New(isolate);
}
v8::Local<v8::Object> slim::utilities::GetObject(v8::Isolate* isolate, v8::Local<v8::Array> array, int array_index) {
	if(array->IsArray() && array_index >= 0 && array_index <= array->Length()) {
		auto maybe_local_value = array->Get(isolate->GetCurrentContext(), array_index);
		if(!maybe_local_value.IsEmpty() && maybe_local_value.ToLocalChecked()->IsObject()) {
			return v8::Local<v8::Object>::Cast(maybe_local_value.ToLocalChecked());
		}
	}
	return v8::Object::New(isolate);
}
v8::Local<v8::Object> slim::utilities::GetObject(v8::Isolate* isolate, v8::Local<v8::Value> object) {
	return (object->IsObject()) ? object->ToObject(isolate->GetCurrentContext()).ToLocalChecked() : v8::Object::New(isolate);
}
v8::Local<v8::Object> slim::utilities::GetObject(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object) {
	return GetObject(isolate, GetValue(isolate, string, object));
}
v8::Local<v8::Value> slim::utilities::GetValue(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object) {
	return object->Get(isolate->GetCurrentContext(), StringToName(isolate, string)).ToLocalChecked();
}
void slim::utilities::V8KeysToVector(v8::Isolate* isolate, std::vector<v8::Local<v8::String>>& vector_to_fill, v8::Local<v8::Object> object) {
	auto property_names_array = object->GetOwnPropertyNames(isolate->GetCurrentContext()).ToLocalChecked();
	for(int array_index = 0; array_index < property_names_array->Length(); array_index++) {
		auto v8_property_name = property_names_array->Get(isolate->GetCurrentContext(), array_index).ToLocalChecked();
		if(!v8_property_name.IsEmpty() && v8_property_name->IsString()) {
			vector_to_fill.push_back(v8_property_name->ToString(isolate->GetCurrentContext()).ToLocalChecked());
		}
	}
}
// old
int slim::utilities::IntValue(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	if(!value->IsInt32()) {
		isolate->ThrowException(slim::utilities::StringToString(isolate, "integer value expected"));
	}  
	return value->Int32Value(isolate->GetCurrentContext()).FromJust();
}
// new
int slim::utilities::V8ValueToInt(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	if(!value->IsInt32()) {
		isolate->ThrowException(slim::utilities::StringToString(isolate, "integer value expected"));
	}  
	return value->Int32Value(isolate->GetCurrentContext()).FromJust();
}
int slim::utilities::IntValuePositive(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object) {
	v8::Local<v8::Value> value = GetValue(isolate, string, object);
	if(value->IsInt32()) {
		return value->Int32Value(isolate->GetCurrentContext()).FromJust();
	}
	return -1;
}
double slim::utilities::NumberValue(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	return value->NumberValue(isolate->GetCurrentContext()).FromJust();
}
int slim::utilities::PropertyCount(v8::Isolate *isolate, v8::Local<v8::Object> object) {
	return object->GetPropertyNames(isolate->GetCurrentContext()).ToLocalChecked()->Length();
}
std::string slim::utilities::ScriptFileName(v8::Local<v8::Message> message) {
	return StringValue(message->GetIsolate(), message->GetScriptOrigin().ResourceName());
}
std::string slim::utilities::ScriptLine(v8::Local<v8::Message> message) {
		return StringValue(message->GetIsolate(), message->GetSourceLine(message->GetIsolate()->GetCurrentContext()).ToLocalChecked());
}
int slim::utilities::ScriptLineNumber(v8::Local<v8::Message> message) {
	return message->GetLineNumber(message->GetIsolate()->GetCurrentContext()).FromJust();
}
std::string slim::utilities::ScriptStackTrace(v8::TryCatch* try_catch) {
	auto isolate = try_catch->Message()->GetIsolate();
	return StringValue(isolate, try_catch->StackTrace(isolate->GetCurrentContext()).ToLocalChecked());
}
bool slim::utilities::BoolValue(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	if(!value->IsBoolean()) {
		isolate->ThrowException(slim::utilities::StringToString(isolate, "boolean value expected"));
	}  
	return value->BooleanValue(isolate);
}
std::string slim::utilities::SlimColorValue(v8::Isolate* isolate, v8::Local<v8::Value> value, std::vector<std::string>& colors) {
	std::string return_value;
	if(value->IsString()) {
		auto color_string = StringValue(isolate, value);
//             for (auto i = std::sregex_iterator(printf_string.begin(), printf_string.end(), format); i != std::sregex_iterator(); ++i) {
//                std::cout << i->str() << '\n';
//            }
		if(std::regex_match(color_string, std::regex(".+;.+;.+"))) {
			std::smatch matches;
			std::regex rgb_code("([0-9]{1,3});([0-9]{1,3});([0-9]{1,3})");
			if(regex_search(color_string, matches, rgb_code)) {
				for(int i = 1; i < 4; i++) {
					int code = stoi(matches[i]);
					if(code < 0 || code > 255) {
						isolate->ThrowException(slim::utilities::StringToString(isolate, color_string + " codes must be 0-255"));
					}
				}
				return_value = color_string;
			}
			else {
				isolate->ThrowException(slim::utilities::StringToString(isolate, color_string + " unsupported rgb format"));
			}
		}
		else {
			auto find_result = std::find(std::begin(colors), std::end(colors), color_string);
			if(find_result != std::end(colors)) {
				return_value = color_string;
			}
			else {
				std::stringstream message;
				message << color_string << " not in supported ASCII colors\n";
				message << "try console.colors()";
				isolate->ThrowException(slim::utilities::StringToString(isolate, message.str()));
			}
		}
	}
	else if(value->IsInt32()) {
		auto int_color = slim::utilities::IntValue(isolate, value);
		if(int_color > 255 || int_color < 0) {
			isolate->ThrowException(slim::utilities::StringToString(isolate, "extended ASCII codes range 0-255"));
		}
		else {
			return_value = std::to_string(int_color);
		}
	}
	return return_value;
}
std::string slim::utilities::StringFunction(v8::Isolate* isolate, v8::Local<v8::Value> function) {
	return StringValue(isolate, function->ToString(isolate->GetCurrentContext()).ToLocalChecked());
}
v8::Local<v8::Name> slim::utilities::StringToName(v8::Isolate* isolate, std::string string) {
	return v8::String::NewFromUtf8(isolate, string.c_str()).ToLocalChecked();
}
v8::Local<v8::String> slim::utilities::StringToString(v8::Isolate* isolate, std::string string) {
	return v8::String::NewFromUtf8(isolate, string.c_str()).ToLocalChecked();
}
v8::Local<v8::Value> slim::utilities::StringToValue(v8::Isolate* isolate, std::string string) {
	return v8::String::NewFromUtf8(isolate, string.c_str()).ToLocalChecked();
}
template <typename Thing>
std::string slim::utilities::StringValue(v8::Isolate* isolate, Thing thingy) {
	v8::String::Utf8Value utf8_value(isolate, thingy);
	return std::string(*utf8_value);
}
std::string slim::utilities::StringValue(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	v8::String::Utf8Value utf8_value(isolate, value);
	return std::string(*utf8_value);	
}
std::string slim::utilities::StringValue(v8::Isolate* isolate, v8::Local<v8::String> string) {
	v8::String::Utf8Value utf8_value(isolate, string);
	return std::string(*utf8_value);	
}
std::string slim::utilities::StringValue(v8::Isolate* isolate, std::string string, v8::Local<v8::String> value) {
	v8::String::Utf8Value utf8_value(isolate, value);
	return std::string(*utf8_value);
}
std::string slim::utilities::StringValue(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object) {
	return StringValue(isolate, GetValue(isolate, string, object));
}
/* New style calls */
v8::Local<v8::Boolean> slim::utilities::BoolToV8Boolean(v8::Isolate* isolate, bool value) {
	return v8::Boolean::New(isolate, value);
}
v8::Local<v8::Value> slim::utilities::CharPointerToV8Value(v8::Isolate* isolate, const char* value) {
	return v8::String::NewFromUtf8(isolate, value).ToLocalChecked().As<v8::Value>();
}
v8::Local<v8::Number> slim::utilities::DoubleToV8Number(v8::Isolate* isolate, const double value) {
	return v8::Number::New(isolate, value);
}
v8::Local<v8::Integer> slim::utilities::IntToV8Integer(v8::Isolate* isolate, const int value) {
	return v8::Integer::New(isolate, value);
}
v8::Local<v8::Integer> slim::utilities::size_t_ToV8Integer(v8::Isolate* isolate, const size_t value) {
	v8::Local<v8::Integer> new_value = v8::Int32::New(isolate, value);
	return new_value;
}
v8::Local<v8::Name> slim::utilities::StringToV8Name(v8::Isolate* isolate, const std::string value) {
	return StringToV8String(isolate, value).As<v8::Name>();
}
v8::Local<v8::String> slim::utilities::StringToV8String(v8::Isolate* isolate, std::string string) {
	return v8::String::NewFromUtf8(isolate, string.c_str()).ToLocalChecked();
}
v8::Local<v8::String> slim::utilities::StringViewToV8String(v8::Isolate* isolate, std::string_view _string) {
	return StringToV8String(isolate, std::string(_string));
}
v8::Local<v8::Value> slim::utilities::StringToV8Value(v8::Isolate* isolate, const std::string value) {
	return v8::String::NewFromUtf8(isolate, value.c_str()).ToLocalChecked().As<v8::Value>();
}
v8::Local<v8::Value> slim::utilities::StringToV8Value(v8::Isolate* isolate, const std::string* value) {
	return v8::String::NewFromUtf8(isolate, value->c_str()).ToLocalChecked().As<v8::Value>();
}
bool slim::utilities::V8BoolToBool(v8::Isolate* isolate, v8::Maybe<bool> value) {
	return value.FromMaybe(false);
}
std::string slim::utilities::V8JsonValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    return(std::string("slim::utilities::V8JsonValueToString not implemented"));
}
std::string slim::utilities::v8NameToString(v8::Isolate* isolate, v8::Local<v8::Name> string) {
	v8::String::Utf8Value string_value(isolate, string);
	return std::string(*string_value);
}
std::string slim::utilities::v8StringToString(v8::Isolate* isolate, v8::Local<v8::String> string) {
	v8::String::Utf8Value string_value(isolate, string);
	return std::string(*string_value);
}
bool slim::utilities::V8ValueToBool(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	if(!value->IsBoolean()) {
		isolate->ThrowException(slim::utilities::StringToString(isolate, "boolean value expected"));
	}  
	return value->BooleanValue(isolate);
}
std::string slim::utilities::v8ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	v8::String::Utf8Value string_value(isolate, value);
	return std::string(*string_value);
}
double slim::utilities::time_spec_to_double(const struct timespec& time_spec_struct) {
	return (double)time_spec_struct.tv_sec + (double)time_spec_struct.tv_nsec / 1000000000.0;
}
std::string slim::utilities::time_spec_to_time_string_gmt(const timespec& time_spec_struct) {
	std::time_t time = time_spec_struct.tv_sec;
	std::tm gmt_tm;
  
  #ifdef _WIN32
	gmtime_s(&gmt_tm, &time); // Windows
  #else
	gmtime_r(&time, &gmt_tm); // POSIX
  #endif
  
	std::stringstream ss;
	ss << std::put_time(&gmt_tm, "%a, %d %b %Y %H:%M:%S GMT");
	return ss.str();
  }
