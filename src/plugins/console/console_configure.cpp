#include <v8.h>
#include <slim/utilities.h>
#include "console.hpp"

namespace slim::plugin::console {
void configure_console(v8::Isolate* isolate, const v8::Local<v8::Object> object, BaseConfiguration* configuration) {
    if(object->IsObject() && slim::utilities::PropertyCount(isolate, object) > 0) {
        configuration->background_color = slim::utilities::SlimColorValue(isolate, slim::utilities::GetValue(isolate, "background_color", object), colors);
        configuration->bold = slim::utilities::BoolValue(isolate, slim::utilities::GetValue(isolate, "bold", object));
        configuration->dim = slim::utilities::BoolValue(isolate, slim::utilities::GetValue(isolate, "dim", object));
        configuration->expand_objects = slim::utilities::BoolValue(isolate, slim::utilities::GetValue(isolate, "expand_objects", object));
        configuration->italic = slim::utilities::BoolValue(isolate, slim::utilities::GetValue(isolate, "italic", object));
        configuration->inverse = slim::utilities::BoolValue(isolate, slim::utilities::GetValue(isolate, "inverse", object));
        configuration->show = slim::utilities::BoolValue(isolate, slim::utilities::GetValue(isolate, "show", object));
        configuration->text_color = slim::utilities::SlimColorValue(isolate, slim::utilities::GetValue(isolate, "text_color", object), colors);
        configuration->underline = slim::utilities::BoolValue(isolate, slim::utilities::GetValue(isolate, "underline", object));
    }
}

void configure(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if(!args[0]->IsObject()) { return; }
    auto isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    auto local_configurations = slim::utilities::GetObject(isolate, args[0]);
    for(auto level: {"dir", "log", "debug", "error", "info", "todo", "trace", "warn"}) {
        auto configuration = slim::utilities::GetObject(isolate, level, local_configurations);
        if(slim::utilities::PropertyCount(isolate, configuration) > 0) {
            auto level_configuration = level_configurations[level];
            configure_console(isolate, configuration, level_configuration);
            auto propagate = slim::utilities::GetValue(isolate, "propagate", configuration);
            if(propagate->IsBoolean() && propagate->BooleanValue(isolate)) {
                for(auto subsection_name: {"location", "remainder", "message_text", "message_value"}) {
                    auto section = level_configuration->members[subsection_name];
//copy_console_configuration(level_configuration, section);
                }
            }
            for(auto subsection_name: {"location", "remainder"}) {
                auto subsection_configuration = slim::utilities::GetObject(isolate, subsection_name, configuration);
                if(slim::utilities::PropertyCount(isolate, subsection_configuration) > 0) {
                    auto section = level_configuration->members[subsection_name];
                    auto inherit = slim::utilities::GetValue(isolate, "inherit", subsection_configuration);
                    if(inherit->IsBoolean() && inherit->BooleanValue(isolate)) {
//copy_console_configuration(level_configuration, section);
                    }
//configure_console(isolate, subsection_configuration, section);
                }
            }
            auto sub_configuration = slim::utilities::GetObject(isolate, "message", configuration);
            if(slim::utilities::PropertyCount(isolate, sub_configuration) > 0) {
                auto inherit = slim::utilities::GetValue(isolate, "inherit", sub_configuration);
                if(inherit->IsBoolean() && inherit->BooleanValue(isolate)) {
                    copy_console_configuration(level_configuration, &level_configuration->message_text);
                    copy_console_configuration(level_configuration, &level_configuration->message_value);
                }
                else {
                    auto message_configuration = slim::utilities::GetObject(isolate, "text", configuration);
                    if(slim::utilities::PropertyCount(isolate, message_configuration) > 0) {
                        auto section = level_configuration->message_text;
                        inherit = slim::utilities::GetValue(isolate, "inherit", sub_configuration);
                        if(inherit->IsBoolean() && inherit->BooleanValue(isolate)) {
                            copy_console_configuration(level_configuration, &section);
                        }
                        configure_console(isolate, message_configuration, &section);
                    }
                    message_configuration = slim::utilities::GetObject(isolate, "value", configuration);
                    if(slim::utilities::PropertyCount(isolate, message_configuration) > 0) {
                        auto section = level_configuration->message_value;
                        inherit = slim::utilities::GetValue(isolate, "inherit", sub_configuration);
                        if(inherit->IsBoolean() && inherit->BooleanValue(isolate)) {
                            copy_console_configuration(level_configuration, &section);
                        }
                        configure_console(isolate, message_configuration, &section);
                    }
                }
            }
        }
    }
}

void copy_console_configuration(const BaseConfiguration* source, BaseConfiguration* destination) {
    destination->bold = source->bold;
    destination->background_color = source->background_color;
    destination->dim = source->dim;
    destination->expand_objects = source->expand_objects;
    destination->italic = source->italic;
    destination->inverse = source->inverse;
    destination->show = source->show;
    destination->text_color = source->text_color;
    destination->underline = source->underline;
}

void copy_configuration(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    auto copy_expects = "copy expects exactly 2 string arguments";
    if(args.Length() != 2) {
        isolate->ThrowException(slim::utilities::StringToString(isolate, copy_expects));
    }
    if(!args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(slim::utilities::StringToString(isolate, copy_expects));
    }
    auto from = slim::utilities::StringValue(isolate, args[0]);
    auto to = slim::utilities::StringValue(isolate, args[1]);
    if(from == to) {
        isolate->ThrowException(slim::utilities::StringToString(isolate, "cannot copy to self"));
    }
    for(const std::string configuration: {to, from}) {
        if(!level_configurations.contains(configuration)) {
            isolate->ThrowException(slim::utilities::StringToString(isolate, "level not found: " + configuration));
        }
    }
    auto from_configuration = level_configurations[from];
    auto to_configuration = level_configurations[to];
    copy_console_configuration(from_configuration, to_configuration);
    for(auto sub_level: {"location", "remainder", "message_text", "message_value"}) {
        //copy_console_configuration(from_configuration->members[sub_level], to_configuration->members[sub_level]);
    }
}
} //namespace slim::plugin::console
