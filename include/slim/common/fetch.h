#ifndef __SLIM__COMMON__FETCH__H
#define __SLIM__COMMON__FETCH__H
#include <memory>
#include <sstream>
#include <vector>
#include <slim/common/http/response.h>
#include <slim/common/http/request.h>
#include <slim/common/web_file.h>
namespace slim::common::fetch {
    std::unique_ptr<std::string> string(std::string& file_name_string);
    std::unique_ptr<std::stringstream> stream(std::string& file_name_string);
    void web_file(slim::common::WebFile* web_file_pointer);
}
#endif