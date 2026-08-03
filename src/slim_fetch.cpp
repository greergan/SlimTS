#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/fetch.h>
#include <slim/runtime.h>
#include <slim/common/io/operations.h>
#include <slim/common/io/error_codes.h>
#include <slim/common/network/client/tcp.h>
#include <slim/common/network/error_codes.h>
#include <slim/common/http/url.h>
#include <slim/common/http/request.h>
#include <slim/common/http/response.h>
#include <slim/common/http/error_codes.h>
#include <openssl/ssl.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>  // std::search
#include <slim/common/utilities.h>
#include <filesystem>
#include <future>
#include <mutex>
#include <optional>   // std::optional
#include <string>

using slim::common::io::Close;
using slim::common::io::Open;
using slim::common::io::Read;
using slim::common::io::Scheduler;
using slim::common::io::Stat;
using slim::common::io::Task;
using slim::common::http::ErrorStatus;
using slim::common::http::Request;
using slim::common::http::Response;
using slim::common::http::URL;
using slim::common::network::client::tcp::Connection;

namespace slim::fetch {
using namespace slim::common;

namespace {

// ── SSL context ──────────────────────────────────────────────────────────────

SSL_CTX* get_ssl_ctx() {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    static SSL_CTX*   ctx = nullptr;
    static std::once_flag flag;
    std::call_once(flag, []() {
#ifdef ENABLE_LOGGING
        log::debug({__func__, "call_once: invoking SSL_CTX_new", __FILE__, __LINE__});
#endif
        ctx = SSL_CTX_new(TLS_client_method());
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("call_once: SSL_CTX_new => {}", (void*)ctx), __FILE__, __LINE__});
#endif
        if (ctx) {
            SSL_CTX_set_default_verify_paths(ctx);
#ifdef ENABLE_LOGGING
            log::debug({__func__, "call_once: SSL_CTX_set_default_verify_paths done", __FILE__, __LINE__});
#endif
        } else {
#ifdef ENABLE_LOGGING
            log::error({__func__, "call_once: SSL_CTX_new returned null", __FILE__, __LINE__});
#endif
        }
    });
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("returning ctx=>{}", (void*)ctx), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    return ctx;
}

// ── Error mapping ────────────────────────────────────────────────────────────

int errno_to_http(int err) {
    switch (err) {
        case ENOENT:       return 404;
        case ENOTDIR:      return 404;
        case EACCES:       return 403;
        case EPERM:        return 403;
        case ENAMETOOLONG: return 400;
        case ELOOP:        return 400;
        default:           return 500;
    }
}

