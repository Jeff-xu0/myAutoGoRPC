// MyClient.h
#ifndef MYCLIENT_H
#define MYCLIENT_H

#include "websocket_client.hpp" // 包含基类的头文件

class MyClient : public WebSocketClient {
public:
	virtual void on_open(connection_hdl hdl) override;

	virtual void on_close(connection_hdl hdl) override;

	virtual void on_message(connection_hdl hdl, ws_client::message_ptr msg) override;

	void send_message(const std::string& message);
};

#endif // MYCLIENT_H
