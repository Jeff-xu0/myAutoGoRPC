#include "MyClient.h"


void MyClient::on_open(connection_hdl hdl) {
	WebSocketClient::on_open(hdl); // 调用基类的方法
	std::cout << "MyClient: Connected to server." << std::endl;

	// 发送消息到服务器
	std::string message = "Hello from MyClient!";
	m_endpoint.send(hdl, message, websocketpp::frame::opcode::text);
}

void MyClient::on_close(connection_hdl hdl) {
	WebSocketClient::on_close(hdl); // 调用基类的方法
	std::cout << "MyClient: Disconnected from server." << std::endl;
}

void MyClient::on_message(connection_hdl hdl, ws_client::message_ptr msg) {
	WebSocketClient::on_message(hdl, msg); // 调用基类的方法
	std::cout << "my ws class receive message: " << msg->get_payload() << std::endl;
}

void MyClient::send_message(const std::string& message)
{
	m_endpoint.send(m_hdl, message, websocketpp::frame::opcode::text);
}
