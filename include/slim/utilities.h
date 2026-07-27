 #ifndef __SLIM__UTILITIES__H
#define __SLIM__UTILITIES__H
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <v8.h>
namespace slim::utilities {
    std::string print_property_type(v8::Isolate* isolate, v8::Local<v8::Value> property_name, v8::Local<v8::Value> property_value);
    void print_v8_array_buffer(v8::Isolate* isolate, const v8::Local<v8::ArrayBuffer>& array_buffer);
    void print_v8_object_keys(v8::Isolate* isolate, const v8::Local<v8::Object>& object_value);
    bool BoolValue(v8::Isolate* isolate, v8::Local<v8::Value> value);  
    int ArrayCount(v8::Local<v8::Value> value);
    // old
    int IntValue(v8::Isolate* isolate, v8::Local<v8::Value> value);
    // new
    int V8ValueToInt(v8::Isolate* isolate, v8::Local<v8::Value> value);

    int IntValue(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object);
    int IntValuePositive(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object);
    int PropertyCount(v8::Isolate* isolate, v8::Local<v8::Object> object);
    int ScriptLineNumber(v8::Local<v8::Message> message);
    double NumberValue(v8::Isolate* isolate, v8::Local<v8::Value> value);

    template <typename Thing>
    v8::Local<v8::Object> GetObject(v8::Isolate* isolate, Thing object);
    v8::Local<v8::Object> GetObject(v8::Isolate* isolate, v8::Local<v8::Array> array, int array_index);
    v8::Local<v8::Object> GetObject(v8::Isolate* isolate, v8::Local<v8::Value> object);
    v8::Local<v8::Object> GetObject(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object);
    v8::Local<v8::Value> GetValue(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object);
    void V8KeysToVector(v8::Isolate* isolate, std::vector<v8::Local<v8::String>>& vector_to_fill, v8::Local<v8::Object> object);

    std::string ScriptFileName(v8::Local<v8::Message> message); 
    std::string ScriptLine(v8::Local<v8::Message> message);
    std::string ScriptStackTrace(v8::TryCatch* try_catch);
    
    std::string SlimColorValue(v8::Isolate* isolate, v8::Local<v8::Value> value, std::vector<std::string>& colors);
    
    std::string StringFunction(v8::Isolate* isolate, v8::Local<v8::Function> function);
    v8::Local<v8::Name> StringToName(v8::Isolate* isolate, std::string string);
    v8::Local<v8::String> StringToString(v8::Isolate* isolate, std::string string);
    v8::Local<v8::Value> StringToValue(v8::Isolate* isolate, std::string string);
    template <typename Thing>
    std::string StringValue(v8::Isolate* isolate, Thing thingy);
    std::string StringValue(v8::Isolate* isolate, v8::Local<v8::Value> value);
    std::string StringValue(v8::Isolate* isolate, v8::Local<v8::String> string);
    std::string StringValue(v8::Isolate* isolate, std::string string, v8::Local<v8::String> value);
    std::string StringValue(v8::Isolate* isolate, std::string string, v8::Local<v8::Object> object);
    std::string StringFunction(v8::Isolate* isolate, v8::Local<v8::Value> function);
    /* new function formats */
    v8::Local<v8::Boolean> BoolToV8Boolean(v8::Isolate* isolate, bool value);
    v8::Local<v8::Value> CharPointerToV8Value(v8::Isolate* isolate, const char* value);
    v8::Local<v8::Number> DoubleToV8Number(v8::Isolate* isolate, const double value);
    v8::Local<v8::Integer> IntToV8Integer(v8::Isolate* isolate, const int value);
    v8::Local<v8::Integer> size_t_ToV8Integer(v8::Isolate* isolate, const size_t value);

    v8::Local<v8::String> StringToV8String(v8::Isolate* isolate, std::string string);
    v8::Local<v8::String> StringViewToV8String(v8::Isolate* isolate, std::string_view string);
    v8::Local<v8::Name> StringToV8Name(v8::Isolate* isolate, const std::string value);
    v8::Local<v8::Value> StringToV8Value(v8::Isolate* isolate, const std::string value);
    v8::Local<v8::Value> StringToV8Value(v8::Isolate* isolate, const std::string* value);

    bool V8BoolToBool(v8::Isolate* isolate, v8::Maybe<bool> value);
    std::string V8JsonValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value);
    std::string v8NameToString(v8::Isolate* isolate, v8::Local<v8::Name> string);
    std::string v8StringToString(v8::Isolate* isolate, v8::Local<v8::String> string);
    bool V8ValueToBool(v8::Isolate* isolate, v8::Local<v8::Value> value);
    std::string v8ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value);
    double time_spec_to_double(const struct timespec& time_spec_struct);
    std::string time_spec_to_time_string_gmt(const timespec& time_spec_struct);
}
#endif
