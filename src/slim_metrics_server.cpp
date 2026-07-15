#include <future>
#include <memory>
#include <slim/common/log.h>
#include <slim/common/network/client/request/handler/metrics.h>
#include <slim/common/network/address_set.h>
#include <slim/common/network/listener.h>
#include <slim/common/network/listener/information.h>
#include <slim/metrics_server.h>
namespace slim::metrics::server {
	using namespace slim::common;
	static std::future<void> metrics_server_promise;
	slim::common::Metrics metrics;
	void reset();
}
void slim::metrics::server::start() {
	log::trace(log::Message("slim::metrics::server::start()","begins",__FILE__,__LINE__));
	//network::address::AddressSet metrics_server_address_set{9090,"127.0.0.1"};
	std::shared_ptr<network::listener::Information> metrics_server_listener_information 
		= std::make_shared<network::listener::Information>(network::listener::Information{{"127.0.0.1:9090"}, 100, 4});
	log::info("Starting metrics server on http://" +  metrics_server_listener_information->address_set.address() 
										+ ":" + std::to_string(metrics_server_listener_information->address_set.port()));
	metrics_server_promise = std::async(std::launch::async,
		slim::common::network::listener::standard,
		metrics_server_listener_information,
		std::ref(slim::common::network::client::request::handler::metrics::handle_request),
		std::ref(metrics)
	);
	log::trace(log::Message("slim::metrics::server::start()","begins",__FILE__,__LINE__));
}