#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <slim/common/http.h>
#include <slim/common/log.h>
#include <slim/common/network/client/tcp.h>
#include <slim/common/web_file.h>
slim::common::network::client::tcp::Connection::~Connection() {
	freeaddrinfo(addrinfo_pointer);
}
slim::common::network::client::tcp::Connection::Connection(slim::common::WebFile* _web_file_ptr) {
	log::trace(log::Message(__func__,"begins",__FILE__, __LINE__));
	auto socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_handle < 0) {
		log::error(log::Message(__func__,"invalid socket handle received",__FILE__, __LINE__));
    }
	else {
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
		auto gai_rc = getaddrinfo(_web_file_ptr->request().url().hostname().value().c_str(), NULL, &hints, &addrinfo_pointer);
		if(gai_rc != 0) {
			std::string error_string = gai_strerror(gai_rc);
			log::error(log::Message(__func__,error_string,__FILE__, __LINE__));
		}
		else {
            memcpy(&server_address, addrinfo_pointer->ai_addr, sizeof(struct sockaddr_in));
    		memset(server_address.sin_zero, 0, sizeof(server_address.sin_zero));
			if(connect(socket_handle, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
				std::string error_string = "connection to server failed";
				log::error(log::Message(__func__,error_string,__FILE__, __LINE__));
			}
			else {
				int flags = fcntl(socket_handle, F_GETFL, 0);
				if(flags == -1) {
					std::string error_string = "error getting socket flags";
					log::error(log::Message(__func__,error_string,__FILE__, __LINE__));
				}
				else {
					bool is_tls = _web_file_ptr->request().url().protocol() == "https" ? true : false;
					//flags = flags | O_NONBLOCK;
					if(fcntl(socket_handle, F_SETFL, flags) == -1) {
						std::string error_string = "error setting socket flags";
						log::error(log::Message(__func__,error_string,__FILE__, __LINE__));
					}
					else {
						log::debug(log::Message(__func__,"connected to => " + _web_file_ptr->request().url().hostname().value(),__FILE__, __LINE__));
						auto request_string = _web_file_ptr->request().to_string();

						int bytes_read = 0;
						long total_bytes_read = 0;
						int index = 0;
						SSL* ssl_socket_handle = nullptr;
						if(is_tls) {
							const SSL_METHOD* method =SSLv23_client_method();
							SSL_CTX* ctx = SSL_CTX_new(method);
							if(ctx == nullptr) {
								log::error(log::Message(__func__,"unable to create SSL context",__FILE__, __LINE__));
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
								log::error(log::Message(__func__,"unable to establish SSL connection",__FILE__, __LINE__));
								return;
							}
							else {
								log::debug(log::Message(__func__,"SSL connection established to => " 
									+ _web_file_ptr->request().url().host().value() + " using cipher => " + SSL_get_cipher(ssl_socket_handle),__FILE__, __LINE__));
								int ret = SSL_write(ssl_socket_handle, request_string.c_str(), strlen(request_string.c_str()));
							}
						}
						else {
							log::debug(log::Message(__func__,"not using TLS for => " + _web_file_ptr->request().url().hostname().value(),__FILE__, __LINE__));
							send(socket_handle, request_string.c_str(), request_string.size(), 0);
						}
						looper:
						_web_file_ptr->data()->resize(total_bytes_read + BUFFER_SIZE);
						memset(&(_web_file_ptr->data().get()->data())[total_bytes_read], 0, _web_file_ptr->data()->size() - total_bytes_read);
						if(is_tls) {
							bytes_read = SSL_read(ssl_socket_handle, &(_web_file_ptr->data().get()->data())[total_bytes_read], BUFFER_SIZE);
						}
						else {
							bytes_read = read(socket_handle, &(_web_file_ptr->data().get()->data())[total_bytes_read], BUFFER_SIZE);
						}
						auto error_number = errno;
						if(bytes_read > 0) {
							log::debug(log::Message(__func__,"read bytes => " + std::to_string(bytes_read),__FILE__, __LINE__));
							total_bytes_read += bytes_read;
							goto looper;
						}
						else if(error_number == EAGAIN) {
							log::debug(log::Message(__func__,"errno => EAGAIN",__FILE__, __LINE__));
							goto looper;
						}
						else if(error_number == EWOULDBLOCK) {
							log::debug(log::Message(__func__,"errno => EWOULDBLOCK",__FILE__, __LINE__));
							goto looper;
						}
						else if(bytes_read == -1) {
							log::error(log::Message(__func__,"error reading from => " 
								+ _web_file_ptr->request().url().hostname().value() + " errno => " + std::to_string(errno),__FILE__, __LINE__));
						}
						log::debug(log::Message(__func__,"read from => " + _web_file_ptr->request().url().host().value(),__FILE__, __LINE__));
						close(socket_handle);
					}
				}
			}
		}
	}
	log::trace(log::Message(__func__,"ends",__FILE__, __LINE__));
}

/* switch (err) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // Retry the operation later
            break;
        case SSL_ERROR_SSL:
        case SSL_ERROR_SYSCALL: {
            unsigned long sslerr;
            while ((sslerr = ERR_get_error())) {
                // Get the human-readable error string
                const char *errmsg = ERR_error_string(sslerr, NULL);
                // Process or log the error string
                printf("SSL error: %s\n", errmsg);
            }
            break;
        }
        case SSL_ERROR_ZERO_RETURN:
            // Connection closed cleanly
            break;
        default:
            // Handle other errors
            break;
    }	
 */

/* #include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <arpa/inet.h>

SSL_CTX* InitCTX(void) {
    SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    // Use TLS_client_method for client connections
    method = (SSL_METHOD *)TLS_client_method();
    ctx = SSL_CTX_new(method);

    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

int main() {
    SSL_CTX *ctx;
    SSL *ssl;
    int sock;
    struct sockaddr_in server;

    ctx = InitCTX();

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(443);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    // Connect to server
    connect(sock, (struct sockaddr *)&server, sizeof(server));

    // Create SSL object
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);

    // Perform handshake
    if (SSL_connect(ssl) == 1) {
        printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
    } else {
        ERR_print_errors_fp(stderr);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    return 0;
}    */