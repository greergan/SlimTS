#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <slim/common/http/request.h>
#include <slim/common/log.h>
#include <slim/common/network/client/tcp.h>
#include <slim/common/web_file.h>
slim::common::network::client::tcp::Connection::Connection(slim::common::WebFile* web_file_pointer) {
	log::trace(log::Message("slim::common::network::client::tcp::Connection::Connection()","begins url => " + web_file_pointer->request().address_set().address,__FILE__, __LINE__));
	bool is_tls = web_file_pointer->request().protocol() == "https://" ? true : false;
	socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_handle < 0) {
		std::string error_string = "Socket creation error";
		log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()",error_string,__FILE__, __LINE__));
    }
	else {
		memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
		auto result = getaddrinfo(web_file_pointer->request().address_set().address.c_str(), std::to_string(web_file_pointer->request().address_set().port).c_str(), &hints, &addrinfo_pointer);
		if(result != 0) {
			std::string error_string = gai_strerror(result);
			log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()",error_string,__FILE__, __LINE__));
		}
		else {
            memcpy(&server_address, addrinfo_pointer->ai_addr, sizeof(struct sockaddr_in));
            freeaddrinfo(addrinfo_pointer);
    		memset(server_address.sin_zero, 0, sizeof(server_address.sin_zero));
			if(connect(socket_handle, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
				std::string error_string = "connection to server failed";
				log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()",error_string,__FILE__, __LINE__));
			}
			else {
				int flags = fcntl(socket_handle, F_GETFL, 0);
				if(flags == -1) {
					std::string error_string = "error getting socket flags";
					log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()",error_string,__FILE__, __LINE__));
				}
				else {
					//flags = flags | O_NONBLOCK;
					if(fcntl(socket_handle, F_SETFL, flags) == -1) {
						std::string error_string = "error setting socket flags";
						log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()",error_string,__FILE__, __LINE__));
					}
					else {
						log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","connected to => " + web_file_pointer->request().address_set().address,__FILE__, __LINE__));
						auto request_string = web_file_pointer->request().to_string();

						//log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","sent request to => " + web_file_pointer->request().address_set().address,__FILE__, __LINE__));
						int bytes_read = 0;
						long total_bytes_read = 0;
						int index = 0;
						SSL* ssl_socket_handle = nullptr;
						if(is_tls) {
							const SSL_METHOD* method =SSLv23_client_method();
							SSL_CTX* ctx = SSL_CTX_new(method);
							if(ctx == nullptr) {
								log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()","unable to create SSL context",__FILE__, __LINE__));
								return;
							}
							ssl_socket_handle = SSL_new(ctx);
							SSL_set_fd(ssl_socket_handle, socket_handle);
							SSL_set_connect_state(ssl_socket_handle);
							if(SSL_connect(ssl_socket_handle) <= 0) {
								int ssl_error_number = SSL_get_error(ssl_socket_handle, -1);
        						SSL_free(ssl_socket_handle);
        						SSL_CTX_free(ctx);
								close(socket_handle);
								log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()","unable to establish SSL connection",__FILE__, __LINE__));
								return;
							}
							else {
								log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","SSL connection established to => " 
									+ web_file_pointer->request().address_set().address + " using cipher => " + SSL_get_cipher(ssl_socket_handle),__FILE__, __LINE__));
								int ret = SSL_write(ssl_socket_handle, request_string.c_str(), strlen(request_string.c_str()));
							}
						}
						else {
							log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","not using TLS for => " + web_file_pointer->request().address_set().address,__FILE__, __LINE__));
							send(socket_handle, request_string.c_str(), request_string.size(), 0);
						}
						looper:
						web_file_pointer->data()->resize(total_bytes_read + BUFFER_SIZE);
						memset(&(web_file_pointer->data().get()->data())[total_bytes_read], 0, web_file_pointer->data()->size() - total_bytes_read);
						if(is_tls) {
							bytes_read = SSL_read(ssl_socket_handle, &(web_file_pointer->data().get()->data())[total_bytes_read], BUFFER_SIZE);
						}
						else {
							bytes_read = read(socket_handle, &(web_file_pointer->data().get()->data())[total_bytes_read], BUFFER_SIZE);
						}
						auto error_number = errno;
						if(bytes_read > 0) {
							log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","read bytes => " + std::to_string(bytes_read),__FILE__, __LINE__));
							total_bytes_read += bytes_read;
							goto looper;
						}
						else if(error_number == EAGAIN) {
							log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","errno => EAGAIN",__FILE__, __LINE__));
							goto looper;
						}
						else if(error_number == EWOULDBLOCK) {
							log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","errno => EWOULDBLOCK",__FILE__, __LINE__));
							goto looper;
						}
						else if(bytes_read == -1) {
							log::error(log::Message("slim::common::network::client::tcp::Connection::Connection()","error reading from => " 
								+ web_file_pointer->request().address_set().address + " errno => " + std::to_string(errno),__FILE__, __LINE__));
						}
						log::debug(log::Message("slim::common::network::client::tcp::Connection::Connection()","read from => " + web_file_pointer->request().address_set().address,__FILE__, __LINE__));
						close(socket_handle);
					}
				}
			}
		}
	}
	log::trace(log::Message("slim::common::network::client::tcp::Connection::Connection()","ends url => " + web_file_pointer->request().address_set().address,__FILE__, __LINE__));
}