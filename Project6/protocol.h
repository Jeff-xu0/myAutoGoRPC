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

// 定义 Point 结构体
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

// GenericRequest 类
class GenericRequest {
public:
	std::string jsonrpc;
	std::string method;				// 请求方法
	Params params;					// 请求参数
	int id;							// 请求 ID


	GenericRequest();
	GenericRequest(const std::string& operation, const Params& operands, int id);

	Json::Value to_json() const;
};

// GenericResponse 类
class GenericResponse {
public:
	std::string result; // 响应结果
	std::string error; // 错误信息
	int id;           // 请求 ID

	GenericResponse();

	bool from_json(const Json::Value& json);
};

// =================================== Protocol =========================================

// Protocol 类
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
