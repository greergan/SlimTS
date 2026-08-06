#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>
#include <v8.h>
#include <slim/utilities.h>
#include "console.hpp"
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif

namespace slim::common {}
namespace slim::plugin::console {
using namespace slim::common;

void local_print(const v8::FunctionCallbackInfo<v8::Value>& args, Configuration* configuration) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    if(args.Length() == 0) {
        return;
    }
    auto isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    v8::Local<v8::Object> log_message;
    std::stringstream output;
    std::stringstream color_output;
    auto log_time_stamp_epoch = std::chrono::system_clock::now().time_since_epoch();
    std::string log_time_stamp_string = "";
    std::string is_epoch = configuration->time_stamp.time_format;
    transform(is_epoch.begin(), is_epoch.end(), is_epoch.begin(), ::tolower);
    if(is_epoch == "epoch") {
        log_time_stamp_string = std::to_string(log_time_stamp_epoch.count());
        if(configuration->time_stamp.show_right > 0) {
            log_time_stamp_string = log_time_stamp_string.substr(log_time_stamp_string.length() - configuration->time_stamp.show_right);
        }
    }
    else {
        log_time_stamp_string = std::to_string(log_time_stamp_epoch.count());
    }
    auto field_separator = configuration->field_separator.field_separator;
    if(configuration->field_separator.trailing_space) {
        field_separator += " ";
    }
    auto colorize_head = [&configuration, &color_output, &field_separator, &log_time_stamp_string]() {
        if(configuration->level_string.length() > 0) {
            color_output << colorize(configuration, configuration->level_string);
            color_output << colorize(&configuration->field_separator, field_separator);
        }
        if(configuration->time_stamp.show) {
            color_output << colorize(&configuration->time_stamp, log_time_stamp_string);
            color_output << colorize(&configuration->field_separator, field_separator);
        }
    };
    if(listening) {
        log_message = v8::Object::New(isolate);
        auto result = log_message->DefineOwnProperty(
            context,
            slim::utilities::StringToName(isolate, "logLevel"),
            slim::utilities::StringToValue(isolate, configuration->level_string)
        );
        result = log_message->DefineOwnProperty(
            context,
            slim::utilities::StringToName(isolate, "fieldSeparator"),
            slim::utilities::StringToValue(isolate, field_separator)
        );
        result = log_message->DefineOwnProperty(
            context,
            slim::utilities::StringToName(isolate, "logTimeStamp"),
            slim::utilities::StringToValue(isolate, log_time_stamp_string)
        );
        if(output_when_listening) {
            colorize_head();
        }
    }
    else {
        colorize_head();
    }
    for(int index = 0; index < args.Length(); index++) {
        auto value = args[index];
        std::string out;

        if(value->IsFunction()) {
            auto name = value.As<v8::Function>()->GetName();
            std::string name_str = slim::utilities::v8ValueToString(isolate, name);
            out = "Function " + (name_str.empty() ? "(anonymous)" : name_str);
        }
        else if(value->IsArray()) {
            auto json_string_value = v8::JSON::Stringify(context, value);
            out = "Array(" + std::to_string(value.As<v8::Array>()->Length()) + ") " +
                slim::utilities::v8StringToString(isolate, json_string_value.ToLocalChecked());
        }
        else if(value->IsObject()) {
            auto json_string_value = v8::JSON::Stringify(context, value);
            out = slim::utilities::v8StringToString(isolate, json_string_value.ToLocalChecked());
        }
        else if(value->IsNull()) {
            out = "null";
        }
        else if(value->IsUndefined()) {
            out = "undefined";
        }
        else if(value->IsNumber()) {
            out = slim::utilities::v8ValueToString(isolate, value);
        }
        else if(value->IsBoolean()) {
            out = value->BooleanValue(isolate) ? "true" : "false";
        }
        else if(value->IsString()) {
            out = slim::utilities::StringValue(isolate, value);
        }
        else if(value->IsSymbol()) {
            auto desc = value.As<v8::Symbol>()->Description(isolate);
            out = "Symbol(" + slim::utilities::v8ValueToString(isolate, desc) + ")";
        }
        else if(value->IsBigInt()) {
            bool lossless;
            out = std::to_string(value.As<v8::BigInt>()->Int64Value(&lossless)) + "n";
        }
        else {
            out = slim::utilities::v8ValueToString(isolate, value);
        }

        if(listening) {
            output << out;
            if(output_when_listening) color_output << out;
        } else {
            color_output << out;
        }

        if(index != args.Length() - 1) {
            if(listening) {
                output << " ";
                if(output_when_listening) color_output << " ";
            } else {
                color_output << " ";
            }
        }
    }
    if(listening) {
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Local<v8::Promise::Resolver> resolver = v8::Promise::Resolver::New(context).ToLocalChecked();
        if(output_when_listening) {
            auto result = log_message->DefineOwnProperty(
                context,
                slim::utilities::StringToName(isolate, "logMessage"),
                slim::utilities::StringToValue(isolate, color_output.str())
            );
            std::cout << color_output.str() << "\n";
        }
        auto result = log_message->DefineOwnProperty(
            context,
            slim::utilities::StringToName(isolate, "rawLogMessage"),
            slim::utilities::StringToValue(isolate, output.str())
        );
        result = resolver->Resolve(context, log_message);
        //result = listen_array->Set(isolate->GetCurrentContext(), listen_array->Length(), resolver->GetPromise());
    }
    else {
        std::cout << color_output.str() << "\n";
    }
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    return;
}

void print_colors(const v8::FunctionCallbackInfo<v8::Value>& args) {
    Configuration temp_configuration;
    auto print_color = [&temp_configuration](auto property) {
        int index = 0;
        for(auto color: colors) {
            *property = color;
            std::cout << colorize(&temp_configuration, *property) << " ";
            index++;
            if(index == 5 || index == 10 || index == 13 || index == 16) {
                std::cout << "\n\t";
            }
        }
        *property = "default";
    };
    std::cout << ".text_color\n\t";
    print_color(&temp_configuration.text_color);
    std::cout << "\n\n";
    std::cout << ".background_color\n\t";
    print_color(&temp_configuration.background_color);
    std::cout << "\n";
}
} //namespace slim::plugin::console
