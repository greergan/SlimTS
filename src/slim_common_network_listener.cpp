#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <functional>
#include <future>
#include <memory>
#include <vector>
#include <slim/common/log.h>
#include <slim/common/network/client/connection/handler/common.h>
#include <slim/common/network/client/connection/information.h>
#include <slim/common/metrics.h>
#include <slim/common/network/listener.h>
#include <slim/common/network/listener/information.h>
namespace slim::common::network::listener {
	using namespace slim::common;
	static std::vector<std::future<void>> client_connection_futures;
}
void slim::common::network::listener::standard(std::shared_ptr<network::listener::Information> listener_information,
									client_request_handler_function client_request_handler, slim::common::Metrics& metrics) {
	log::trace(log::Message("slim::common::network::listener::standard()", "begins",__FILE__, __LINE__));
	struct sockaddr_in listener_socket_address;
	int server_socket;
	int socket_options = 1;
	if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
        exit(1);
    }
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_options, sizeof(socket_options)) == -1) {
        perror("setsockopt");
        close(server_socket);
        exit(1);
    }
    memset(&listener_socket_address, 0, sizeof(listener_socket_address));
	log::info(std::string("Binding server to tcp://" + listener_information->address_set.address() + ":" + std::to_string(listener_information->address_set.port())));
	listener_socket_address.sin_addr.s_addr = inet_addr(listener_information->address_set.address().c_str());
    listener_socket_address.sin_port = htons(listener_information->address_set.port());
    listener_socket_address.sin_family = AF_INET;
    if(bind(server_socket, (struct sockaddr *) &listener_socket_address, sizeof(listener_socket_address)) == -1) {
        perror("bind");
        close(server_socket);
        exit(1);
    }
    //printf("slim::network::server::listener:: accepting connections on port %d\n", listening_port);
    auto result = listen(server_socket, listener_information->back_pressure);
	if(result == -1) {
		perror("listen");
		exit(1);
	}
	for(;;) {
  		struct sockaddr_in client_address;
  		socklen_t address_length = sizeof(client_address);
		memset(&client_address, 0, address_length);
		int client_connection = accept(server_socket, (struct sockaddr*) &client_address, &address_length);
		if(client_connection == -1) {
			perror("accept");
			exit(1);
		}
		// Set nonblocking mode on the socket.
		int flags = fcntl(client_connection, F_GETFL, 0);
		if (flags == -1) {
			perror("fcntl");
			exit(1);
		}
		if (fcntl(client_connection, F_SETFL, flags | O_NONBLOCK) == -1) {
			perror("fcntl");
			exit(1);
		}
		metrics.counters["tcp_connections"].inc();
		slim::common::network::client::connection::Information client_connection_information;
		client_connection_information.read_timeout = listener_information->read_time_out;
		client_connection_information.client_address = client_address;
		client_connection_information.socket_handle = client_connection;
		//client_connection_futures.emplace_back(
		auto t = std::async(std::launch::async,
				slim::common::network::client::connection::handler::common::handle_client_connection,
				std::ref(client_connection_information),
				client_request_handler,
				std::ref(metrics)
			);
		//);
						const auto is_future_ready = [&t]()->bool {
					return t.valid() ? t.wait_for(std::chrono::seconds(0)) == std::future_status::ready : false;
				};
				//while(!is_future_ready())
				//t.get();
    }
	log::trace(log::Message("slim::common::network::listener::standard()", "ends",__FILE__, __LINE__));
	return;
}