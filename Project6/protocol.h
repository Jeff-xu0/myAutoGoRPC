#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <iostream>
#include <string>
#include <variant>
#include <map>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h> 
#include "json/json.h"
#include <any>
#include <fstream>
#include <sys/stat.h>


// =================================== Utils =========================================
std::string wstringToUtf8(const std::wstring& wstr);
Json::Value convert_any_to_json(const std::any& operand);
bool string2json(const std::string& jsonString, Json::Value& jsonValue);

// =================================== Structs =========================================

// ���� Point �ṹ��
struct Point {
	int Left;
	int Top;
	int Right;
	int Bottom;
};

struct Params
{
	std::vector<std::any> parameter;
	int req_id;
};


// =================================== Request and Response =========================================

// GenericRequest ��
class GenericRequest {
public:
	std::string jsonrpc;
	std::string method;				// ���󷽷�
	Params params;					// �������
	int id;							// ���� ID


	GenericRequest();
	GenericRequest(const std::string& operation, const Params& operands, int id);

	Json::Value to_json() const;
};

// GenericResponse ��
class GenericResponse {
public:
	std::string result; // ��Ӧ���
	std::string error; // ������Ϣ
	int id;           // ���� ID

	GenericResponse();

	bool from_json(const Json::Value& json);
};

// =================================== Protocol =========================================

// Protocol ��
class Protocol {
public:
	Protocol(const std::string& serverIp, int serverPort);
	~Protocol();

	bool Connect();
	bool SendJson(const Json::Value& j);
	bool ReceiveResponse2String(std::string& response);
	bool ReceiveResponse2File(const std::string& fileName);
	bool ReceiveResponseAndSaveAsPng(const std::string& fileName);
	bool CallWithGenericRequest(const GenericRequest& request);
	void Close();

private:
	std::string serverIp;
	int serverPort;
	SOCKET clientSocket;
};

#endif // PROTOCOL_H
