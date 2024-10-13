#include "protocol.h"

#include <stdexcept>
#include <openssl/bio.h>
#include <openssl/evp.h>

// =================================== GenericRequest =============================================

GenericRequest::GenericRequest() : id(0) {}
GenericRequest::GenericRequest(
	const std::string& operation, 
	const Params& operands, 
	int id) :
	method(operation), params(operands), id(id) {}


// 实现 to_json 方法
Json::Value GenericRequest::to_json() const {
	Json::Value json_request;

	// 设置 method 和 id
	json_request["method"] = method;
	json_request["id"] = id;
	json_request["jsonrpc"] = "2.0";

	// 构建 params 数组
	Json::Value json_params(Json::arrayValue);

	// 创建包含 "parameter" 数组的对象
	Json::Value json_param_object; // obj
	Json::Value json_p_array(Json::arrayValue); // arr

	// 遍历 params.p 中的 std::any 并转换为 JSON
	for (const auto& param : params.parameter) {
		json_p_array.append(convert_any_to_json(param));
	}

	// 将 "parameter" 数组加入对象
	json_param_object["parameter"] = json_p_array;
	json_param_object["id"] = params.req_id;

	// 将对象加入 params 数组
	json_params.append(json_param_object);

	// 将 params 数组加入请求
	json_request["params"] = json_params;

	return json_request;
}

// =================================== GenericResponse =============================================

GenericResponse::GenericResponse() : id(0) {}


// GenericResponse 类的 from_json 方法
bool GenericResponse::from_json(const Json::Value& json) {
	try {
		// 检查 json 是否为有效的对象
		if (!json.isObject()) {
			std::cerr << "Error: The provided JSON is not an object." << std::endl;
			return false;
		}

		// 检查 "result" 字段是否存在并处理
		if (json.isMember("result")) {
			result = json["result"].asString();
		}
		else
		{
			result = "";
		}

		// 处理 "error" 字段
		if (json.isMember("error") && json["error"].isString()) {
			error = json["error"].asString();
		}
		else {
			error = "";
		}

		// 处理 "id" 字段
		if (json.isMember("id") && json["id"].isInt()) {
			id = json["id"].asInt();
		}
		else {
			id = -1;  // 如果 ID 不存在，设置为默认值 -1
		}

		return true;
	}
	catch (const Json::LogicError& e) {
		std::cerr << "JSON parsing error: " << e.what() << std::endl;
		return false;
	}
}


// =================================== Protocol =============================================

// Protocol 构造函数：初始化服务器 IP 和端口
Protocol::Protocol(const std::string& serverIp, int serverPort)
	: serverIp(serverIp), serverPort(serverPort), clientSocket(INVALID_SOCKET) {}

// Protocol 析构函数：关闭连接
Protocol::~Protocol() {
	Close();
}

// 连接服务器
bool Protocol::Connect() {
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed: " << result << std::endl;
		return false;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(serverPort);

	result = inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr);
	if (result <= 0) {
		std::cerr << "Invalid server IP address format" << std::endl;
		Close();
		return false;
	}

	result = connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
	if (result == SOCKET_ERROR) {
		std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
		Close();
		return false;
	}

	return true;
}

