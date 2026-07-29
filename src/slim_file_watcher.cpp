#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include <slim/common/log.h>
#include <slim/file/watcher.h>

namespace slim::file::watcher {
    using namespace slim::common;
    static std::vector<std::string> watched_paths;
    static std::function<void()> restart_callback;
    static std::atomic<bool> running{false};
    static int inotify_fd{-1};
    static int pipe_fd[2]{-1, -1};
    static std::unordered_map<int, std::string> wd_to_path;
}

void slim::file::watcher::add(std::string_view path) {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    //watched_paths.emplace_back(path.substr(7));
    watched_paths.emplace_back(path);
    log::debug(log::Message(__func__, std::format("adding path => {}", watched_paths.back()), __FILE__, __LINE__));
    if(inotify_fd >= 0) {
        int wd = inotify_add_watch(inotify_fd, watched_paths.back().c_str(), IN_CLOSE_WRITE);
        if(wd >= 0) {
            wd_to_path[wd] = watched_paths.back();
            log::debug(log::Message(__func__, std::format("registered watch => {}", watched_paths.back()), __FILE__, __LINE__));
        } else {
            throw std::runtime_error(std::format("inotify_add_watch failed for path => {}: {}", watched_paths.back(), strerror(errno)));
        }
    }
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}

// Clears registered watches and bookkeeping only. Does NOT touch
// running or inotify_fd -- those are owned exclusively by stop()
// (signals the loop) and watch() (owns the fd's lifetime on its own
// thread), so clear() stays safe to call from the watch loop's own
// on_change callback without killing the loop or the fd out from
// under itself.
void slim::file::watcher::clear() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    if(inotify_fd >= 0) {
        for(auto& [wd, path] : wd_to_path) {
            log::debug(log::Message(__func__, std::format("removing watch => {}", path), __FILE__, __LINE__));
            inotify_rm_watch(inotify_fd, wd);
        }
    }
    watched_paths.clear();
    wd_to_path.clear();
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}

void slim::file::watcher::on_change(std::function<void()> restart) {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    restart_callback = std::move(restart);
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}

// Thread-safe: only flips the atomic running flag and writes to the
// pipe to wake watch()'s select() on whatever thread it's blocked on.
// inotify_fd is never touched here -- it's closed by watch() itself,
// on the watcher thread, after the loop breaks.
void slim::file::watcher::stop() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    running = false;
    if(pipe_fd[1] >= 0) {
        char byte = 0;
        write(pipe_fd[1], &byte, 1);
    }
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}

void slim::file::watcher::watch() {
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
    inotify_fd = inotify_init();
    if(inotify_fd < 0) {
        throw std::runtime_error(std::format("inotify_init failed: {}", strerror(errno)));
    }
    if(pipe(pipe_fd) < 0) {
        throw std::runtime_error(std::format("pipe failed: {}", strerror(errno)));
    }
    for(auto& path : watched_paths) {
        int wd = inotify_add_watch(inotify_fd, path.c_str(), IN_CLOSE_WRITE);
        if(wd >= 0) {
            wd_to_path[wd] = path;
            log::debug(log::Message(__func__, std::format("watching path => {}", path), __FILE__, __LINE__));
        } else {
            throw std::runtime_error(std::format("inotify_add_watch failed for path => {}: {}", path, strerror(errno)));
        }
    }
    running = true;
    constexpr int DEBOUNCE_MS = 500;
    constexpr int EVENT_BUF_SIZE = 1024 * (sizeof(inotify_event) + 16);
    char event_buf[EVENT_BUF_SIZE];
    std::chrono::steady_clock::time_point last_event_time;
    bool pending = false;
    while(running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(inotify_fd, &fds);
        FD_SET(pipe_fd[0], &fds);
        int max_fd = std::max(inotify_fd, pipe_fd[0]) + 1;
        timeval timeout{0, pending ? DEBOUNCE_MS * 1000 : 100 * 1000};
        int result = select(max_fd, &fds, nullptr, nullptr, &timeout);
        if(!running) {
            break;
        }
        if(result < 0) {
            throw std::runtime_error(std::format("select failed: {}", strerror(errno)));
        }
        if(FD_ISSET(pipe_fd[0], &fds)) {
            log::debug(log::Message(__func__, "stop signal received", __FILE__, __LINE__));
            break;
        }
        if(FD_ISSET(inotify_fd, &fds)) {
            read(inotify_fd, event_buf, EVENT_BUF_SIZE);
            last_event_time = std::chrono::steady_clock::now();
            pending = true;
            log::debug(log::Message(__func__, "file change event received", __FILE__, __LINE__));
        }
        if(pending) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - last_event_time).count();
            if(elapsed >= DEBOUNCE_MS) {
                pending = false;
                log::debug(log::Message(__func__, "debounce elapsed, invoking restart callback", __FILE__, __LINE__));
                if(restart_callback) {
                    restart_callback();
                    // drain stale events accumulated during restart
                    char drain_buf[EVENT_BUF_SIZE];
                    fd_set drain_fds;
                    timeval zero{0, 0};
                    do {
                        FD_ZERO(&drain_fds);
                        FD_SET(inotify_fd, &drain_fds);
                        zero = {0, 0};
                    } while(select(inotify_fd + 1, &drain_fds, nullptr, nullptr, &zero) > 0
                            && read(inotify_fd, drain_buf, EVENT_BUF_SIZE) > 0);
                }
            }
        }
    }
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    pipe_fd[0] = pipe_fd[1] = -1;
    if(inotify_fd >= 0) {
        close(inotify_fd);
        inotify_fd = -1;
    }
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
}