int error_status_to_http(ErrorStatus e) {
    switch (e) {
        case ErrorStatus::OK:                                    return 200;
        case ErrorStatus::BadAllocation:                         return 500;
        case ErrorStatus::UrlStringEmpty:
        case ErrorStatus::UrlInvalidControlCharacter:
        case ErrorStatus::UrlSchemeInvalidCharacter:
        case ErrorStatus::UrlSchemeDelimiterMissing:
        case ErrorStatus::UrlSchemeUnsupported:
        case ErrorStatus::UrlHostInvalidStart:
        case ErrorStatus::UrlHostInvalidCharacter:
        case ErrorStatus::UrlHostMissing:
        case ErrorStatus::UrlPortInvalidCharacter:
        case ErrorStatus::UrlBodyInvalidCharacter:
        case ErrorStatus::UrlFilePathMissing:
        case ErrorStatus::UrlFilePathTrailingSlash:
        case ErrorStatus::UrlUnparsable:                         return 400;
        case ErrorStatus::RequestStringEmpty:
        case ErrorStatus::RequestStatusLineInvalid:
        case ErrorStatus::RequestStatusLineMalformed:
        case ErrorStatus::RequestHeadersNotTerminated:           return 400;
        case ErrorStatus::HeaderDelimiterInvalid:
        case ErrorStatus::HeaderNameEmpty:
        case ErrorStatus::HeaderNameInvalidChar:
        case ErrorStatus::HeaderValueEmpty:
        case ErrorStatus::HeaderValueInvalidChar:
        case ErrorStatus::HeaderValueInvalidFolding:             return 400;
        case ErrorStatus::CookieDomainBareDot:
        case ErrorStatus::CookieDomainEmpty:
        case ErrorStatus::CookieDomainInvalidChar:
        case ErrorStatus::CookieDomainLabelEmpty:
        case ErrorStatus::CookieDomainLabelInvalidHyphen:
        case ErrorStatus::CookieDomainLabelTooLong:
        case ErrorStatus::CookieDomainNumericTld:
        case ErrorStatus::CookieDomainTooLong:
        case ErrorStatus::CookieDomainTrailingDot:
        case ErrorStatus::CookieEmptyString:
        case ErrorStatus::CookieExpiresInvalidFormat:
        case ErrorStatus::CookieInvalidBoolean:
        case ErrorStatus::CookieInvalidName:
        case ErrorStatus::CookieInvalidValue:
        case ErrorStatus::CookieMalformedMissingEquals:
        case ErrorStatus::CookieMalformedPairMissingEquals:
        case ErrorStatus::CookieMaxAgeEmpty:
        case ErrorStatus::CookieMaxAgeExceedsLimit:
        case ErrorStatus::CookieMaxAgeInvalidFormat:
        case ErrorStatus::CookieMaxAgeTrailingChars:
        case ErrorStatus::CookieNameEmpty:
        case ErrorStatus::CookieNameHostPrefixHasDomain:
        case ErrorStatus::CookieNameHostPrefixInvalidPath:
        case ErrorStatus::CookieNameInvalidChar:
        case ErrorStatus::CookieNamePrefixRequiresSecure:
        case ErrorStatus::CookiePartitionedRequiresSameSiteNone:
        case ErrorStatus::CookiePartitionedRequiresSecure:
        case ErrorStatus::CookiePathInvalidChar:
        case ErrorStatus::CookiePathMissingLeadingSlash:
        case ErrorStatus::CookieSameSiteInvalid:
        case ErrorStatus::CookieSameSiteNoneRequiresSecure:
        case ErrorStatus::CookieTooLarge:
        case ErrorStatus::CookieValueInvalidChar:
        case ErrorStatus::CookieValueUnmatchedQuote:             return 400;
        case ErrorStatus::SearchParamNameEmpty:
        case ErrorStatus::SearchParamNameInvalidChar:
        case ErrorStatus::SearchParamValueInvalidChar:
        case ErrorStatus::SearchParamInvalidPercentEncoding:     return 400;
        case ErrorStatus::ResponseStorageEmpty:
        case ErrorStatus::ResponseStatusLineInvalid:
        case ErrorStatus::ResponseStatusCodeInvalid:
        case ErrorStatus::ResponseStatusCodeOutOfRange:
        case ErrorStatus::ResponseHeadersTerminatorMalformed:
        case ErrorStatus::ResponseHeadersBareCR:
        case ErrorStatus::ResponseHeadersNotTerminated:
        case ErrorStatus::ResponseChunkedMissingCRLF:
        case ErrorStatus::ResponseChunkedSizeInvalid:
        case ErrorStatus::ResponseChunkedTruncated:
        case ErrorStatus::ResponseChunkedMissingCRLFAfterData:
        case ErrorStatus::ResponseContentLengthInvalid:
        case ErrorStatus::ResponseContentLengthMismatch:         return 502;
        default:                                                 return 500;
    }
}

