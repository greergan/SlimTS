#include <cstring>
#include <format>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <regex>
#include <slim/common/exception.h>
#include <slim/common/fetch.h>
#include <slim/common/http.h>
#include <slim/common/log.h>
#include <slim/common/network/client/tcp.h>
#include <slim/common/validators.h>
#include <slim/common/web_file.h>
namespace slim::common::fetch {
    using namespace slim;
    using namespace slim::common;
}
std::unique_ptr<std::string> slim::common::fetch::string(const std::filesystem::path& _path) {
    log::trace(log::Message(__func__,"sets => " + _path.string(),__FILE__, __LINE__));
    return std::move(string(_path.string()));
}
std::unique_ptr<std::string> slim::common::fetch::string(const std::string& _string) {
    log::trace(log::Message(__func__,"begins => " + _string,__FILE__, __LINE__));
    auto new_string_ptr = std::make_unique<std::string>(stream(_string).get()->str());
    log::debug(log::Message(__func__,_string + " file size => " + std::to_string(new_string_ptr.get()->size()),__FILE__, __LINE__));
    log::trace(log::Message(__func__,"ends => " + _string,__FILE__, __LINE__));
    return std::move(new_string_ptr);
}
std::unique_ptr<std::stringstream> slim::common::fetch::stream(const std::string& _string) {
    log::trace(log::Message(__func__,"begins => " + _string,__FILE__, __LINE__));
    auto file_contents_stringstream = std::make_unique<std::stringstream>();
    if(slim::common::validators::is_file_url(_string)) {
        const std::string file_name_string = _string.substr(7);
        log::debug(log::Message(__func__,"fetching => " + file_name_string,__FILE__, __LINE__));
        std::ifstream file_stringstream(file_name_string, std::ios::in);
        if(file_stringstream.is_open()) {
            *file_contents_stringstream << file_stringstream.rdbuf();
            file_stringstream.close();
        }
        else {
            log::error(log::Message(__func__,(std::string("file access errno => ") + std::string(strerror(errno))).c_str(),__FILE__, __LINE__));
            std::string error_message(strerror(errno));
            error_message += " opening file";
            throw slim::common::SlimFileException(__func__, error_message.c_str(), _string.c_str(), errno);
        }
    }
    log::trace(log::Message(__func__,"ends => " + _string,__FILE__, __LINE__));
    return std::move(file_contents_stringstream);
}
void slim::common::fetch::web_file(slim::common::WebFile* _web_file_ptr) {
    log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
    if(_web_file_ptr->request().url().protocol() == "file") {
        log::debug(log::Message(__func__,"fetching disk file => " + _web_file_ptr->request().url().url().value_or("not set"),__FILE__, __LINE__));
        std::ifstream input_file_stream(_web_file_ptr->request().url().pathname().value(), std::ios::in|std::ios::binary);
        if(input_file_stream.is_open()) {
            log::debug(log::Message(__func__,"fetching disk file => " + _web_file_ptr->request().url().url().value_or("not set"),__FILE__, __LINE__));
            input_file_stream.seekg(0, std::ios::end);
            long long bytes_to_read = input_file_stream.tellg();
            input_file_stream.seekg(0, std::ios::beg);
            if(bytes_to_read > 0) {
                _web_file_ptr->data().get()->resize(bytes_to_read + 1, '\0');
                log::debug(log::Message(__func__,"reading disk file => " + _web_file_ptr->request().url().url().value_or("not set"),__FILE__, __LINE__));
                input_file_stream.read(reinterpret_cast<char*>(_web_file_ptr->data().get()->data()), bytes_to_read);
                if(input_file_stream.gcount() != bytes_to_read) {
                    _web_file_ptr->error("slim::common::fetch::web_file(_web_file_ptr)|unknown file access error while reading => " + _web_file_ptr->request().url().url().value_or("not set"));
                }
            }
            else {
                log::debug(log::Message(__func__,"not reading empty file => " + _web_file_ptr->request().url().url().value_or("not set"),__FILE__, __LINE__));
            }
            input_file_stream.close();
        }
        else {
            log::error(log::Message(__func__,"file access error => " + std::string(strerror(errno)),__FILE__, __LINE__));
            std::string error_string = "slim::common::fetch::web_file(_web_file_ptr)|file access error => " + std::string(strerror(errno));
            std::string error_message(strerror(errno));
            error_string += " " + error_message + " opening file";
            _web_file_ptr->error_number((int)errno);
            _web_file_ptr->error(error_string);
        }
    }
    else {
        log::debug(log::Message(__func__,"fetching file from internet => " + _web_file_ptr->request().url().url().value_or("not set"),__FILE__, __LINE__));
        auto connection = slim::common::network::client::tcp::Connection(_web_file_ptr);
    }
    log::trace(log::Message(__func__,"ends => " + _web_file_ptr->request().url().url().value_or("not set"),__FILE__, __LINE__));
}