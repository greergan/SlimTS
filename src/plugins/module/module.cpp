#include <format>
#include <string>
#include <v8.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/plugin.hpp>
#include <slim/utilities.h>

namespace slim::common {}
namespace slim::plugin::module_plugin {
    using namespace slim;
    using namespace slim::common;

    static constexpr const char* builtin_modules[] = {
        "assert", "assert/strict", "async_hooks", "buffer", "child_process", "cluster",
        "console", "constants", "crypto", "dgram", "diagnostics_channel", "dns", "dns/promises",
        "domain", "events", "fs", "fs/promises", "http", "http2", "https", "inspector",
        "inspector/promises", "module", "net", "os", "path", "path/posix", "path/win32",
        "perf_hooks", "process", "punycode", "querystring", "readline", "readline/promises",
        "repl", "stream", "stream/consumers", "stream/promises", "stream/web", "string_decoder",
        "sys", "timers", "timers/promises", "tls", "trace_events", "tty", "url", "util",
        "util/types", "v8", "vm", "wasi", "worker_threads", "zlib"
    };
}

extern "C" void expose_plugin(v8::Isolate* isolate) {
    using namespace slim::common;
#ifdef ENABLE_LOGGING
    log::trace({"expose_plugin", "begins", __FILE__, __LINE__});
#endif
    auto context = isolate->GetCurrentContext();
    auto module_obj = v8::Object::New(isolate);
    auto builtin_modules_array = v8::Array::New(isolate);
    for(int i = 0; i < sizeof(slim::plugin::module_plugin::builtin_modules) / sizeof(char*); i++) {
        builtin_modules_array->Set(context, i,
            utilities::StringToV8String(isolate, plugin::module_plugin::builtin_modules[i])).Check();
    }
    module_obj->Set(context, slim::utilities::StringToV8String(isolate, "builtinModules"), builtin_modules_array).Check();
    context->Global()->Set(context, slim::utilities::StringToV8String(isolate, "module"), module_obj).Check();
#ifdef ENABLE_LOGGING
    log::trace({"expose_plugin", "ends", __FILE__, __LINE__});
#endif
}
