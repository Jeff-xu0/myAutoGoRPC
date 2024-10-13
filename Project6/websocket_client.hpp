// websocket_client.hpp
#include <iostream>
#include <string>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <functional>

using websocketpp::client;
using websocketpp::connection_hdl;

typedef client<websocketpp::config::asio> ws_client;

class WebSocketClient {
public:
	WebSocketClient() {
		m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
		m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
		m_endpoint.init_asio();
		m_endpoint.set_open_handler(std::bind(&WebSocketClient::on_open, this, std::placeholders::_1));
		m_endpoint.set_close_handler(std::bind(&WebSocketClient::on_close, this, std::placeholders::_1));
		m_endpoint.set_message_handler(std::bind(&WebSocketClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
	}

	virtual void on_open(connection_hdl hdl) {
		//std::cout << "Connected to server." << std::endl;
		m_hdl = hdl;
	}

	virtual void on_close(connection_hdl hdl) {
		//std::cout << "Disconnected from server." << std::endl;
	}

	virtual void on_message(connection_hdl hdl, ws_client::message_ptr msg) {
		//std::cout << "Received message: " << msg->get_payload() << std::endl;
	}

	void connect(const std::string& uri) {
		websocketpp::lib::error_code ec;
		ws_client::connection_ptr con = m_endpoint.get_connection(uri, ec);
		if (ec) {
			std::cout << "Error during connection: " << ec.message() << std::endl;
			return;
		}

		m_endpoint.connect(con);
		m_endpoint.run();
	}

protected:
	ws_client m_endpoint;
	connection_hdl m_hdl;
};