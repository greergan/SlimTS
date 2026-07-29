#include "http_plugin.h"

#include <algorithm>
#include <slim/common/log.h>
#include <slim/common/http/error_codes.h>
#include <slim/common/network/client/http.h>
#include <slim/isolate_wake.h>
#include <slim/utilities.h>

namespace slim::plugin::http {
    using namespace slim::common;

    slim::common::io::Task<void> connection_handler(slim::common::io::Scheduler& scheduler, int fd,
                                                    SSL_CTX* ssl_ctx, std::shared_ptr<ListenerState> state) {
        log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
        log::debug(log::Message(__func__, "fd => " + std::to_string(fd), __FILE__, __LINE__));

        auto connection = co_await slim::common::network::client::http::Connection::create(scheduler, fd, nullptr);
        log::debug(log::Message(__func__, "connection created", __FILE__, __LINE__));

        std::vector<uint8_t> buf;
        log::debug(log::Message(__func__, "calling read", __FILE__, __LINE__));
        co_await connection.read(buf);
        log::debug(log::Message(__func__, "read complete, bytes => " + std::to_string(buf.size()), __FILE__, __LINE__));

        slim::common::http::Request request;
        request.set_body(std::move(buf));
        auto parse_status = request.parse();
        if (parse_status != slim::common::http::ErrorStatus::OK) {
            log::error(log::Message(__func__, std::string("request parse error => ") + std::string(slim::common::http::error::status::to_string(parse_status)), __FILE__, __LINE__));
            co_return;
        }
        std::string path = std::string(request.url().pathname());
        log::debug(log::Message(__func__, "parsed path => " + path, __FILE__, __LINE__));

        // extract Connection header value
        std::string conn_hdr;
        auto conn_hdr_ptr = request.headers().get("connection");
        if (conn_hdr_ptr) {
            auto& vals = const_cast<slim::common::http::Header&>(*conn_hdr_ptr).get_value();
            if (!vals.empty()) conn_hdr = vals[0];
        }
        std::transform(conn_hdr.begin(), conn_hdr.end(), conn_hdr.begin(), ::tolower);
        log::debug(log::Message(__func__, "connection header => " + conn_hdr, __FILE__, __LINE__));

        auto conn_state = std::make_shared<ConnectionState>(std::move(connection), state, std::move(conn_hdr), fd);

        log::debug(log::Message(__func__, "state.use_count() before post => " + std::to_string(state.use_count()), __FILE__, __LINE__));
        log::debug(log::Message(__func__, "posting to isolate", __FILE__, __LINE__));

        slim::isolate_wake::post(state->isolate, [state, conn_state, request = std::move(request)]() mutable {
            log::trace(log::Message(__func__, "resolve task begins", __FILE__, __LINE__));
            auto* isolate = state->isolate;
            auto context = isolate->GetCurrentContext();

            auto request_obj  = make_request_object(isolate, request);
            auto response_obj = make_response_object(isolate, conn_state);

            auto event_obj = v8::Object::New(isolate);
            if (event_obj->Set(context, utilities::StringToV8String(isolate, "request"), request_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set request on event", __FILE__, __LINE__));
                return;
            }
            if (event_obj->Set(context, utilities::StringToV8String(isolate, "response"), response_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set response on event", __FILE__, __LINE__));
                return;
            }

            if (state->pending_resolver.IsEmpty()) {
                log::debug(log::Message(__func__, "no resolver waiting, buffering request", __FILE__, __LINE__));
                v8::Global<v8::Object> global_event(isolate, event_obj);
                state->pending_requests.push_back(std::move(global_event));
                return;
            }

            v8::Local<v8::Promise::Resolver> resolver = state->pending_resolver.Get(isolate);
            state->pending_resolver.Reset();

            auto iter_result = v8::Object::New(isolate);
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "value"), event_obj).IsNothing()) {
                log::error(log::Message(__func__, "failed to set value", __FILE__, __LINE__));
                return;
            }
            if (iter_result->Set(context, utilities::StringToV8String(isolate, "done"), v8::Boolean::New(isolate, false)).IsNothing()) {
                log::error(log::Message(__func__, "failed to set done", __FILE__, __LINE__));
                return;
            }
            if (resolver->Resolve(context, iter_result).IsNothing()) {
                log::error(log::Message(__func__, "failed to resolve promise", __FILE__, __LINE__));
                return;
            }
            log::debug(log::Message(__func__, "state.use_count() inside lambda => " + std::to_string(state.use_count()), __FILE__, __LINE__));
            log::debug(log::Message(__func__, "promise resolved", __FILE__, __LINE__));
            log::trace(log::Message(__func__, "resolve task ends", __FILE__, __LINE__));
        });

        log::debug(log::Message(__func__, "state.use_count() after post => " + std::to_string(state.use_count()), __FILE__, __LINE__));
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
    }

} // namespace slim::plugin::http
