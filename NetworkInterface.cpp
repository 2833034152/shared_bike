#include "NetworkInterface.h"
#include <string.h>  // 使用到了memset函数
#include "DispatchMsgService.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>




//#define LOG_DEBUG printf
//#define LOG_WARN  printf
//#define LOG_ERROR printf

//ConnectSession必须时c类型的成员变量
static ConnectSession* session_init(int fd, struct bufferevent* bev) {
	ConnectSession* temp = NULL;
	temp = (ConnectSession*)malloc(sizeof(ConnectSession));

	if (!temp) {
		fprintf(stderr, "malloc failed. reason: %m\n");
		return NULL;
	}

	memset(temp, '\0', sizeof(ConnectSession));
	temp->bev = bev;
	temp->fd = fd;

}

void session_reset(ConnectSession* cs) {
	if (cs) {
		if (cs->read_buf) {
			delete[]  cs->read_buf;
			cs->read_buf = nullptr;
		}
		if (cs->write_buf) {
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}
		cs->session_stat = SESSION_STATUS::SS_REQUEST;
		cs->req_stat = MESSAGE_STATUS::MS_READ_HADER;

		cs->message_len = 0;
		cs->read_message_len = 0;
		cs->read_header_len = 0;
	}
}

void session_free(ConnectSession* cs) {
	if (cs) {
		if (cs->read_buf) {
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
		}
		if (cs->write_buf) {
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}
		delete cs;
	}

}



NetworkInterface::NetworkInterface() {
	base_ = nullptr;
	listener_ = nullptr;

}

NetworkInterface::~NetworkInterface() {
	close();

}

bool NetworkInterface::start(int port) {
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	base_ = event_base_new();

	struct evconnlistener* listener_
		= evconnlistener_new_bind(base_, listener_cb, base_,
			LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
			512, (struct sockaddr*)&sin,
			sizeof(struct sockaddr_in));


}

void NetworkInterface::close() {
	if (base_) {
		event_base_free(base_);
		base_ = nullptr;
	}
	if (listener_) {
		evconnlistener_free(listener_);
		listener_ = nullptr;
	}
}

void NetworkInterface::listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
	struct sockaddr* sock, int socklen, void* arg) {
	struct event_base* base = (struct event_base*)arg;
	LOG_DEBUG("accept a client %d\n", fd);

	//为这个客户端分配一个bufferevent  
	struct bufferevent* bev = bufferevent_socket_new(base, fd,
		BEV_OPT_CLOSE_ON_FREE);   //当关闭bufferevent时会关闭socket

	ConnectSession* cs = session_init(fd, bev);

	cs->session_stat = SESSION_STATUS::SS_REQUEST;
	cs->req_stat = MESSAGE_STATUS::MS_READ_HADER;

	strcpy(cs->remote_ip, inet_ntoa(((sockaddr_in*)sock)->sin_addr));  //ntoa 网络的二进制数据转换为十进制字符串,我认为和ntop差不多
	LOG_DEBUG("remote ip : %s\n", cs->remote_ip);

	bufferevent_setcb(bev, handle_request, handle_response, handle_error, cs);    //三个回调函数
	//void (*bufferevent_event_cb)(struct bufferevent* bev, short what, void* ctx);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);
	//设置超时断开连接,防止恶意多个链接而很多少发数据
	bufferevent_settimeout(bev, 10, 10);       //10s没有读写数据   ,超时就到handle_error回调函数

}