int network_error_to_http(slim::common::network::ErrorStatus e) {
    switch (e) {
        case slim::common::network::ErrorStatus::OutOfMemory:        return 500;
        // Timeouts → 504 Gateway Timeout
        case slim::common::network::ErrorStatus::SocketConnectionTimedOut:
        case slim::common::network::ErrorStatus::TlsHandshakeTimedOut:
        case slim::common::network::ErrorStatus::WriteTimedOut:      return 504;
        // Everything else → 502 Bad Gateway (upstream unreachable or failed)
        default:                                                     return 502;
    }
}

int io_error_to_http(slim::common::io::ErrorStatus e) {
    switch (e) {
        case slim::common::io::ErrorStatus::BadAllocation:           return 500;
        // Transient scheduler/io_uring failures → 503 Service Unavailable
        default:                                                     return 503;
    }
}

// ── HTTP reason phrases ───────────────────────────────────────────────────────

std::string_view slim_http_reason(int code) {
    switch (code) {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default:  return "Unknown";
    }
}

Response make_error(ErrorStatus e) {
    Response r;
    r.code      = error_status_to_http(e);
    r.code_text = slim_http_reason(r.code);
    return r;
}

Response make_error(int http_code) {
    Response r;
    r.code      = http_code;
    r.code_text = slim_http_reason(http_code);
    return r;
}

// ── Incremental read helpers ──────────────────────────────────────────────────

// Read from conn until \r\n\r\n found in raw.
// Returns byte offset just past the terminator, -1 on failure.
Task<ptrdiff_t> read_headers(Connection& conn, std::vector<uint8_t>& raw) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    static constexpr uint8_t term[] = {'\r', '\n', '\r', '\n'};
    std::vector<uint8_t> chunk;
    size_t search_from = 0;
    for (;;) {
        if (raw.size() >= 4) {
            auto begin = raw.begin() + static_cast<ptrdiff_t>(search_from);
            auto it    = std::search(begin, raw.end(), std::begin(term), std::end(term));
            if (it != raw.end()) {
                ptrdiff_t end = (it - raw.begin()) + 4;
#ifdef ENABLE_LOGGING
                log::debug({__func__, std::format("header_end=>{}", end), __FILE__, __LINE__});
                log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
                co_return end;
            }
        }
        search_from = raw.size() >= 3 ? raw.size() - 3 : 0;
        chunk.clear();
        co_await conn.read(chunk);
        if (chunk.empty()) {
#ifdef ENABLE_LOGGING
            log::error({__func__, "connection closed before headers complete", __FILE__, __LINE__});
#endif
            co_return -1;
        }
        raw.insert(raw.end(), chunk.begin(), chunk.end());
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("raw.size()=>{}", raw.size()), __FILE__, __LINE__});
#endif
    }
}

// Case-insensitive scan for Content-Length value in raw[0..header_end].
std::optional<size_t> scan_content_length(const std::vector<uint8_t>& raw, size_t header_end) {
    using namespace slim::common::utilities;
    static constexpr std::string_view needle = "content-length:";
    const char* data = reinterpret_cast<const char*>(raw.data());
    const char* end  = data + header_end;
    const char* it   = data;
    for (; it + static_cast<ptrdiff_t>(needle.size()) <= end; ++it) {
        if (iequals(std::string_view(it, needle.size()), needle)) {
            it += needle.size();
            while (it < end && *it == ' ') ++it;
            size_t val   = 0;
            bool   found = false;
            while (it < end && is_digit(*it)) {
                val = val * 10 + static_cast<size_t>(*it - '0');
                ++it;
                found = true;
            }
            return found ? std::optional<size_t>(val) : std::nullopt;
        }
    }
    return std::nullopt;
}

