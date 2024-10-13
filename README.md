# myAutoGoRPC
myAutoGoRPC 用C++ 做中控，远程调用go的jsonrpc服务

用到了JsonCpp，websocketpp，socket

JsonCpp已经放到源文件里了，只有一个websocketpp需要添加lib

我这里是用的vcpkg，

```bash
vcpkg install websocketpp
```

怎么安装vcpkg和怎么在vs里使用可以自己网上找一下

都很简单的，照着做就行。然后就没有什么难的了

然后可能需要链接`libssl.lib`,`libcrypto.lib`,`Ws2_32.lib`

然后就是需要用到一个go的服务端，也就是投递到手机的二进制

可以看我的另一个仓库：

```json
https://github.com/Jeff-xu0/myAutoGoRPC
```



---

然后我简单讲一下代码，写的很乱，就是一个socket通信，protocol就是通信协议

里面有几个方法：

```C++
CallWithGenericRequest // 发送rpc请求调用远程方法

ReceiveResponse2String // 就收远程服务返回的信息

// GenericRequest 类 这个是请求用到的参数类
// 全部赋值完，调用CallWithGenericRequest即可发送请求
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
```

使用：
```C++
// 创建 Protocol 对象
Protocol protocol(serverIp, serverPort);

// 连接服务器
if (!protocol.Connect()) {
	std::cout<<"Failed to connect to server"<<std::endl;
	return -1;
}

std::cout << "Connected to server" << std::endl;
GenericRequest request(
	"Server.ClickScreen",
	{ {}, req_id },
	req_id);

protocol.CallWithGenericRequest(request);

std::string response;
if (protocol.ReceiveResponse2String(response)) {
	std::cout << "Response: %s" << response << std::endl;
}
client.send_message(static_cast<std::string>(BREAK_MSG));
// 关闭连接
protocol.Close();
```

然后讲一讲websocket：

```C++
// MyClient 实现基类websocket，这里可以看作是我们的游戏操作类
// 实现之后可以让每次服务端的消息走到我们的派生类方法内，再进行处理
int main(){
    MyClient client; 
    std::string  wsUri = "ws://" + static_cast<std::string>(serverIp) + ":" + std::to_string(wsPort) + "/ws/" + std::to_string(req_id);

    std::thread clientThread1(startClient, &client, wsUri);
    clientThread1.detach();
    Sleep(1000);
}
void startClient(MyClient* client,const std::string& wsUri) {
	client->connect(wsUri);
}
```



然后想要做中控的话就起几个线程来分别控制不同的手机就好，这个就不多讲了

实现的效果就是：


<video src="./result.mp4"></video>

其他的都没什么讲的了，反正代码都很简单，看不懂就google，chatgpt，poe
