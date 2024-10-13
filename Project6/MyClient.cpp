#include "MyClient.h"


void MyClient::on_open(connection_hdl hdl) {
	WebSocketClient::on_open(hdl); // ���û���ķ���
	std::cout << "MyClient: Connected to server." << std::endl;

	// ������Ϣ��������
	std::string message = "Hello from MyClient!";
	m_endpoint.send(hdl, message, websocketpp::frame::opcode::text);
}

void MyClient::on_close(connection_hdl hdl) {
	WebSocketClient::on_close(hdl); // ���û���ķ���
	std::cout << "MyClient: Disconnected from server." << std::endl;
}

void MyClient::on_message(connection_hdl hdl, ws_client::message_ptr msg) {
	WebSocketClient::on_message(hdl, msg); // ���û���ķ���
	std::cout << "my ws class receive message: " << msg->get_payload() << std::endl;
}

void MyClient::send_message(const std::string& message)
{
	m_endpoint.send(m_hdl, message, websocketpp::frame::opcode::text);
}
