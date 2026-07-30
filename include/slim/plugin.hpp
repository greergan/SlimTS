#pragma once
#include <functional>
#include <memory>
#include <variant>
#include <vector>
#include <v8.h>
#include <slim/utilities.h>

#define BOOL 1
#define CHAR 2
#define DOUBLE 3
#define FLOAT 4
#define INTEGER 5
#define NUMBER 6
#define STRING 7
#include <iostream>

namespace slim::plugin {
    class PropertyPointer {
        std::variant<bool*, std::string*, int*> property;
        public:
            PropertyPointer(auto&& property): property{property} {}
            ~PropertyPointer() {}
            v8::Local<v8::Value> GetV8Value(v8::Isolate* isolate) {
                v8::Local<v8::Value> value;
                if(std::holds_alternative<std::string*>(property)) {
                    value = slim::utilities::StringToString(isolate, *std::get<std::string*>(property)).As<v8::Value>();
                }
                else if(std::holds_alternative<bool*>(property)) {
                    value = v8::Boolean::New(isolate, *std::get<bool*>(property)).As<v8::Value>();
                }
                else if(std::holds_alternative<int*>(property)) {
                    value = v8::Int32::New(isolate, *std::get<int*>(property)).As<v8::Value>();
                }
                return value;
            }
            void SetValue(v8::Isolate* isolate, v8::Local<v8::Value> value) {
                if(std::holds_alternative<std::string*>(property)) {
                    std::get<std::string*>(property)->assign(slim::utilities::StringValue(isolate, value));
                }
                else if(std::holds_alternative<bool*>(property)) {
                    *std::get<bool*>(property) = slim::utilities::BoolValue(isolate, value);
                }
                else if(std::holds_alternative<int*>(property)) {
                    *std::get<int*>(property) = slim::utilities::IntValue(isolate, value);
                }
            }
    };
    struct plugin {
        plugin(v8::Isolate* isolate, std::string name): isolate{isolate} {
            plugin_template = v8::ObjectTemplate::New(isolate);
            v8_name = slim::utilities::StringToName(isolate, name);
        }
/*         template<typename Function, typename Signature = typename std::decay<Function>::type>
        plugin(v8::Isolate* isolate, std::string name, Function&& function): isolate{isolate} {
            plugin_template = v8::ObjectTemplate::New(isolate);
            v8_name = slim::utilities::StringToName(isolate, name);
            //v8::FunctionTemplate constructor_template = v8::FunctionTemplate::New(isolate, std::forward<Signature>(function));
            //isolate->GetCurrentContext()->Global()->Set(isolate->GetCurrentContext(), v8_name, constructor_template).ToChecked();

            //slim::gv8::GetGlobalObjectTemplate()->Set(isolate, v8_name, v8::FunctionTemplate::New(isolate, function));
            //isolate->GetCurrentContext()->Global()->Set(isolate->GetCurrentContext(), v8_name, v8::FunctionTemplate::New(isolate, std::forward<Signature>(function))).ToChecked();
        } */
        template<typename Function, typename Signature = typename std::decay<Function>::type>
        void add_function(std::string name, Function&& function) {
            v8::HandleScope scope(isolate);
            plugin_template->Set(slim::utilities::StringToName(isolate, name), v8::FunctionTemplate::New(isolate, std::forward<Signature>(function)));
        }
        void add_plugin(std::string name, plugin* child) {
            v8::HandleScope scope(isolate);
            plugin_template->Set(slim::utilities::StringToName(isolate, name), child->plugin_template);
        }
        void add_property(std::string name, auto&& property) {
            v8::HandleScope scope(isolate);
            auto getter = [](v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& args){
                v8::Handle<v8::External> data = v8::Handle<v8::External>::Cast(args.Data());
                args.GetReturnValue().Set(static_cast<slim::plugin::PropertyPointer*>(data->Value())->GetV8Value(args.GetIsolate()));
            };
            auto setter = [](v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& args) {
                    v8::Handle<v8::External> data = v8::Handle<v8::External>::Cast(args.Data());
                    static_cast<slim::plugin::PropertyPointer*>(data->Value())->SetValue(args.GetIsolate(), value);
            };
            auto& property_pointer = property_pointers_.emplace_back(std::make_unique<PropertyPointer>(property));
            plugin_template->SetNativeDataProperty(slim::utilities::StringToName(isolate, name), getter, setter, v8::External::New(isolate, (void*)property_pointer.get()));
        }
        template<typename T, typename U>
        void add_property_immutable(T property_name, U property_value) {
            std::string property_name_string;
            using property_name_data_type = std::remove_cvref_t<std::remove_pointer_t<T>>;
            if constexpr (std::is_same_v<char, property_name_data_type>) {
                property_name_string = std::string(property_name);
            }
            else if constexpr (std::is_same_v<std::string, property_name_data_type>) {
                property_name_string = property_name;
            }
            else {
                throw std::invalid_argument("slim::plugin::plugin::add_property_immutable requires a string or char type for the property name");
            }
            using property_value_data_type = std::remove_cvref_t<std::remove_pointer_t<U>>;
            if constexpr (std::is_same_v<bool, property_value_data_type>) {
                plugin_template->Set(slim::utilities::StringToV8String(isolate, property_name_string), v8::Boolean::New(isolate, property_value), v8::ReadOnly);
            }
            else if constexpr (std::is_same_v<char, property_value_data_type> || std::is_same_v<std::string, property_value_data_type>) {
                plugin_template->Set(slim::utilities::StringToV8String(isolate, property_name_string), slim::utilities::StringToV8String(isolate, property_value), v8::ReadOnly);
            }
            else if constexpr (std::is_same_v<int, property_value_data_type>) {
                plugin_template->Set(slim::utilities::StringToV8String(isolate, property_name_string), v8::Integer::New(isolate, property_value), v8::ReadOnly);
            }
            else if constexpr (std::is_same_v<double, property_value_data_type> || std::is_same_v<float, property_value_data_type>
                                                                                    || std::is_same_v<unsigned long, property_value_data_type>) {
                plugin_template->Set(slim::utilities::StringToV8String(isolate, property_name_string), v8::Number::New(isolate, property_value), v8::ReadOnly);
            }
            else {
                throw std::invalid_argument("slim::plugin::plugin::add_property_immutable requires a primitive type for the property value");
            }
        }
        void expose_plugin(v8::Local<v8::Object> new_v8_object) {
            isolate->GetCurrentContext()->Global()->Set(isolate->GetCurrentContext(), v8_name, new_v8_object).ToChecked();
        }
        void expose_plugin() {
            expose_plugin(new_instance());
        }
        v8::Local<v8::ObjectTemplate>& get_plugin() {
            return plugin_template;
        }
        v8::Local<v8::Object> new_instance() {
            return plugin_template->NewInstance(isolate->GetCurrentContext()).ToLocalChecked();
        }
        private:
            v8::Isolate* isolate;
            v8::Local<v8::Name> v8_name;
            v8::Local<v8::ObjectTemplate> plugin_template;
            std::vector<std::unique_ptr<PropertyPointer>> property_pointers_;
    };
}
