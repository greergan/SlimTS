#include <arpa/inet.h>
#include <errno.h>
#include <future>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <slim/common/network/client/request/handler/metrics.h>
#include <slim/common/http/response.h>
#include <slim/common/http/request.h>
#include <slim/common/metrics.h>
#include <slim/common/metrics/counter.h>

#include <iostream>

static std::string slim::common::network::client::request::handler::metrics::get_metrics(slim::common::Metrics& metrics) {
	metrics.counters["metrics_requests"].inc();
	std::stringstream metrics_stream;
	std::ifstream filestat("/proc/self/stat");
	std::string line;
	getline(filestat, line);
	if(line.length() > 0) {
		std::stringstream line_stream(line);
		std::string value;
		int count = 1;
		while(getline(line_stream, value, ' ')) {
			if(count == 20) {
				metrics_stream << "# tendril_process_threads_running number of threads running in a specific process" << std::endl;
				metrics_stream << "# tendril_process_threads_running number summary" << std::endl;
				metrics_stream << "tendril_process_threads_running" << metrics.pid_string << " " << value << std::endl;
			}
			count++;
		}
	}
	for(auto& [key, metric] : metrics.counters) {
		metrics_stream << "# " << metric.help << std::endl;
		metrics_stream << "# " << metric.type << std::endl;
		metrics_stream << metric.format << " " << metric.count << std::endl;
	}
	return metrics_stream.str();
}
std::string slim::common::network::client::request::handler::metrics::handle_request(char* request_pointer,
		const slim::common::network::client::connection::Information& client_connection_information, slim::common::Metrics& metrics) {
	std::cout << "tendril::client::request::handler::metrics::handle_request\n";
	slim::common::http::Request request(request_pointer);
	slim::common::http::Response response;
	if(request.version() == "HTTP/1.1") {
		response.version(request.version());
		if(request.method() == "GET") {
			if(request.path() == "/") {
				response.body("<html><div>Tendril metrics</div><a href=\"/metrics\">metrics</a></html>");
				response.headers().set("Content-type", "text/html");
				response.headers().set("Content-length", std::to_string(response.body().length()));
				response.response_code(200);
			}
			else if(request.path() == "/metrics") {
				auto result = std::async(get_metrics, std::ref(metrics));
				response.body(result.get());
				response.headers().set("Content-type", "text/plain");
				response.headers().set("Content-length", std::to_string(response.body().length() + 1));
				response.response_code(200);
			}
			else {
				response.response_code(404);
			}
		}
		else {
			response.response_code(501);
		}
	}
	else {
		response.version("HTTP/1.1");
		response.response_code(505);
	}
	std::stringstream response_stream;
	response_stream << response.version() << " " << response.response_code() << " " << response.response_code_text() << "\r\n";
	for(auto& [key, value] : response.headers().get()) {
		response_stream << key << ": " << value << "\r\n";
	}
	response_stream << "\r\n\r\n" << response.body() << "\r\n";
	std::cout << "tendril::client::request::handler::metrics::handle_request done\n";
	return response_stream.str();
}