// Case-insensitive scan for Transfer-Encoding: chunked in raw[0..header_end].
bool scan_chunked(const std::vector<uint8_t>& raw, size_t header_end) {
    using namespace slim::common::utilities;
    static constexpr std::string_view needle  = "transfer-encoding:";
    static constexpr std::string_view chunked = "chunked";
    const char* data = reinterpret_cast<const char*>(raw.data());
    const char* end  = data + header_end;
    for (const char* it = data; it + static_cast<ptrdiff_t>(needle.size()) <= end; ++it) {
        if (iequals(std::string_view(it, needle.size()), needle)) {
            it += needle.size();
            while (it < end && *it == ' ') ++it;
            if (it + static_cast<ptrdiff_t>(chunked.size()) > end) return false;
            return iequals(std::string_view(it, chunked.size()), chunked);
        }
    }
    return false;
}

// Read until raw contains exactly header_end + content_length bytes.
Task<bool> read_body_fixed(Connection& conn, std::vector<uint8_t>& raw,
                            size_t header_end, size_t content_length) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("header_end=>{} content_length=>{}", header_end, content_length), __FILE__, __LINE__});
#endif
    const size_t         target = header_end + content_length;
    std::vector<uint8_t> chunk;
    while (raw.size() < target) {
        chunk.clear();
        co_await conn.read(chunk);
        if (chunk.empty()) {
#ifdef ENABLE_LOGGING
            log::error({__func__, std::format("truncated: got=>{} want=>{}", raw.size(), target), __FILE__, __LINE__});
#endif
            co_return false;
        }
        raw.insert(raw.end(), chunk.begin(), chunk.end());
    }
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("done raw.size()=>{}", raw.size()), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    co_return true;
}

// Read until terminal chunk (0\r\n\r\n) is present in raw.
Task<bool> read_body_chunked(Connection& conn, std::vector<uint8_t>& raw) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
#endif
    static constexpr uint8_t term[] = {'0', '\r', '\n', '\r', '\n'};
    std::vector<uint8_t> chunk;
    size_t search_from = 0;
    for (;;) {
        auto begin = raw.begin() + static_cast<ptrdiff_t>(search_from);
        auto it    = std::search(begin, raw.end(), std::begin(term), std::end(term));
        if (it != raw.end()) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("terminal chunk found raw.size()=>{}", raw.size()), __FILE__, __LINE__});
            log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
            co_return true;
        }
        search_from = raw.size() >= 4 ? raw.size() - 4 : 0;
        chunk.clear();
        co_await conn.read(chunk);
        if (chunk.empty()) {
#ifdef ENABLE_LOGGING
            log::error({__func__, "connection closed before terminal chunk", __FILE__, __LINE__});
#endif
            co_return false;
        }
        raw.insert(raw.end(), chunk.begin(), chunk.end());
    }
}

// ── Local file fetch ─────────────────────────────────────────────────────────

Task<Response> fetch_local(Scheduler& scheduler, std::string_view path) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("path=>'{}'", path), __FILE__, __LINE__});
#endif
    std::string resolved;
    try {
        std::filesystem::path p(path);
        if (!p.is_absolute()) {
            p = std::filesystem::absolute(p);
        }
        resolved = p.string();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("resolved=>'{}'", resolved), __FILE__, __LINE__});
