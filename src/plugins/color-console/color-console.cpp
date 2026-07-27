/* todo: add try catch throw as a measure to catch v8 errors see 'auto result = ' below */



#include <any>
#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <v8.h>
#include <slim/plugin.hpp>
#include <slim/utilities.h>
/*
 * Reference https://console.spec.whatwg.org/#printer
 */
namespace slim::console {
//    v8::Local<v8::Array> listen_array = v8::Array::New(isolate);
    bool listening = false;
    bool output_when_listening = false;
    /* remember to keep these in sync through out other definitions */
    auto common_log_levels = {"debug", "error", "info", "log", "print", "todo", "trace", "warn"};
    auto common_log_level_members = {"field_separator", "message_text", "message_value", "remainder", "time_stamp"};
    struct BaseConfiguration {
        bool dim = false;
        bool bold = false;
        bool italic = false;
        bool inverse = false;
        bool underline = false;
        bool show = true;
        bool expand_objects = false;
        std::string text_color = "default";
        std::string background_color = "default";
    };
    struct FieldSeparatorConfiguration: BaseConfiguration {
        std::string field_separator = ":";
        bool trailing_space = true;
    };
    struct TimeStampConfiguration: BaseConfiguration {
        std::string time_format = "epoch";
        bool show = false;
        int show_right = 0;
    };
    struct Configuration: BaseConfiguration {
        std::string level_string = "";
        std::string text_color = "default";
        bool expand_objects = false;
        FieldSeparatorConfiguration field_separator{};
        BaseConfiguration message_text{};
        BaseConfiguration message_value{};
        BaseConfiguration remainder{};
        TimeStampConfiguration time_stamp{};
        std::unordered_map<std::string, std::any> members {
            {"field_separator", &field_separator},
            {"message_text", &message_text},
            {"message_value", &message_value},
            {"remainder", &remainder},
            {"time_stamp", &time_stamp}
        };
    };
    std::vector<std::string> colors {
        "default", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white", "bright black",
        "bright red", "bright green", "bright yellow", "bright blue", "bright magenta", "bright cyan", "bright white"
    };
    std::unordered_map<std::string, int> text_colors {
        {"default", 39}, {"black", 30}, {"red", 31}, {"green", 32}, {"yellow", 33}, {"blue", 34},
        {"magenta", 35}, {"cyan", 36}, {"white", 37}, {"bright black", 90}, {"bright red", 91}, {"bright green", 92},
        {"bright yellow", 93}, {"bright blue", 94}, {"bright magenta", 95}, {"bright cyan", 96}, {"bright white", 97},
    };
    std::unordered_map<std::string, int> background_colors {
        {"default", 49}, {"black", 40}, {"red", 41}, {"green", 42}, {"yellow", 43}, {"blue", 44},
        {"magenta", 45}, {"cyan", 46}, {"white", 47}, {"bright black", 100}, {"bright red", 101}, {"bright green", 102},
        {"bright yellow", 103}, {"bright blue", 104}, {"bright magenta", 105}, {"bright cyan", 106}, {"bright white", 107}
    };
    Configuration dir_configuration{.expand_objects=true};
    Configuration dirxml_configuration{};
    Configuration debug_configuration{.level_string="DEBUG", .text_color="red"};
    Configuration error_configuration{.level_string="ERROR", .text_color="red"};
    Configuration info_configuration{.level_string="INFO", .text_color="bright white"};
    Configuration log_configuration{.level_string="LOG"};
    Configuration print_configuration{};
    Configuration todo_configuration{.level_string="TODO", .text_color="blue"};
    Configuration table_configuration{.level_string="TABLE", .text_color="bright red"};
    Configuration trace_configuration{.level_string="TRACE", .text_color="bright green"};
    Configuration warn_configuration{.level_string="WARN", .text_color="yellow"};
    std::unordered_map<std::string, slim::console::Configuration*> level_configurations {
        {"dir", &dir_configuration},
        {"dirxml", &dir_configuration},
        {"debug", &debug_configuration},
        {"error", &error_configuration},
        {"info", &info_configuration},
        {"log", &log_configuration},
        {"print", &print_configuration},
        {"todo", &todo_configuration},
        {"table", &table_configuration},
        {"trace", &trace_configuration},
        {"warn", &warn_configuration}
    };
    /********/
    void configure(const v8::FunctionCallbackInfo<v8::Value>& args);
    void configure(v8::Isolate* isolate, const v8::Local<v8::Object> object, BaseConfiguration* configuration);
    void copy(const v8::FunctionCallbackInfo<v8::Value>& args);
    void copy(const BaseConfiguration* source, BaseConfiguration* destination);
    void copy_configuration(const v8::FunctionCallbackInfo<v8::Value>& args);
    void copy_console_configuration(const BaseConfiguration* source, BaseConfiguration* destination);
    static std::string colorize(auto* configuration, const std::string string_value);
    slim::plugin::plugin CreateSublevelPlugin(v8::Isolate* isolate, const std::string level, auto configuration);
    void local_print(const v8::FunctionCallbackInfo<v8::Value>& args, Configuration* configuration);
    void print_colors(const v8::FunctionCallbackInfo<v8::Value>& args);
    /********/
    void assert_console(const v8::FunctionCallbackInfo<v8::Value>& args);
    void clear(const v8::FunctionCallbackInfo<v8::Value>& args);
    void count(const v8::FunctionCallbackInfo<v8::Value>& args);
    void countReset(const v8::FunctionCallbackInfo<v8::Value>& args);
    void dir(const v8::FunctionCallbackInfo<v8::Value>& args);
    void dirxml(const v8::FunctionCallbackInfo<v8::Value>& args);
    void debug(const v8::FunctionCallbackInfo<v8::Value>& args);
    void error(const v8::FunctionCallbackInfo<v8::Value>& args);
    void group(const v8::FunctionCallbackInfo<v8::Value>& args);
    void groupCollapsed(const v8::FunctionCallbackInfo<v8::Value>& args);
    void groupEnd(const v8::FunctionCallbackInfo<v8::Value>& args);
    void info(const v8::FunctionCallbackInfo<v8::Value>& args);
    void listen(const v8::FunctionCallbackInfo<v8::Value>& args);
    void log(const v8::FunctionCallbackInfo<v8::Value>& args);
    void print(const v8::FunctionCallbackInfo<v8::Value>& args);
    void print_colors(const v8::FunctionCallbackInfo<v8::Value>& args);
    void table(const v8::FunctionCallbackInfo<v8::Value>& args);
    void time(const v8::FunctionCallbackInfo<v8::Value>& args);
    void timeEnd(const v8::FunctionCallbackInfo<v8::Value>& args);
    void timeLog(const v8::FunctionCallbackInfo<v8::Value>& args);
    void todo(const v8::FunctionCallbackInfo<v8::Value>& args);
    void trace(const v8::FunctionCallbackInfo<v8::Value>& args);
    void warn(const v8::FunctionCallbackInfo<v8::Value>& args);
    void write(const v8::FunctionCallbackInfo<v8::Value>& args);
    /********/
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
        for(auto level: {"dir", "log" "debug", "error", "info", "todo", "trace", "warn"}) {
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
    void old_local_print(const v8::FunctionCallbackInfo<v8::Value>& args, Configuration* configuration) {
        if(args.Length() == 0) { return; }
        auto isolate = args.GetIsolate();
        v8::HandleScope scope(isolate);
        std::stringstream output;
        if(configuration->level_string.length() > 0) {
            output << colorize(configuration, configuration->level_string + ": ");
        }
        if(configuration->show) {
            v8::TryCatch try_catch(isolate);
            isolate->ThrowException(slim::utilities::StringToString(isolate, ""));
            if(try_catch.HasCaught()) {
                auto context = isolate->GetCurrentContext();
                auto message = try_catch.Message();
                auto file_name = slim::utilities::ScriptFileName(message);
                auto line_number = slim::utilities::ScriptLineNumber(message);
                file_name = file_name.substr(file_name.find_last_of('/')+1);
                //output << colorize(&configuration->location, file_name + ":" + std::to_string(line_number) + "\t");
            }
        }
        std::stringstream value_stream;
        int index = 0;
        if(args[index]->IsString()) {
            auto printf_string = slim::utilities::StringValue(isolate, args[index]);
            std::regex format("(%s)|(%d)|(%f)|(%i)|(%o)|(%O)");
            for(auto i = std::sregex_iterator(printf_string.begin(), printf_string.end(), format); i != std::sregex_iterator(); i++) {
                std::cout << "now in position " << i->position() << "\n";
                index++;
                if(i->str() == "%s") {
                    printf_string = std::regex_replace(printf_string, std::regex("(%s)"),
                        slim::utilities::StringValue(isolate, args[index]), std::regex_constants::format_first_only);
                }
                else if(i->str() == "%d" || i->str() == "%i") {
                    if(args[index]->IsInt32()) {
                        printf_string = std::regex_replace(printf_string, std::regex("(" + i->str() + ")"),
                            std::to_string(slim::utilities::IntValue(isolate, args[index])), std::regex_constants::format_first_only);
                    }
                    else {
                        printf_string = std::regex_replace(printf_string, std::regex("(" + i->str() + ")"),
                            "NaN", std::regex_constants::format_first_only);
                    }
                }
                else if(i->str() == "%f") {
                    if(args[index]->IsNumber()) {
                        auto number_string = std::to_string(slim::utilities::NumberValue(isolate, args[index]));
                        printf_string = std::regex_replace(printf_string, std::regex("(%f)"),
                            number_string, std::regex_constants::format_first_only);
                    }
                }
            }
            index++;
            value_stream << printf_string;
        }
        for(; index < args.Length(); index++) {
            auto value = args[index];
            if(value->IsNumber()) {
                value_stream << slim::utilities::NumberValue(isolate, value);
            }
            else if(value->IsBoolean()) {
                auto bool_value = (value->BooleanValue(isolate)) ? "true" : "false";
                value_stream << bool_value;
            }
            else if(value->IsString()) {
                value_stream << slim::utilities::StringValue(isolate, value);
            }
            else if(value->IsFunction()) {
                value_stream << "Function " << slim::utilities::StringFunction(isolate, value);
            }
            else if(value->IsArray()) {
                value_stream << "Array(" << slim::utilities::ArrayCount(value) << ") " << slim::utilities::V8JsonValueToString(isolate, value);
            }
            else if(value->IsObject()) {
                value_stream << "Object " << slim::utilities::V8JsonValueToString(isolate, value);
            }
            else {
                value_stream << "Typeof " << slim::utilities::StringValue(isolate, value->TypeOf(isolate));
            }
            if(index != args.Length() - 1) { value_stream << " "; }
        }
        if(configuration->level_string.length() > 0) {
            output << colorize(&configuration->remainder, value_stream.str());
        }
        else {
            output << colorize(configuration, value_stream.str());
        }
        std::cout << output.str() << "\n";
    }
}
std::string slim::console::colorize(auto* configuration, const std::string string_value) {
    std::stringstream return_stream;
    if(configuration->show) {
        if(configuration->bold) {
            return_stream << "\33[1m";
        }
        if(configuration->dim) {
            return_stream << "\33[2m";
        }
        if(configuration->italic) {
            return_stream << "\33[3m";
        }
        if(configuration->underline) {
            return_stream << "\33[4m";
        }
        if(configuration->inverse) {
            return_stream << "\33[7m";
        }
        int console_text_color = text_colors[configuration->text_color];
        int console_background_color = background_colors[configuration->background_color];
        if(console_text_color > 29) {
            return_stream << "\33[" + std::to_string(console_text_color) << "m";
        }
        else {
            if(stoi(configuration->text_color) > -1) {
                return_stream << "\33[38;5;" + configuration->text_color << "m";
            }
            else if(std::regex_match(configuration->text_color, std::regex("[0-9]{1,3};[0-9]{1,3};[0-9]{1,3}"))) {
                return_stream << "\33[38;2;" + configuration->text_color << "m";
            }
        }
        if(console_background_color > 38) {
            return_stream << "\33[" + std::to_string(console_background_color) << "m";
        }
        else {
            if(stoi(configuration->background_color) > -1) {
                return_stream << "\33[48;5;" + configuration->background_color << "m";
            }
            else if(std::regex_match(configuration->text_color, std::regex("[0-9]{1,3};[0-9]{1,3};[0-9]{1,3}"))) {
                return_stream << "\33[48;2;" + configuration->background_color << "m";
            }
        }
        return_stream << string_value << "\33[0m";
    }
    return return_stream.str();
}
void slim::console::local_print(const v8::FunctionCallbackInfo<v8::Value>& args, Configuration* configuration) {
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
        if(value->IsObject()) {
            auto json_string_value = v8::JSON::Stringify(isolate->GetCurrentContext(), value);
            std::string string_value = slim::utilities::v8StringToString(isolate, json_string_value.ToLocalChecked());
            if(listening) {
                output << string_value;
                if(output_when_listening) {
                    color_output << string_value;
                }
            }
            else {
                color_output << string_value;
            }
        }
        else {
            if(listening) {
                output << slim::utilities::v8ValueToString(isolate, value);
                if(output_when_listening) {
                    color_output << slim::utilities::v8ValueToString(isolate, value);
                }
            }
            else {
                color_output << slim::utilities::v8ValueToString(isolate, value);
            }
        }
        if(index != args.Length() - 1) {
            if(listening) {
                output << " ";
                if(output_when_listening) {
                    color_output << " ";
                }
            }
            else {
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
    return;
}
/********/
void slim::console::assert_console(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("assert not implemented");
}
void slim::console::clear(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::cout << "\x1B[2J\x1B[H";
}
void slim::console::count(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("count not implemented");
}
void slim::console::countReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("countReset not implemented");
}
void slim::console::debug(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["debug"]);
}
void slim::console::dir(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["dir"]);
}
void slim::console::dirxml(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["dirxml"]);
}
void slim::console::error(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["error"]);
}
void slim::console::group(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("group not implemented");
}
void slim::console::groupCollapsed(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("groupCollapsed not implemented");
}
void slim::console::groupEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("groupEnd not implemented");
}
void slim::console::info(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["info"]);
}
void slim::console::listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    //args.GetReturnValue().Set(listen_array);
}
void slim::console::log(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["log"]);
}
void slim::console::print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["print"]);
}
void slim::console::print_colors(const v8::FunctionCallbackInfo<v8::Value>& args) {
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
void slim::console::table(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["table"]);
}
void slim::console::time(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("time not implemented");
}
void slim::console::timeEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("timeEnd not implemented");
}
void slim::console::timeLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
    puts("timeLog not implemented");
}
void slim::console::todo(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["todo"]);
}
void slim::console::trace(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["trace"]);
}
void slim::console::warn(const v8::FunctionCallbackInfo<v8::Value>& args) {
    local_print(args, level_configurations["warn"]);
}
void slim::console::write(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    std::stringstream output;
    int index = 0;
    for(; index < args.Length(); index++) {
        auto value = args[index];
        if(value->IsObject()) {
            auto json_string_value = v8::JSON::Stringify(isolate->GetCurrentContext(), value);
            std::string string_value = slim::utilities::v8StringToString(isolate, json_string_value.ToLocalChecked());
            output << string_value;
        }
        else {
            output << slim::utilities::v8ValueToString(isolate, value);
        }
        if(index != args.Length() - 1) { output << " "; }
    }
    std::cout << output.str() << "\n";
}
extern "C" void expose_plugin(v8::Isolate* isolate) {
	slim::plugin::plugin console_plugin(isolate, "console");
    console_plugin.add_function("assert",         &slim::console::assert_console);
    console_plugin.add_function("clear",          &slim::console::clear);
    console_plugin.add_function("count",          &slim::console::count);
    console_plugin.add_function("countReset",     &slim::console::countReset);
    console_plugin.add_function("debug",          &slim::console::debug);
	console_plugin.add_function("dir",            &slim::console::dir);
	console_plugin.add_function("dirxml",         &slim::console::dirxml);
	console_plugin.add_function("error",          &slim::console::error);
    console_plugin.add_function("group",          &slim::console::group);
    console_plugin.add_function("groupCollapsed", &slim::console::groupCollapsed);
    console_plugin.add_function("groupEnd",       &slim::console::groupEnd);
	console_plugin.add_function("info",           &slim::console::info);
	console_plugin.add_function("log",            &slim::console::log);
    console_plugin.add_function("listen",         &slim::console::listen);
	console_plugin.add_function("print",          &slim::console::print);
    console_plugin.add_function("print_colors",   &slim::console::print_colors);
	console_plugin.add_function("table",          &slim::console::table);
    console_plugin.add_function("time",           &slim::console::time);
	console_plugin.add_function("timeEnd",        &slim::console::timeEnd);
    console_plugin.add_function("timeLog",        &slim::console::timeLog);
    console_plugin.add_function("todo",           &slim::console::todo);
	console_plugin.add_function("trace",          &slim::console::trace);
	console_plugin.add_function("warn",           &slim::console::warn);
    console_plugin.add_function("write",          &slim::console::write);
    auto createSublevelPlugin = [&isolate](const std::string level, auto* configuration) -> slim::plugin::plugin {
        slim::plugin::plugin sublevel_plugin(isolate, level);
        sublevel_plugin.add_property("background_color", &configuration->background_color);
        sublevel_plugin.add_property("bold",             &configuration->bold);
        sublevel_plugin.add_property("dim",              &configuration->dim);
        sublevel_plugin.add_property("expand_objects",   &configuration->expand_objects);
        sublevel_plugin.add_property("inverse",          &configuration->inverse);
        sublevel_plugin.add_property("italic",           &configuration->italic);      
        sublevel_plugin.add_property("show",             &configuration->show);
        sublevel_plugin.add_property("text_color",       &configuration->text_color);
        sublevel_plugin.add_property("underline",        &configuration->underline);
        return sublevel_plugin;
    };
    slim::plugin::plugin configuration_plugin(isolate, "configuration");
    configuration_plugin.add_property("listen",                &slim::console::listening);
    configuration_plugin.add_property("output_when_listening", &slim::console::output_when_listening);
    configuration_plugin.add_function("copy",                  &slim::console::copy_configuration);
    for(auto level: slim::console::common_log_levels) {
        auto level_configuration = slim::console::level_configurations[level];
        auto level_plugin = createSublevelPlugin(level, level_configuration);
        for(auto member: slim::console::common_log_level_members) {
            try  {
                if(member == "time_stamp") {
                    auto member_configuration = any_cast<slim::console::TimeStampConfiguration*>(level_configuration->members[member]);
                    auto member_plugin = createSublevelPlugin(member, member_configuration);
                    member_plugin.add_property("show_right", &member_configuration->show_right);
                    member_plugin.add_property("time_format", &member_configuration->time_format);
                    level_plugin.add_plugin(member, &member_plugin);
                }
                else if(member == "field_separator") {
                    auto member_configuration = any_cast<slim::console::FieldSeparatorConfiguration*>(level_configuration->members[member]);
                    auto member_plugin = createSublevelPlugin(member, member_configuration);
                    member_plugin.add_property("trailing_space", &member_configuration->trailing_space);
                    level_plugin.add_plugin(member, &member_plugin);
                }
                else {
                    auto member_configuration = any_cast<slim::console::BaseConfiguration*>(level_configuration->members[member]);
                    auto member_plugin = createSublevelPlugin(member, member_configuration);
                    level_plugin.add_plugin(member, &member_plugin);
                }
            }
            catch (const std::bad_any_cast& e) {
                std::cout << "console.cpp: " << e.what() << '\n';
            }
        }
        configuration_plugin.add_plugin(level, &level_plugin);
    }
    console_plugin.add_plugin("configuration", &configuration_plugin);
	console_plugin.expose_plugin();
	return;
}