// 发送 JSON 数据
bool Protocol::SendJson(const Json::Value& j) {
	Json::StreamWriterBuilder writer;
	std::string message = Json::writeString(writer, j);
	int result = send(clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
	if (result == SOCKET_ERROR) {
		std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
		return false;
	}
	std::cout << "sent" << message << std::endl;
	return true;
}

bool Protocol::ReceiveResponse2String(std::string& response) {
	const int bufferSize = 4 << 20;
	std::vector<char> buffer(bufferSize);
	response.clear();  // 清空响应字符串，以确保接收新的数据

	while (true) {
		int result = recv(clientSocket, buffer.data(), bufferSize, 0);
		if (result > 0) {
			// 将接收到的数据追加到响应字符串中
			response.reserve(response.size() + result);
			response.append(buffer.data(), result);

			if (result < bufferSize) {
				break;
			}
		}
		else if (result == 0) {
			std::cerr << "Connection closed by server" << std::endl;
			break; // 连接关闭
		}
		else {
			std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
			break; // 发生错误
		}
	}

	return !response.empty(); // 如果响应非空，返回 true
}



// 接收响应并保存到文件
bool Protocol::ReceiveResponse2File(const std::string& fileName) {
	std::string response;
	if (ReceiveResponse2String(response)) {
		std::ofstream outFile(fileName, std::ios::binary);
		if (!outFile.is_open()) {
			std::cerr << "Could not open file: " << fileName << std::endl;
			return false;
		}
		outFile << response;
		outFile.close();
		return true;
	}
	return false;
}

// 
bool Protocol::ReceiveResponseAndSaveAsPng(const std::string& fileName) {
	std::string jsonResponse;
	if (!ReceiveResponse2String(jsonResponse)) {
		std::cerr << "Failed to receive JSON response." << std::endl;
		return false; // 添加返回值以避免继续执行
	}

	// 打印 JSON 响应以进行调试
	std::cout << "JSON Response: " << jsonResponse << std::endl;

	// 解析 JSON 响应
	Json::Value root;
	if (!string2json(jsonResponse, root)) {
		std::cerr << "Failed to parse JSON response." << std::endl;
		return false;
	}

	// 确认是否存在 result 字段
	if (!root.isMember("result")) {
		std::cerr << "The 'result' field is missing in the JSON response." << std::endl;
		return false;
	}

	// 获取 result 字段
	std::string base64Data = root["result"].asString();
	if (base64Data.empty()) {
		std::cerr << "'result' field is empty." << std::endl;
		return false;
	}

	// 创建BIO对象
	BIO* bio = BIO_new(BIO_f_base64());
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	// 创建内存BIO对象
	BIO* memBio = BIO_new_mem_buf(base64Data.c_str(), base64Data.length());

	// 将内存BIO连接到base64解码BIO
	BIO_push(bio, memBio);

	// 创建输出文件流
	std::ofstream outputFile(fileName, std::ios::binary);

	// 缓冲区大小
	const int bufferSize = 4096;
	char buffer[bufferSize];

	// 从base64解码BIO中读取数据并写入文件
	int bytesRead;
	while ((bytesRead = BIO_read(bio, buffer, bufferSize)) > 0) {
		outputFile.write(buffer, bytesRead);
	}

	// 释放资源
	BIO_free_all(bio);
	outputFile.close();

	return true;
}


//// Base64 解码函数
//std::vector<uint8_t> base64Decode(const std::string& input) {
//	BIO* bio;
//	BIO* b64;
//	BUF_MEM* bufferPtr;
//
//	b64 = BIO_new(BIO_f_base64());
//	bio = BIO_new_mem_buf(input.data(), input.size());
//	bio = BIO_push(b64, bio);
//	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 不要换行
//	BIO_get_buf(bio, &bufferPtr); // 获取输出缓冲区
//
//	std::vector<uint8_t> output(bufferPtr->length);
//	int decodedLength = BIO_read(bio, output.data(), output.size());
//
//	BIO_free_all(bio); // 清理
//	output.resize(decodedLength); // 调整输出大小
//	return output;
//}


// 远程调用 JSON-RPC 方法
bool Protocol::CallWithGenericRequest(const GenericRequest& request) {
	return SendJson(request.to_json());
}

// 关闭连接
void Protocol::Close() {
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
		WSACleanup();
		clientSocket = INVALID_SOCKET;
	}
}

// =================================== Util =============================================

// 将 std::wstring 转换为 UTF-8 编码的 std::string
std::string wstringToUtf8(const std::wstring& wstr) {
	if (wstr.empty()) {
		return "";
	}

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string utf8Str(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &utf8Str[0], sizeNeeded, nullptr, nullptr);
	return utf8Str;
}

// 将任意类型转为Json （要自己实现）
Json::Value convert_any_to_json(const std::any& operand) {
	Json::Value json_value;

	if (operand.type() == typeid(int)) {
		json_value = std::any_cast<int>(operand);
	}
	else if (operand.type() == typeid(bool)) {
		json_value = std::any_cast<bool>(operand);
	}
	else if (operand.type() == typeid(std::string)) {
		json_value=std::any_cast<std::string>(operand);
	}
	else if (operand.type() == typeid(std::wstring)) {
		std::wstring tmpStr = std::any_cast<std::wstring>(operand);
		std::string utf8Str = wstringToUtf8(tmpStr);
		json_value=utf8Str;
	}// 剩下的自己加
	else {
		std::cerr << "Unsupported parameter type" << std::endl;
		return false;
	}

	return json_value;
}

bool string2json(const std::string& jsonString, Json::Value& jsonValue)
{
	Json::CharReaderBuilder readerBuilder;
	std::string errs;

	// 使用 CharReader 将字符串解析为 JSON 对象
	std::istringstream iss(jsonString);
	if (!Json::parseFromStream(readerBuilder, iss, &jsonValue, &errs)) {
		std::cerr << "Failed to parse JSON: " << errs << std::endl;
		return false;
	}

	return true;
}