#endif
    } catch (...) {
#ifdef ENABLE_LOGGING
        log::error({__func__, "exception resolving path", __FILE__, __LINE__});
#endif
        auto r = make_error(500);
        r.code_text = "Internal error";
        co_return r;
    }

    Stat stat_op(scheduler, resolved);
    int stat_res = co_await stat_op;
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("stat=>{}", stat_res), __FILE__, __LINE__});
#endif
    if (stat_res < 0) {
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("stat failed errno=>{}", -stat_op.result), __FILE__, __LINE__});
#endif
        auto r = make_error(errno_to_http(-stat_op.result));
        r.code_text = strerror(-stat_op.result);
        co_return r;
    }
    std::vector<uint32_t> word_buf((stat_op.buf_.stx_size + sizeof(uint32_t) - 1) / sizeof(uint32_t));
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("file size=>{} word_buf slots=>{}", stat_op.buf_.stx_size, word_buf.size()), __FILE__, __LINE__});
#endif

    Open open_op(scheduler, resolved, O_RDONLY);
    int fd = co_await open_op;
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("open => fd=>{}", fd), __FILE__, __LINE__});
#endif
    if (fd < 0) {
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("open failed errno=>{}", -open_op.result), __FILE__, __LINE__});
#endif
        auto r = make_error(errno_to_http(-open_op.result));
        r.code_text = strerror(-open_op.result);
        co_return r;
    }

    Read read_op(scheduler, fd, word_buf);
    int bytes_read = co_await read_op;
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("read => bytes=>{}", bytes_read), __FILE__, __LINE__});
#endif

    Close close_op(scheduler, fd);
    co_await close_op;
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("closed fd=>{}", fd), __FILE__, __LINE__});
#endif

    if (bytes_read < 0) {
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("read failed errno=>{}", -read_op.result), __FILE__, __LINE__});
#endif
        auto r = make_error(errno_to_http(-read_op.result));
        r.code_text = strerror(-read_op.result);
        co_return r;
    }

    auto* data = reinterpret_cast<uint8_t*>(word_buf.data());

    Response r;
    r.code    = 200;
    r.version = "HTTP/1.1";
    r.body    = std::vector<uint8_t>(data, data + static_cast<size_t>(bytes_read));
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("returning 200 body_size=>{}", r.body.size()), __FILE__, __LINE__});
    log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
    co_return r;
}

// ── HTTP fetch ───────────────────────────────────────────────────────────────

Task<Response> fetch_http(Scheduler& scheduler, const URL& url, SSL_CTX* ssl_ctx, int redirects_left);

Task<Response> fetch_http(Scheduler& scheduler, const URL& url, SSL_CTX* ssl_ctx, int redirects_left) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, "begins", __FILE__, __LINE__});
    log::debug({__func__, std::format("url='{}' => ssl_ctx=>{} => redirects_left=>{}", url.href(), (void*)ssl_ctx, redirects_left), __FILE__, __LINE__});
#endif
    std::string host_str(url.hostname());
    std::string port_str(url.port());
    uint16_t port = 0;
    if (port_str.empty()) {
        port = (url.protocol() == "https") ? 443 : 80;
    } else {
        for (char c : port_str) port = static_cast<uint16_t>(port * 10 + (c - '0'));
    }
#ifdef ENABLE_LOGGING
    log::debug({__func__, std::format("host=>'{}' => port=>{}", host_str, port), __FILE__, __LINE__});
#endif

    try {
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("creating connection ssl=>{}", ssl_ctx ? "yes" : "no"), __FILE__, __LINE__});
#endif
        Connection conn = ssl_ctx
            ? co_await Connection::create(scheduler, host_str, port, ssl_ctx)
            : co_await Connection::create(scheduler, host_str, port);
#ifdef ENABLE_LOGGING
        log::debug({__func__, "connection established", __FILE__, __LINE__});
#endif

        Request req(url);
        co_await conn.write(req.serialize());
#ifdef ENABLE_LOGGING
        log::debug({__func__, "request written", __FILE__, __LINE__});
#endif

        std::vector<uint8_t> raw;
        ptrdiff_t hdr_end = co_await read_headers(conn, raw);
        if (hdr_end < 0) {
#ifdef ENABLE_LOGGING
            log::error({__func__, "headers incomplete => 502", __FILE__, __LINE__});
#endif
            co_return make_error(502);
        }

        auto cl  = scan_content_length(raw, static_cast<size_t>(hdr_end));
        bool chk = scan_chunked(raw, static_cast<size_t>(hdr_end));
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("hdr_end=>{} content_length=>{} chunked=>{}",
            hdr_end, cl ? std::to_string(*cl) : "none", chk), __FILE__, __LINE__});
