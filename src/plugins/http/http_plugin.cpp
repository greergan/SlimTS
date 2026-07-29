#include "http_plugin.h"
#include <slim/plugin.hpp>

namespace slim::plugin::http {

    extern "C" void expose_plugin(v8::Isolate* isolate) {
        v8::HandleScope handle_scope(isolate);
        slim::plugin::plugin http_plugin(isolate, "http");
        http_plugin.add_function("serve", slim::plugin::http::serve);
        http_plugin.expose_plugin();
    }

} // namespace slim::plugin::http
