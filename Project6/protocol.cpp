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


// ʵ�� to_json ����
Json::Value GenericRequest::to_json() const {
	Json::Value json_request;

	// ���� method �� id
	json_request["method"] = method;
	json_request["id"] = id;
	json_request["jsonrpc"] = "2.0";

	// ���� params ����
	Json::Value json_params(Json::arrayValue);

	// �������� "parameter" ����Ķ���
	Json::Value json_param_object; // obj
	Json::Value json_p_array(Json::arrayValue); // arr

	// ���� params.p �е� std::any ��ת��Ϊ JSON
	for (const auto& param : params.parameter) {
		json_p_array.append(convert_any_to_json(param));
	}

	// �� "parameter" ����������
	json_param_object["parameter"] = json_p_array;
	json_param_object["id"] = params.req_id;

	// ��������� params ����
	json_params.append(json_param_object);

	// �� params �����������
	json_request["params"] = json_params;

	return json_request;
}

// =================================== GenericResponse =============================================

GenericResponse::GenericResponse() : id(0) {}


// GenericResponse ��� from_json ����
bool GenericResponse::from_json(const Json::Value& json) {
	try {
		// ��� json �Ƿ�Ϊ��Ч�Ķ���
		if (!json.isObject()) {
			std::cerr << "Error: The provided JSON is not an object." << std::endl;
			return false;
		}

		// ��� "result" �ֶ��Ƿ���ڲ�����
		if (json.isMember("result")) {
			result = json["result"].asString();
		}
		else
		{
			result = "";
		}

		// ���� "error" �ֶ�
		if (json.isMember("error") && json["error"].isString()) {
			error = json["error"].asString();
		}
		else {
			error = "";
		}

		// ���� "id" �ֶ�
		if (json.isMember("id") && json["id"].isInt()) {
			id = json["id"].asInt();
		}
		else {
			id = -1;  // ��� ID �����ڣ�����ΪĬ��ֵ -1
		}

		return true;
	}
	catch (const Json::LogicError& e) {
		std::cerr << "JSON parsing error: " << e.what() << std::endl;
		return false;
	}
}


// =================================== Protocol =============================================

// Protocol ���캯������ʼ�������� IP �Ͷ˿�
Protocol::Protocol(const std::string& serverIp, int serverPort)
	: serverIp(serverIp), serverPort(serverPort), clientSocket(INVALID_SOCKET) {}

// Protocol �����������ر�����
Protocol::~Protocol() {
	Close();
}

// ���ӷ�����
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

// ���� JSON ����
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
	response.clear();  // �����Ӧ�ַ�������ȷ�������µ�����

	while (true) {
		int result = recv(clientSocket, buffer.data(), bufferSize, 0);
		if (result > 0) {
			// �����յ�������׷�ӵ���Ӧ�ַ�����
			response.reserve(response.size() + result);
			response.append(buffer.data(), result);

			if (result < bufferSize) {
				break;
			}
		}
		else if (result == 0) {
			std::cerr << "Connection closed by server" << std::endl;
			break; // ���ӹر�
		}
		else {
			std::cerr << "Receive failed: " << WSAGetLastError() << std::endl;
			break; // ��������
		}
	}

	return !response.empty(); // �����Ӧ�ǿգ����� true
}



// ������Ӧ�����浽�ļ�
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
		return false; // ��ӷ���ֵ�Ա������ִ��
	}

	// ��ӡ JSON ��Ӧ�Խ��е���
	std::cout << "JSON Response: " << jsonResponse << std::endl;

	// ���� JSON ��Ӧ
	Json::Value root;
	if (!string2json(jsonResponse, root)) {
		std::cerr << "Failed to parse JSON response." << std::endl;
		return false;
	}

	// ȷ���Ƿ���� result �ֶ�
	if (!root.isMember("result")) {
		std::cerr << "The 'result' field is missing in the JSON response." << std::endl;
		return false;
	}

	// ��ȡ result �ֶ�
	std::string base64Data = root["result"].asString();
	if (base64Data.empty()) {
		std::cerr << "'result' field is empty." << std::endl;
		return false;
	}

	// ����BIO����
	BIO* bio = BIO_new(BIO_f_base64());
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	// �����ڴ�BIO����
	BIO* memBio = BIO_new_mem_buf(base64Data.c_str(), base64Data.length());

	// ���ڴ�BIO���ӵ�base64����BIO
	BIO_push(bio, memBio);

	// ��������ļ���
	std::ofstream outputFile(fileName, std::ios::binary);

	// ��������С
	const int bufferSize = 4096;
	char buffer[bufferSize];

	// ��base64����BIO�ж�ȡ���ݲ�д���ļ�
	int bytesRead;
	while ((bytesRead = BIO_read(bio, buffer, bufferSize)) > 0) {
		outputFile.write(buffer, bytesRead);
	}

	// �ͷ���Դ
	BIO_free_all(bio);
	outputFile.close();

	return true;
}


//// Base64 ���뺯��
//std::vector<uint8_t> base64Decode(const std::string& input) {
//	BIO* bio;
//	BIO* b64;
//	BUF_MEM* bufferPtr;
//
//	b64 = BIO_new(BIO_f_base64());
//	bio = BIO_new_mem_buf(input.data(), input.size());
//	bio = BIO_push(b64, bio);
//	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // ��Ҫ����
//	BIO_get_buf(bio, &bufferPtr); // ��ȡ���������
//
//	std::vector<uint8_t> output(bufferPtr->length);
//	int decodedLength = BIO_read(bio, output.data(), output.size());
//
//	BIO_free_all(bio); // ����
//	output.resize(decodedLength); // ���������С
//	return output;
//}


// Զ�̵��� JSON-RPC ����
bool Protocol::CallWithGenericRequest(const GenericRequest& request) {
	return SendJson(request.to_json());
}

// �ر�����
void Protocol::Close() {
	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
		WSACleanup();
		clientSocket = INVALID_SOCKET;
	}
}

// =================================== Util =============================================

// �� std::wstring ת��Ϊ UTF-8 ����� std::string
std::string wstringToUtf8(const std::wstring& wstr) {
	if (wstr.empty()) {
		return "";
	}

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string utf8Str(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &utf8Str[0], sizeNeeded, nullptr, nullptr);
	return utf8Str;
}

// ����������תΪJson ��Ҫ�Լ�ʵ�֣�
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
	}// ʣ�µ��Լ���
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

	// ʹ�� CharReader ���ַ�������Ϊ JSON ����
	std::istringstream iss(jsonString);
	if (!Json::parseFromStream(readerBuilder, iss, &jsonValue, &errs)) {
		std::cerr << "Failed to parse JSON: " << errs << std::endl;
		return false;
	}

	return true;
}