#endif

        if (cl) {
            if (!co_await read_body_fixed(conn, raw, static_cast<size_t>(hdr_end), *cl)) {
#ifdef ENABLE_LOGGING
                log::error({__func__, "read_body_fixed failed => 502", __FILE__, __LINE__});
#endif
                co_return make_error(502);
            }
        } else if (chk) {
            if (!co_await read_body_chunked(conn, raw)) {
#ifdef ENABLE_LOGGING
                log::error({__func__, "read_body_chunked failed => 502", __FILE__, __LINE__});
#endif
                co_return make_error(502);
            }
        } else {
            // fallback: no Content-Length, not chunked — read until EOF
            std::vector<uint8_t> chunk;
            for (;;) {
                chunk.clear();
                co_await conn.read(chunk);
                if (chunk.empty()) break;
                raw.insert(raw.end(), chunk.begin(), chunk.end());
            }
        }
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("raw response bytes=>{}", raw.size()), __FILE__, __LINE__});
#endif

        if (raw.empty()) {
#ifdef ENABLE_LOGGING
            log::error({__func__, "raw response empty => 502", __FILE__, __LINE__});
#endif
            co_return make_error(502);
        }

        Response r;
        ErrorStatus es = r.parse(std::span<const uint8_t>(raw.data(), raw.size()));
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("parse => es=>{} => code=>{}", static_cast<int>(es), r.code), __FILE__, __LINE__});
#endif
        if (es != ErrorStatus::OK) {
#ifdef ENABLE_LOGGING
            log::error({__func__, std::format("parse failed es=>{}", static_cast<int>(es)), __FILE__, __LINE__});
#endif
            co_return make_error(es);
        }

        // Follow redirects (301, 302, 303, 307, 308).
        if (redirects_left > 0 &&
            (r.code == 301 || r.code == 302 ||
                r.code == 303 || r.code == 307 || r.code == 308)) {
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("redirect code=>{} => redirects_left=>{}", r.code, redirects_left), __FILE__, __LINE__});
#endif
            auto location_header = r.headers.get("Location");
            if (!location_header) {
#ifdef ENABLE_LOGGING
                log::error({__func__, "redirect missing Location header => 502", __FILE__, __LINE__});
#endif
                co_return make_error(502);
            }
            const auto& values = location_header->get_value();
            if (values.empty()) {
#ifdef ENABLE_LOGGING
                log::error({__func__, "redirect Location header empty => 502", __FILE__, __LINE__});
#endif
                co_return make_error(502);
            }
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("redirect location=>'{}'", values[0]), __FILE__, __LINE__});
#endif

            URL next;
            try {
                next = URL(values[0]);
            } catch (const slim::common::http::UrlParseException& e) {
#ifdef ENABLE_LOGGING
                log::error({__func__, std::format("redirect URL parse failed: {}", e.what()), __FILE__, __LINE__});
#endif
                co_return make_error(error_status_to_http(e.error()));
            } catch (...) {
#ifdef ENABLE_LOGGING
                log::error({__func__, "redirect URL parse unknown exception", __FILE__, __LINE__});
#endif
                co_return make_error(500);
            }

            SSL_CTX* next_ssl = (next.protocol() == "https") ? ssl_ctx : nullptr;
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("following redirect to '{}' => ssl=>{}", next.href(), next_ssl ? "yes" : "no"), __FILE__, __LINE__});
#endif
            co_return co_await fetch_http(scheduler, next, next_ssl, redirects_left - 1);
        }

#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("returning code=>{} => body_size=>{}", r.code, r.body.size()), __FILE__, __LINE__});
        log::trace({__func__, "ends", __FILE__, __LINE__});
