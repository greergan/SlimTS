#pragma once
#include <string_view>
#include <future>
#include <slim/common/http/response.h>
namespace slim::fetch {
std::future<slim::common::http::Response> fetch_file(std::string_view uri);
void test();
} // namespace slim::fetch
