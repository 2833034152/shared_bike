#ifndef NETWORK_INTERFACE_H_
#define NETWORK_INTERFACE_H_

#include <event.h>
#include <event2/event.h>
#include  <event2/listener.h>
#include  <string>

#include "ievent.h"
#include "glo_def.h"

#define MESSAGE_HEADER_LEN  10     //消息头部的总长度  "FEEB" + 2字节的数据类型 + 4字节数据长度
#define MESSAGE_HEADER_ID  "FEEB"  //消息头部标识

//class 相当于将成员限定作用域为SESSION_STATUS 
enum class  SESSION_STATUS {			//服务器的状态 是正在处理请求还是响应
	SS_REQUEST,
	SS_RESPONSE
};

enum class  MESSAGE_STATUS {
	MS_READ_HADER = 0,
	MS_READ_MESSAGE = 1,
	MS_READ_DONE = 2,
	MS_SENDING = 3
};


typedef struct _ConnectSession {
	char remote_ip[32];   //连接上来的客户端的ip地址
	SESSION_STATUS  session_stat;    //服务器的状态

	iEvent* request;
	MESSAGE_STATUS req_stat;

	iEvent* response;
	MESSAGE_STATUS res_stat;

	u16 eid;
	i32 fd;

	struct bufferevent* bev;  // _ConnectStat 中需要用到

	char* read_buf;    //保存消息的数据内容 
	u32 message_len;   //当前消息长度
	u32 read_message_len; //已经读取的消息长度

	char header[MESSAGE_HEADER_LEN + 1];  //保存头部 10 + 1个字节
	u32 read_header_len;  //已经去读头部的长度

	u32 sent_len;   //已经发送的长度
	char* write_buf;  //要写的缓冲区

} ConnectSession;




class NetworkInterface
{
public:
	NetworkInterface();
	~NetworkInterface();

	bool start(int port);    //开始链接网络
	void close();

	static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
		struct sockaddr* sock, int socklen, void* arg);       //监听事件

	static void handle_request(struct bufferevent* bev, void*);   //读请求回调
	static void handle_response(struct bufferevent* bev, void*);   //写,发送响应回调
	static void handle_error(struct bufferevent* bev, short event, void* arg);  //出了错误或者连接异常
	//void (*bufferevent_event_cb)(struct bufferevent *bev, short what, void *ctx);
	void network_event_dispatch();    //
	void send_response_message(ConnectSession* cs);  //服务器与客户端相关联再一起的状态用ConnectSession类来表示


private:
	struct event_base* base_;     //libevent 中用到的事件集
	struct evconnlistener* listener_;
};



#endif

