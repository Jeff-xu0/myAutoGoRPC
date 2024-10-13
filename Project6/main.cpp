#include "protocol.h"
#include <iostream>
#include "MyClient.h"
#include <thread>
#include <windows.h>

#define serverIp    "192.168.68.77"    // �޸�Ϊ��ķ�������ַ
#define serverPort 18080         // �޸�Ϊ��ķ������˿�
#define wsPort 18081         // �޸�Ϊ��ķ������˿�
#define req_id     1
#define BREAK_MSG  "ws://break"



void startClient(MyClient* client,const std::string& wsUri) {
	client->connect(wsUri);
}

// �����ֽ����ݵ� PNG �ļ�
void save_png(const std::string& filename, const std::vector<uint8_t>& binary_data) {
	std::ofstream file(filename, std::ios::binary);
	if (file.is_open()) {
		file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
		file.close();
		std::cout << "PNG file saved as " << filename << std::endl;
	}
	else {
		std::cerr << "Error opening file for writing: " << filename << std::endl;
	}
}


int main() {

	SetConsoleOutputCP(CP_UTF8);
	MyClient client;
	std::string  wsUri = "ws://" + static_cast<std::string>(serverIp) + ":" + std::to_string(wsPort) + "/ws/" + std::to_string(req_id);

	std::thread clientThread1(startClient, &client, wsUri);
    clientThread1.detach();
    Sleep(1000);

	// ���� Protocol ����
	Protocol protocol(serverIp, serverPort);

	// ���ӷ�����
	if (!protocol.Connect()) {
		std::cout<<"Failed to connect to server"<<std::endl;
		return -1;
	}

	std::cout << "Connected to server" << std::endl;


	std::string click = wstringToUtf8(L"		1. �����Ļ			");
	std::string open2predit = wstringToUtf8(L"		2. �������������			");
	std::string exit = wstringToUtf8(L"		3. �˳�				");
	std::string order = wstringToUtf8(L"������ָ�");
	std::string orderWrong = wstringToUtf8(L"ָ�����");
	while (1)
	{
		
		std::cout<< "===============================" << std::endl;
		std::cout << click << std::endl;
		std::cout << open2predit << std::endl;
		std::cout << exit << std::endl;
		std::cout<< "===============================" << std::endl;
		std::cout << order;
		// ���հ�����Ϣ
		char key;
		std::cin >> key; 

		if (key == '1') {
			
			GenericRequest request(
				"Server.ClickScreen",
				{ {}, req_id },
				req_id);
			
			protocol.CallWithGenericRequest(request);

			std::string response;
			if (protocol.ReceiveResponse2String(response)) {
				std::cout << "Response: %s" << response << std::endl;
			}
		}
		else if(key=='2')
		{
			
			GenericRequest request(
				"Server.RunPredict",
				{ {}, req_id },
				req_id);

			protocol.CallWithGenericRequest(request);

			protocol.ReceiveResponseAndSaveAsPng("result.png");
		}
		else if (key=='3')
		{
			break;
		}
		else
		{
            std::cout << orderWrong << std::endl;
		}
	}

	client.send_message(static_cast<std::string>(BREAK_MSG));
	// �ر�����
	protocol.Close();
	
	return 0;
	
}


