#include <arpa/inet.h>
#include <errno.h>
#include <chrono>
#include <functional>
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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <slim/common/log.h>
#include <slim/common/network/client.h>

using namespace slim::common;
#define BUFFER_SIZE 2048
void slim::common::network::client::connection::handler(Connection& _connection, std::function<slim::common::WebFile(slim::common::WebFile&, Connection&)> _request_handler) {
	log::trace(log::Message(__func__, "begins",__FILE__,__LINE__));
	std::future<bool> response;
	WebFile connection_data = WebFile();
	timeval timeout;
	fd_set read_set, write_set, except_set;
/* 	if(_connection.read_timeout > -1) {
		timeout.tv_usec = _connection.read_timeout;
	}
	timeout.tv_usec = 1000; */
	for(;;) {
		int  bytes_read = 0;
		int  total_bytes_read = 0;
		FD_ZERO(&read_set);
		FD_ZERO(&except_set);
		FD_SET(_connection.socket_handle, &read_set);
		int selected = select(_connection.socket_handle + 1, &read_set, NULL, &except_set, NULL);
		if(selected == -1 && errno == EINTR) {
			continue;
		}
		if(selected == -1) {
			break;
		}
		if(FD_ISSET(_connection.socket_handle, &read_set)) {
			read_looper:
			connection_data.data()->resize(total_bytes_read + BUFFER_SIZE);
			memset(&(connection_data.data().get()->data())[total_bytes_read], 0, connection_data.data()->size() - total_bytes_read);

			if(_connection.is_tls) {

			}
			else {
				bytes_read = read(_connection.socket_handle, &(connection_data.data().get()->data())[total_bytes_read], BUFFER_SIZE);
			}
			auto error_number = errno;
			if(bytes_read > 0) {
				log::debug(log::Message(__func__,"read bytes => " + std::to_string(bytes_read),__FILE__, __LINE__));
				total_bytes_read += bytes_read;
				goto read_looper;
			}
			else if(error_number == EAGAIN) {
				log::debug(log::Message(__func__,"errno => EAGAIN",__FILE__, __LINE__));
				goto read_looper;
			}
			else if(error_number == EWOULDBLOCK) {
				log::debug(log::Message(__func__,"errno => EWOULDBLOCK",__FILE__, __LINE__));
				goto read_looper;
			}
			else if(bytes_read == -1) {
				log::error(log::Message(__func__,"error reading from socket => errno => " + std::to_string(errno),__FILE__, __LINE__));
			}

			auto response = std::async(std::launch::async, _request_handler, std::ref(connection_data), std::ref(_connection));
			response.get();
		}
		if(FD_ISSET(_connection.socket_handle, &except_set)) {
			char c;
			oob_looper:
			auto bytes_read = recv(_connection.socket_handle, &c, 1, MSG_OOB);
			if(bytes_read == 1) {
				log::error(log::Message(__func__,"ignoring OOB data => " + c,__FILE__, __LINE__));
				goto oob_looper;
			}
		}
	}
	for(;;) {
		int  bytes_written = 0;
		int  total_bytes_written = 0;
		FD_ZERO(&write_set);
		FD_ZERO(&except_set);
		FD_SET(_connection.socket_handle, &write_set);
		int selected = select(_connection.socket_handle + 1, NULL, &write_set, NULL, NULL);
		if(selected == -1 && errno == EINTR) {
			continue;
		}
		if(selected == -1) {
			log::error(log::Message(__func__,"error writing data to client socket => errno => " + std::to_string(errno),__FILE__, __LINE__));
			break;
		}
		if(FD_ISSET(_connection.socket_handle, &write_set)) {
			auto response_data_ptr = connection_data.data().get()->data();
			write_looper:
			if(response_data_ptr) {
				bytes_written = send(_connection.socket_handle, response_data_ptr, connection_data.data().get()->size(), 0);
			}
			else {
				log::error(log::Message(__func__,"error writing data to client socket => data has become invalid",__FILE__, __LINE__));
				break;
			}
			
			if(bytes_written == -1) {
				log::error(log::Message(__func__,"error writing data to client socket => errno => " + std::to_string(errno),__FILE__, __LINE__));
				break;
			}
			else if(bytes_written == 0) {
				break;
			}
			else {
				total_bytes_written += bytes_written;
			 	if(total_bytes_written < connection_data.data().get()->size()) {
					response_data_ptr += bytes_written;
					goto write_looper;
				}
			}
		}
	}
	shutdown(_connection.socket_handle, SHUT_WR);
	close(_connection.socket_handle);
	log::trace(log::Message(__func__, "ends",__FILE__,__LINE__));
}
