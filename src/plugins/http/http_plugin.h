#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <stop_token>
#include <string>
#include <v8.h>
#include <slim/common/http/request.h>
#include <slim/common/network/client/http.h>
#include <slim/common/network/server/tcp.h>
#include <slim/common/io/task.h>
#include <slim/common/io/scheduler.h>

namespace slim::plugin::http {
    using namespace slim::common;

    struct ListenerState {
        v8::Isolate* isolate;
        v8::Global<v8::Promise::Resolver> pending_resolver;
        std::deque<v8::Global<v8::Object>> pending_requests;
        std::unique_ptr<slim::common::network::server::Tcp> tcp_server;
        explicit ListenerState(v8::Isolate* iso) : isolate(iso) {}
    };

    struct ConnectionState {
        slim::common::network::client::tcp::Connection connection;
        std::shared_ptr<ListenerState> listener_state;
        std::string connection_header; // value of Connection: header from request
        int fd;                        // raw socket fd for non-coroutine writes
        explicit ConnectionState(slim::common::network::client::tcp::Connection conn,
                                 std::shared_ptr<ListenerState> ls, std::string conn_hdr, int fd_)
            : connection(std::move(conn)), listener_state(std::move(ls)),
              connection_header(std::move(conn_hdr)), fd(fd_) {}
    };

    v8::Local<v8::Object> make_headers_object(v8::Isolate* isolate, const slim::common::http::Headers& headers);
    v8::Local<v8::Object> make_request_object(v8::Isolate* isolate,  slim::common::http::Request& request);
    v8::Local<v8::Object> make_response_object(v8::Isolate* isolate, std::shared_ptr<ConnectionState> conn_state);

    slim::common::io::Task<void> connection_handler(slim::common::io::Scheduler& scheduler, int fd, SSL_CTX* ssl_ctx,
                                                     std::shared_ptr<ListenerState> state);

    void listener_next(const v8::FunctionCallbackInfo<v8::Value>& args);
    void listener_async_iterator(const v8::FunctionCallbackInfo<v8::Value>& args);
    void serve(const v8::FunctionCallbackInfo<v8::Value>& args);

} // namespace slim::plugin::http
