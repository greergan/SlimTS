#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <sys/inotify.h>
#include <sys/select.h>
#include <unistd.h>
#include "config.h"
#ifdef ENABLE_LOGGING
#include <slim/common/log.h>
#endif
#include <slim/file/watcher.h>

namespace slim::common {}
namespace slim::file::watcher {
    using namespace slim::common;
    static std::vector<std::string> watched_paths;
    static std::function<void()> restart_callback;
    static std::atomic<bool> running{false};
    static int inotify_fd{-1};
    static int pipe_fd[2]{-1, -1};
    static std::unordered_map<int, std::string> wd_to_path;
    static std::unordered_map<int, std::string> wd_to_dir;
}

void slim::file::watcher::add(std::string_view path) {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    //watched_paths.emplace_back(path.substr(7));
    watched_paths.emplace_back(path);
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__, std::format("adding path => {}", watched_paths.back()), __FILE__, __LINE__));
#endif
    if(inotify_fd >= 0) {
        int wd = inotify_add_watch(inotify_fd, watched_paths.back().c_str(), IN_CLOSE_WRITE);
        if(wd >= 0) {
            wd_to_path[wd] = watched_paths.back();
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, std::format("registered watch => {}", watched_paths.back()), __FILE__, __LINE__));
#endif
        } else {
            throw std::runtime_error(std::format("inotify_add_watch failed for path => {}: {}", watched_paths.back(), strerror(errno)));
        }
    }
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}

// Clears registered watches and bookkeeping only. Does NOT touch
// running or inotify_fd -- those are owned exclusively by stop()
// (signals the loop) and watch() (owns the fd's lifetime on its own
// thread), so clear() stays safe to call from the watch loop's own
// on_change callback without killing the loop or the fd out from
// under itself.
void slim::file::watcher::clear() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    if(inotify_fd >= 0) {
        for(auto& [wd, path] : wd_to_path) {
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, std::format("removing watch => {}", path), __FILE__, __LINE__));
#endif
            inotify_rm_watch(inotify_fd, wd);
        }
        for(auto& [wd, dir] : wd_to_dir) {
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, std::format("removing dir watch => {}", dir), __FILE__, __LINE__));
#endif
            inotify_rm_watch(inotify_fd, wd);
        }
    }
    watched_paths.clear();
    wd_to_path.clear();
    wd_to_dir.clear();
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}

void slim::file::watcher::on_change(std::function<void()> restart) {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    restart_callback = std::move(restart);
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}

// Thread-safe: only flips the atomic running flag and writes to the
// pipe to wake watch()'s select() on whatever thread it's blocked on.
// inotify_fd is never touched here -- it's closed by watch() itself,
// on the watcher thread, after the loop breaks.
void slim::file::watcher::stop() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
    running = false;
    if(pipe_fd[1] >= 0) {
        char byte = 0;
        if (write(pipe_fd[1], &byte, 1) < 0) {
#ifdef ENABLE_LOGGING
            log::error(log::Message(__func__, "write to pipe failed => " + std::string(strerror(errno)), __FILE__, __LINE__));
#endif
        }
    }
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}

void slim::file::watcher::watch() {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
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
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, std::format("watching path => {}", path), __FILE__, __LINE__));
#endif
        } else {
            throw std::runtime_error(std::format("inotify_add_watch failed for path => {}: {}", path, strerror(errno)));
        }
    }
    // register any directories that were queued before inotify_fd was ready
    std::vector<std::string> deferred_dirs;
    for(auto& [wd, dir] : wd_to_dir) {
        if(wd < 0) {
            deferred_dirs.push_back(dir);
        }
    }
    wd_to_dir.clear();
    for(auto& dir : deferred_dirs) {
        watch_dir(dir);
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
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, "stop signal received", __FILE__, __LINE__));
#endif
            break;
        }
        if(FD_ISSET(inotify_fd, &fds)) {
            ssize_t len = read(inotify_fd, event_buf, EVENT_BUF_SIZE);
            last_event_time = std::chrono::steady_clock::now();
            pending = true;
            // check for new subdirectory creation and add watches dynamically
            ssize_t i = 0;
            while(i < len) {
                auto* event = reinterpret_cast<inotify_event*>(event_buf + i);
                if((event->mask & IN_CREATE) && (event->mask & IN_ISDIR) && event->len > 0) {
                    auto it = wd_to_dir.find(event->wd);
                    if(it != wd_to_dir.end()) {
                        std::string new_dir = it->second + "/" + event->name;
#ifdef ENABLE_LOGGING
                        log::debug(log::Message(__func__, std::format("new subdirectory detected => {}", new_dir), __FILE__, __LINE__));
#endif
                        watch_dir(new_dir);
                    }
                }
                i += sizeof(inotify_event) + event->len;
            }
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, "file change event received", __FILE__, __LINE__));
#endif
        }
        if(pending) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - last_event_time).count();
            if(elapsed >= DEBOUNCE_MS) {
                pending = false;
#ifdef ENABLE_LOGGING
                log::debug(log::Message(__func__, "debounce elapsed, invoking restart callback", __FILE__, __LINE__));
#endif
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
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}

void slim::file::watcher::watch_dir(std::string_view path) {
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
#endif
#ifdef ENABLE_LOGGING
    log::debug(log::Message(__func__, std::format("watching directory => {}", path), __FILE__, __LINE__));
#endif
    if(inotify_fd < 0) {
#ifdef ENABLE_LOGGING
        log::debug(log::Message(__func__, "inotify_fd not ready, deferring directory watch", __FILE__, __LINE__));
#endif
        // store for watch() to register when it initializes
        wd_to_dir[-1 - static_cast<int>(wd_to_dir.size())] = std::string(path);
#ifdef ENABLE_LOGGING
        log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
        return;
    }
    // recursively register watches for path and all subdirectories
    std::function<void(const std::string&)> register_dir = [&](const std::string& dir_path) {
        int wd = inotify_add_watch(inotify_fd, dir_path.c_str(),
            IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
        if(wd >= 0) {
            wd_to_dir[wd] = dir_path;
#ifdef ENABLE_LOGGING
            log::debug(log::Message(__func__, std::format("registered dir watch => {}", dir_path), __FILE__, __LINE__));
#endif
        } else {
            throw std::runtime_error(std::format("inotify_add_watch failed for dir => {}: {}", dir_path, strerror(errno)));
        }
        // recurse into subdirectories
        for(auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if(entry.is_directory()) {
                register_dir(entry.path().string());
            }
        }
    };
    register_dir(std::string(path));
#ifdef ENABLE_LOGGING
    log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
#endif
}