#endif
        co_return r;

    } catch (const slim::common::network::NetworkException& e) {
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("network exception: {}", e.what()), __FILE__, __LINE__});
#endif
        auto r = make_error(network_error_to_http(e.error()));
        r.code_text = e.what();
        co_return r;
    } catch (const slim::common::io::IOException& e) {
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("io exception: {}", e.what()), __FILE__, __LINE__});
#endif
        auto r = make_error(io_error_to_http(e.status()));
        r.code_text = e.what();
        co_return r;
    } catch (const std::exception& e) {
#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("exception: {}", e.what()), __FILE__, __LINE__});
#endif
        auto r = make_error(500);
        r.code_text = e.what();
        co_return r;
    } catch (...) {
#ifdef ENABLE_LOGGING
        log::error({__func__, "unknown exception", __FILE__, __LINE__});
#endif
        co_return make_error(500);
    }
}

} // namespace

// ── Public API ───────────────────────────────────────────────────────────────

std::future<slim::common::http::Response> fetch_file(std::string_view uri) {
#ifdef ENABLE_LOGGING
    log::trace({__func__, std::format("begins uri=>'{}'", uri), __FILE__, __LINE__});
    log::debug({__func__, std::format("uri=>'{}'", uri), __FILE__, __LINE__});
#endif
    auto promise = std::make_shared<std::promise<Response>>();
    auto future  = promise->get_future();

    slim::runtime::instance().post([uri, promise](Scheduler& scheduler, size_t) {
#ifdef ENABLE_LOGGING
        log::trace({__func__, "post lambda begins", __FILE__, __LINE__});
#endif
        URL url;
        try {
            url = URL(uri);
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("{} => {} => hostname=>'{}' => pathname=>'{}'",
                uri, url.protocol(), url.hostname(), url.pathname()), __FILE__, __LINE__});
#endif
        } catch (const slim::common::http::UrlParseException& e) {
#ifdef ENABLE_LOGGING
            log::error({__func__, std::format("URL parse failed: {}", e.what()), __FILE__, __LINE__});
#endif
            Response r;
            r.code      = error_status_to_http(e.error());
            r.code_text = e.what();
            promise->set_value(std::move(r));
            return;
        } catch (...) {
#ifdef ENABLE_LOGGING
            log::error({__func__, "URL parse unknown exception", __FILE__, __LINE__});
#endif
            promise->set_value(make_error(500));
            return;
        }

        auto protocol = url.protocol();
#ifdef ENABLE_LOGGING
        log::debug({__func__, std::format("{} => protocol=>'{}'", uri, protocol), __FILE__, __LINE__});
#endif

        if (protocol == "file") {
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("{} => dispatching to fetch_local", uri), __FILE__, __LINE__});
#endif
            scheduler.spawn([](Scheduler& sched, std::string path, std::shared_ptr<std::promise<Response>> p) -> Task<void> {
                auto r = co_await fetch_local(sched, path);
                p->set_value(std::move(r));
            }(scheduler, std::string(url.pathname()), promise));
            return;
        }

        if (protocol == "http" || protocol == "https") {
            SSL_CTX* ctx = (protocol == "https") ? get_ssl_ctx() : nullptr;
#ifdef ENABLE_LOGGING
            log::debug({__func__, std::format("{} => dispatching to fetch_http ssl_ctx=>{}", uri, (void*)ctx), __FILE__, __LINE__});
#endif
            scheduler.spawn([](Scheduler& sched, URL u, SSL_CTX* ssl, std::shared_ptr<std::promise<Response>> p) -> Task<void> {
                auto r = co_await fetch_http(sched, u, ssl, /*redirects_left=*/2);
                p->set_value(std::move(r));
            }(scheduler, std::move(url), ctx, promise));
            return;
        }

#ifdef ENABLE_LOGGING
        log::error({__func__, std::format("{} => unsupported protocol=>'{}'", uri, protocol), __FILE__, __LINE__});
#endif
        promise->set_value(make_error(ErrorStatus::UrlSchemeUnsupported));
    });

#ifdef ENABLE_LOGGING
    log::trace({__func__, std::format("ends uri=>'{}'", uri), __FILE__, __LINE__});
#endif
    return future;
}

} // namespace slim
