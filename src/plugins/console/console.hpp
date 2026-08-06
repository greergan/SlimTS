#pragma once

#include <any>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <v8.h>

namespace slim::plugin::console {

    extern bool listening;
    extern bool output_when_listening;

    extern std::initializer_list<const char*> common_log_levels;
    extern std::initializer_list<const char*> common_log_level_members;

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
            {"message_text",    &message_text},
            {"message_value",   &message_value},
            {"remainder",       &remainder},
            {"time_stamp",      &time_stamp}
        };
    };

    extern std::vector<std::string> colors;
    extern std::unordered_map<std::string, int> text_colors;
    extern std::unordered_map<std::string, int> background_colors;

    extern Configuration dir_configuration;
    extern Configuration dirxml_configuration;
    extern Configuration debug_configuration;
    extern Configuration error_configuration;
    extern Configuration info_configuration;
    extern Configuration log_configuration;
    extern Configuration print_configuration;
    extern Configuration todo_configuration;
    extern Configuration table_configuration;
    extern Configuration trace_configuration;
    extern Configuration warn_configuration;

    extern std::unordered_map<std::string, Configuration*> level_configurations;

    // colorize
    std::string colorize(auto* configuration, const std::string string_value) {
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

    // configure
    void configure(const v8::FunctionCallbackInfo<v8::Value>& args);
    void configure_console(v8::Isolate* isolate, const v8::Local<v8::Object> object, BaseConfiguration* configuration);
    void copy_configuration(const v8::FunctionCallbackInfo<v8::Value>& args);
    void copy_console_configuration(const BaseConfiguration* source, BaseConfiguration* destination);

    // print
    void local_print(const v8::FunctionCallbackInfo<v8::Value>& args, Configuration* configuration);
    void old_local_print(const v8::FunctionCallbackInfo<v8::Value>& args, Configuration* configuration);
    void print_colors(const v8::FunctionCallbackInfo<v8::Value>& args);

    // plugin functions
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
    void table(const v8::FunctionCallbackInfo<v8::Value>& args);
    void time(const v8::FunctionCallbackInfo<v8::Value>& args);
    void timeEnd(const v8::FunctionCallbackInfo<v8::Value>& args);
    void timeLog(const v8::FunctionCallbackInfo<v8::Value>& args);
    void todo(const v8::FunctionCallbackInfo<v8::Value>& args);
    void trace(const v8::FunctionCallbackInfo<v8::Value>& args);
    void warn(const v8::FunctionCallbackInfo<v8::Value>& args);
    void write(const v8::FunctionCallbackInfo<v8::Value>& args);

} // namespace slim::console
