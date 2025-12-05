#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <regex>
#include <slim/common/exception.h>
#include <slim/common/fetch.h>
#include <slim/common/http/response.h>
#include <slim/common/http/request.h>
#include <slim/common/log.h>
#include <slim/common/network/client/tcp.h>
#include <slim/common/web_file.h>
namespace slim::common::fetch {
    using namespace slim;
    using namespace slim::common;
}
std::unique_ptr<std::string> slim::common::fetch::string(std::string& file_name_string) {
    log::trace(log::Message("slim::common::fetch::string()",std::string("begins, file name => " + file_name_string).c_str(),__FILE__, __LINE__));
    auto file_content_pointer = stream(file_name_string);
    auto return_string_pointer = std::make_unique<std::string>(file_content_pointer.get()->str());
    log::trace(log::Message("slim::common::fetch::string()",std::string("ends, file name => " + file_name_string).c_str(),__FILE__, __LINE__));
    return std::move(return_string_pointer);
}
std::unique_ptr<std::stringstream> slim::common::fetch::stream(std::string& file_name_string) {
    log::trace(log::Message("slim::common::fetch::stream()",std::string("begins" + file_name_string).c_str(),__FILE__, __LINE__));
    std::string input_file_name = file_name_string;
    if(input_file_name.starts_with("file:///") || input_file_name.starts_with("file://")) {
		input_file_name = input_file_name.substr(7);
	}
    log::debug(log::Message("slim::common::fetch::stream()",std::string("file_name => " + input_file_name).c_str(),__FILE__, __LINE__));
    std::ifstream input_file_stream(input_file_name, std::ios::in);
    auto file_contents_stringstream = std::make_unique<std::stringstream>();
    if(input_file_stream.is_open()) {
        *file_contents_stringstream << input_file_stream.rdbuf();
        input_file_stream.close();
    }
    else {
        log::error(log::Message("slim::common::fetch::stream()",(std::string("file access errno => ") + std::string(strerror(errno))).c_str(),__FILE__, __LINE__));
        std::string error_message(strerror(errno));
        error_message += " opening file";
        throw slim::common::SlimFileException("slim::common::fetch::stream()", error_message.c_str(), file_name_string.c_str(), errno);
    }
    log::trace(log::Message("slim::common::fetch::stream()",std::string("ends" + file_name_string).c_str(),__FILE__, __LINE__));
    return std::move(file_contents_stringstream);
}
void slim::common::fetch::web_file(slim::common::WebFile* web_file_pointer) {
    log::trace(log::Message("slim::common::fetch::web_file()","begins url => " + web_file_pointer->request().url(),__FILE__, __LINE__));
    if(web_file_pointer->request().protocol() == "file://") {
        log::debug(log::Message("slim::common::fetch::web_file()","fetching disk file => " + web_file_pointer->request().url(),__FILE__, __LINE__));
        std::ifstream input_file_stream(web_file_pointer->request().url().substr(7), std::ios::in|std::ios::binary);
        if(input_file_stream.is_open()) {
            log::debug(log::Message("slim::common::fetch::web_file()","fetching disk file => " + web_file_pointer->request().url(),__FILE__, __LINE__));
            input_file_stream.seekg(0, std::ios::end);
            long long bytes_to_read = input_file_stream.tellg();
            input_file_stream.seekg(0, std::ios::beg);
            web_file_pointer->data().get()->resize(bytes_to_read);
            log::debug(log::Message("slim::common::fetch::web_file()","reading disk file => " + web_file_pointer->request().url(),__FILE__, __LINE__));
            input_file_stream.read(reinterpret_cast<char*>(web_file_pointer->data().get()->data()), bytes_to_read);
            if(input_file_stream.gcount() != bytes_to_read) {
                web_file_pointer->error("slim::common::fetch::web_file()|unknown file access error while reading => " + web_file_pointer->request().url());
            }
            input_file_stream.close();
        }
        else {
            log::error(log::Message("slim::common::fetch::web_file()","file access error => " + std::string(strerror(errno)),__FILE__, __LINE__));
            std::string error_string = "slim::common::fetch::web_file()|file access error => " + std::string(strerror(errno));
            std::string error_message(strerror(errno));
            error_string += " " + error_message + " opening file";
            web_file_pointer->error_number((int)errno);
            web_file_pointer->error(error_string);
        }
    }
    else {
        log::debug(log::Message("slim::common::fetch::web_file()","fetching file from internet => " + web_file_pointer->request().url(),__FILE__, __LINE__));
        log::debug(log::Message("slim::common::fetch::web_file()","fetching file from internet => " + web_file_pointer->request().address_set().address ,__FILE__, __LINE__));
        log::debug(log::Message("slim::common::fetch::web_file()","fetching file from internet => " + std::to_string(web_file_pointer->request().address_set().port) ,__FILE__, __LINE__));
        auto connection = slim::common::network::client::tcp::Connection(web_file_pointer);
    }
    log::trace(log::Message("slim::common::fetch::web_file()","ends url => " + web_file_pointer->request().url(),__FILE__, __LINE__));
}