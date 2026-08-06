#include <any>
#include <iostream>
#include <v8.h>
#include <slim/plugin.hpp>
#include <slim/utilities.h>
#include "console.hpp"

#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif

namespace slim::common {};
namespace slim::plugin::console {
using namespace::slim::common;
bool listening = false;
bool output_when_listening = false;

std::initializer_list<const char*> common_log_levels = {"debug", "error", "info", "log", "print", "todo", "trace", "warn"};
std::initializer_list<const char*> common_log_level_members = {"field_separator", "message_text", "message_value", "remainder", "time_stamp"};

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

std::unordered_map<std::string, Configuration*> level_configurations {
    {"dir",    &dir_configuration},
    {"dirxml", &dir_configuration},
    {"debug",  &debug_configuration},
    {"error",  &error_configuration},
    {"info",   &info_configuration},
    {"log",    &log_configuration},
    {"print",  &print_configuration},
    {"todo",   &todo_configuration},
    {"table",  &table_configuration},
    {"trace",  &trace_configuration},
    {"warn",   &warn_configuration}
};

/********/
void assert_console(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("assert not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void clear(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    std::cout << "\x1B[2J\x1B[H";
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void count(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("count not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void countReset(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("countReset not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void debug(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["debug"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void dir(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["dir"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void dirxml(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["dirxml"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void error(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["error"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void group(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("group not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void groupCollapsed(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("groupCollapsed not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void groupEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("groupEnd not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void info(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["info"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    //args.GetReturnValue().Set(listen_array);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void log(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["log"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void print(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["print"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void table(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["table"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void time(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("time not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void timeEnd(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("timeEnd not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void timeLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    puts("timeLog not implemented");
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void todo(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["todo"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void trace(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["trace"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void warn(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    local_print(args, level_configurations["warn"]);
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}
void write(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
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
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
}


extern "C" void expose_plugin(v8::Isolate* isolate) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    slim::plugin::plugin console_plugin(isolate, "console");
    console_plugin.add_function("assert",         assert_console);
    console_plugin.add_function("clear",          clear);
    console_plugin.add_function("count",          count);
    console_plugin.add_function("countReset",     countReset);
    console_plugin.add_function("debug",          debug);
    console_plugin.add_function("dir",            dir);
    console_plugin.add_function("dirxml",         dirxml);
    console_plugin.add_function("error",          error);
    console_plugin.add_function("group",          group);
    console_plugin.add_function("groupCollapsed", groupCollapsed);
    console_plugin.add_function("groupEnd",       groupEnd);
    console_plugin.add_function("info",           info);
    console_plugin.add_function("log",            log);
    console_plugin.add_function("listen",         listen);
    console_plugin.add_function("print",          print);
    console_plugin.add_function("print_colors",   print_colors);
    console_plugin.add_function("table",          table);
    console_plugin.add_function("time",           time);
    console_plugin.add_function("timeEnd",        timeEnd);
    console_plugin.add_function("timeLog",        timeLog);
    console_plugin.add_function("todo",           todo);
    console_plugin.add_function("trace",          trace);
    console_plugin.add_function("warn",           warn);
    console_plugin.add_function("write",          write);
    auto createSublevelPlugin = [&isolate](const std::string level, auto* configuration) -> slim::plugin::plugin {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
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
#ifdef ENABLE_LOGGING
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
        return sublevel_plugin;
    };
    slim::plugin::plugin configuration_plugin(isolate, "configuration");
    configuration_plugin.add_property("listen",                &listening);
    configuration_plugin.add_property("output_when_listening", &output_when_listening);
    configuration_plugin.add_function("copy",                  &copy_configuration);
    for(auto level: common_log_levels) {
        auto level_configuration = level_configurations[level];
        auto level_plugin = createSublevelPlugin(level, level_configuration);
        for(auto member: common_log_level_members) {
            try  {
                if(member == "time_stamp") {
                    auto member_configuration = std::any_cast<TimeStampConfiguration*>(level_configuration->members[member]);
                    auto member_plugin = createSublevelPlugin(member, member_configuration);
                    member_plugin.add_property("show_right",  &member_configuration->show_right);
                    member_plugin.add_property("time_format", &member_configuration->time_format);
                    level_plugin.add_plugin(member, &member_plugin);
                }
                else if(member == "field_separator") {
                    auto member_configuration = std::any_cast<FieldSeparatorConfiguration*>(level_configuration->members[member]);
                    auto member_plugin = createSublevelPlugin(member, member_configuration);
                    member_plugin.add_property("trailing_space", &member_configuration->trailing_space);
                    level_plugin.add_plugin(member, &member_plugin);
                }
                else {
                    auto member_configuration = std::any_cast<BaseConfiguration*>(level_configuration->members[member]);
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
#ifdef ENABLE_LOGGING
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    return;
}
} // namespace slim::plugin::console
