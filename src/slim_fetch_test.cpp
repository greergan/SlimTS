#include <chrono>
#include <filesystem>
#include <fstream>
#include <slim/fetch.h>
#include <slim/runtime.h>
#include <slim/common/log.h>
#include <slim/common/http/response.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <format>
#include <thread>
#include <string>
#include <string_view>
#include <cstring>

namespace slim::fetch {

namespace {

// ── Helpers ──────────────────────────────────────────────────────────────────

void pass(std::string_view name, int& passed) {
    slim::common::log::debug({__func__, std::format("PASS => {}", name), __FILE__, __LINE__});
    ++passed;
}

void fail(std::string_view name, int& failed, const slim::common::http::Response& r) {
    slim::common::log::error({__func__, std::format("FAIL => {} => code={} => text={}", name, r.code, r.code_text), __FILE__, __LINE__});
    ++failed;
}

// ── Mock HTTP server ─────────────────────────────────────────────────────────

uint16_t start_mock_server(std::string response_str) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = 0;
    bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    listen(server_fd, 1);

    socklen_t len = sizeof(addr);
    getsockname(server_fd, reinterpret_cast<sockaddr*>(&addr), &len);
    uint16_t port = ntohs(addr.sin_port);

    std::thread([server_fd, response_str]() {
        int client_fd = accept(server_fd, nullptr, nullptr);
        char buf[4096];
        recv(client_fd, buf, sizeof(buf), 0);
        send(client_fd, response_str.data(), response_str.size(), 0);
        close(client_fd);
        close(server_fd);
    }).detach();

    return port;
}

// ── Tests ────────────────────────────────────────────────────────────────────

void run_tests(int& passed, int& failed) {

    // 1. file:// valid file with content
    {
        std::string_view name = "file:// valid file returns 200 with body";
        std::string_view uri  = "file:///etc/hostname";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 200 && !r.body.empty()) pass(name, passed);
        else                                  fail(name, failed, r);
    }

    // 2. file:// empty file
    {
        std::string_view name = "file:// empty file returns 200";
        std::string_view uri  = "file:///dev/null";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 200) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 3. file:// file not found
    {
        std::string_view name = "file:// missing file returns 404";
        std::string_view uri  = "file:///nonexistent_slim_test_file_xyz";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 404) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 4. file:// permission denied
    {
        std::string_view name = "file:// permission denied returns 403";
        std::string_view uri  = "file:///etc/shadow";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 403) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 5a. file:// relative path in CWD resolves correctly
        {
            std::string_view name = "file:// relative path in CWD returns 200";
            auto tag      = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
            std::string filename  = "slim_test_" + tag + ".txt";
            { std::ofstream f(filename); }
            auto uri = "file://" + filename;
            auto r = slim::fetch::fetch_file(uri).get();
            std::filesystem::remove(filename);
            if (r.code == 200) pass(name, passed);
            else               fail(name, failed, r);
        }

        // 5b. file:// relative path in subdir resolves correctly
        {
            std::string_view name = "file:// relative path in subdir returns 200";
            auto tag      = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
            std::string subdir    = "slim_test_dir_" + tag;
            std::string filename  = subdir + "/slim_test_" + tag + ".txt";
            std::filesystem::create_directory(subdir);
            { std::ofstream f(filename); }
            auto uri = "file://" + filename;
            auto r = slim::fetch::fetch_file(uri).get();
            std::filesystem::remove_all(subdir);
            if (r.code == 200) pass(name, passed);
            else               fail(name, failed, r);
        }

    // 6. file:// trailing slash rejected
    {
        std::string_view name = "file:// trailing slash returns 400";
        std::string_view uri  = "file:///etc/";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 400) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 7. empty URI
    {
        std::string_view name = "empty URI returns 400";
        std::string_view uri  = "";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 400) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 8. unsupported scheme
    {
        std::string_view name = "unsupported scheme returns 400";
        std::string_view uri  = "ftp://example.com/file.txt";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 400) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 9. malformed URI
    {
        std::string_view name = "malformed URI returns 400";
        std::string_view uri  = "not_a_uri";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 400) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 10. http:// valid 200
    {
        std::string_view name = "http:// example.com returns 200 with body";
        std::string_view uri  = "http://example.com/";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 200 && !r.body.empty()) pass(name, passed);
        else                                  fail(name, failed, r);
    }

    // 11. http:// 404 from server
    {
        std::string_view name = "http:// missing resource returns 404";
        std::string_view uri  = "http://example.com/doesnotexist";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 404) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 12. http:// redirect followed
    {
        std::string_view name = "http:// redirect to https returns 200";
        std::string_view uri  = "http://example.com/";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 200) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 13. https:// valid 200
    {
        std::string_view name = "https:// example.com returns 200 with body";
        std::string_view uri  = "https://example.com/";
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 200 && !r.body.empty()) pass(name, passed);
        else                                  fail(name, failed, r);
    }

    // 14. mock: server closes connection immediately (no data)
    {
        std::string_view name = "mock: server closes with no data returns 502";
        uint16_t port = start_mock_server("");
        auto uri = std::format("http://127.0.0.1:{}/", port);
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 502) pass(name, passed);
        else               fail(name, failed, r);
    }

    // 15. mock: malformed HTTP response
    {
        std::string_view name = "mock: malformed response returns 502";
        uint16_t port = start_mock_server("GARBAGE RESPONSE\r\n\r\n");
        auto uri = std::format("http://127.0.0.1:{}/", port);
        auto r = slim::fetch::fetch_file(uri).get();
        if (r.code == 502) pass(name, passed);
        else               fail(name, failed, r);
    }
}

} // namespace

// ── Public ───────────────────────────────────────────────────────────────────

void test() {
    int passed = 0;
    int failed = 0;


    run_tests(passed, failed);

    if (failed == 0) {
        slim::common::log::debug({__func__, std::format("Results => {} passed => {} failed", passed, failed), __FILE__, __LINE__});
    } else {
        slim::common::log::error({__func__, std::format("Results => {} passed => {} failed", passed, failed), __FILE__, __LINE__});
    }

}

} // namespace slim::fetch