void NetworkInterface::handle_request(struct bufferevent* bev, void* arg) {
	ConnectSession* cs = (ConnectSession*)arg;

	//当有很多请求事件和回应事件同时进来时,再这个里面只处理请求事件
	if (cs->session_stat != SESSION_STATUS::SS_REQUEST) {
		LOG_WARN("NetworkInterface::handle_request - wrong session state[%d]", cs->session_stat);
		return;
	}

	//针对分包的读取
	printf("do echo request ...\n");

	if (cs->req_stat == MESSAGE_STATUS::MS_READ_HADER) {     //读取头部
		i32  len = bufferevent_read(bev, cs->header + cs->read_header_len, MESSAGE_HEADER_LEN - cs->read_header_len);  //参数:读到哪里去? 读多少?
		cs->read_header_len += len;

		cs->header[cs->read_header_len] = '\0';
		LOG_DEBUG("recv from client <<<< %s\n", cs->header);  //头部已经读取完毕

		//检验头部
		if (cs->read_header_len == MESSAGE_HEADER_LEN) {     //当读取头部长度后

			if (strncmp(cs->header, MESSAGE_HEADER_ID, strlen(MESSAGE_HEADER_ID)) == 0) {   //检验"FEEB"
				cs->eid = *((u16*)(cs->header + 4));
				cs->message_len = *((i32*)(cs->header + 6));
				LOG_DEBUG("NetworkInterface::handle_request - read %d bytes in header,message len: %d\n", cs->read_message_len, cs->message_len);

				if (cs->message_len < 1 || cs->message_len > MAX_MESSAGE_LEN) {     //判断请求信息的长度符合要求不
					LOG_ERROR("NetworkInterface::handle_request - wrong message , len: %u", cs->message_len);
					bufferevent_free(bev);
					session_free(cs);
					return;
				}

				cs->read_buf = new char[cs->message_len];
				cs->req_stat = MESSAGE_STATUS::MS_READ_MESSAGE;
				cs->read_message_len = 0;

			}
			else
			{
				LOG_ERROR("NetWorkInterface::handle_request - Invalid request from %s\n", cs->remote_ip);
				//直接关闭请求, 不给予任何响应, 防止客户端恶意试探
				bufferevent_free(bev);
				session_free(cs);
				return;
			}
		}
	}

	//读取状态为 读取数据部分  且bufferevent 缓存的buffer里面有数据部分
	if (cs->req_stat == MESSAGE_STATUS::MS_READ_MESSAGE && evbuffer_get_length(bufferevent_get_input(bev)) > 0)
	{
		i32  len = bufferevent_read(bev, cs->read_buf + cs->read_message_len, cs->message_len - cs->read_message_len);  //参数:读到哪里去? 读多少?
		cs->read_message_len += len;
		LOG_DEBUG("NetworkInterface::bufferevent_read: %d bytes,message len: %d\n", len, cs->message_len);
		cs->header[cs->read_message_len] = '\0';

		if (cs->message_len == cs->read_message_len) {
			cs->session_stat = SESSION_STATUS::SS_RESPONSE;
			iEvent* ev = DispatchMsgService::getInstance()->parseEvent(cs->read_buf, cs->read_message_len, cs->eid);

			delete[] cs->read_buf;
			cs->read_buf = nullptr;
			cs->read_message_len = 0;

			if (ev) {
				ev->set_args(cs);       //响应和请求事件有关系 , 所以关联上
				DispatchMsgService::getInstance()->enqueue(ev);
			}
			else
			{
				LOG_ERROR("NetworkInterface::handle_request - ev is null. remote ip: %s, eid: %d\n", cs->remote_ip, cs->eid);
				bufferevent_free(bev);
				session_free(cs);
				return;
			}
		}
	}
}


void NetworkInterface::handle_response(struct bufferevent* bev, void*) {

	i32 icode;
	LOG_DEBUG("NetworkInterface::handle_response ...\n");
	
}

//超时 连接关闭 读写出错等异常情况指定的回调函数  
//关闭流程: 1.log_debug 打印出错误日志     2.关闭客户端的socket连接
void NetworkInterface::handle_error(struct bufferevent* bev, short event, void* arg) {

	ConnectSession* cs = (ConnectSession*)arg;                //再listener_cb中的bufferevent_setcb会自动传递cs过来
	LOG_DEBUG("NetworkInterface:handler_error ...\n");

	if (event & BEV_EVENT_EOF) {                            //状态标记是保存在event中的
		LOG_DEBUG("connection is closed");

	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_READING)) {    //读超时
		LOG_DEBUG("NetworkInterface:: reading timeout...");     //  可以打印出对方的ip地址
	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_WRITING)) {    //写超时
		LOG_DEBUG("NetworkInterface:: writing timeout...");     //  可以打印出对方的ip地址
	}
	else if (event & BEV_EVENT_ERROR) {
		LOG_DEBUG("NetworkInterface::  some other error...");      //最好找到哪一个客户端出错
	}

	//关闭套接字 和free读写缓冲区
	bufferevent_free(bev);
	session_free(cs);
}
//void (*bufferevent_event_cb)(struct bufferevent *bev, short what, void *ctx);

void NetworkInterface::network_event_dispatch() {
	event_base_loop(base_, EVLOOP_NONBLOCK);   //如果有事件就处理完事件，没有就立刻返回，这里只能处理一个读的事件
	//处理响应事件, 回复响应消息, 未完待续...  请求事件
	DispatchMsgService::getInstance()->handleAllResponseEvent(this);   //处理响应的事件
}

void NetworkInterface::send_response_message(ConnectSession* cs)
{
	if (cs->response == nullptr) {     //有的事件不需要响应
		bufferevent_free(cs->bev);
		if (cs->request) {
			delete cs->request;      //因为session_free中没有吧request销毁掉,所以要把对应的请求事件也给删除掉
		}
		session_free(cs);
	}
	else {
		//通过bufferevent将序列化的数据发出去了,此时会转到handle_response函数
		bufferevent_write(cs->bev, cs->write_buf, cs->message_len + MESSAGE_HEADER_LEN);
		//此时可能还有其他请求发过来,所以进行长链接,不关闭
		//重新进入接收请求的状态
		session_reset(cs);


	}

}



