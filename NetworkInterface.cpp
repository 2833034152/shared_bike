#include "NetworkInterface.h"
#include <string.h>  // ʹ�õ���memset����
#include "DispatchMsgService.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>




//#define LOG_DEBUG printf
//#define LOG_WARN  printf
//#define LOG_ERROR printf

//ConnectSession����ʱc���͵ĳ�Ա����
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

	//Ϊ����ͻ��˷���һ��bufferevent  
	struct bufferevent* bev = bufferevent_socket_new(base, fd,
		BEV_OPT_CLOSE_ON_FREE);   //���ر�buffereventʱ��ر�socket

	ConnectSession* cs = session_init(fd, bev);

	cs->session_stat = SESSION_STATUS::SS_REQUEST;
	cs->req_stat = MESSAGE_STATUS::MS_READ_HADER;

	strcpy(cs->remote_ip, inet_ntoa(((sockaddr_in*)sock)->sin_addr));  //ntoa ����Ķ���������ת��Ϊʮ�����ַ���,����Ϊ��ntop���
	LOG_DEBUG("remote ip : %s\n", cs->remote_ip);

	bufferevent_setcb(bev, handle_request, handle_response, handle_error, cs);    //�����ص�����
	//void (*bufferevent_event_cb)(struct bufferevent* bev, short what, void* ctx);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);
	//���ó�ʱ�Ͽ�����,��ֹ���������Ӷ��ܶ��ٷ�����
	bufferevent_settimeout(bev, 10, 10);       //10sû�ж�д����   ,��ʱ�͵�handle_error�ص�����

}

void NetworkInterface::handle_request(struct bufferevent* bev, void* arg) {
	ConnectSession* cs = (ConnectSession*)arg;

	//���кܶ������¼��ͻ�Ӧ�¼�ͬʱ����ʱ,���������ֻ���������¼�
	if (cs->session_stat != SESSION_STATUS::SS_REQUEST) {
		LOG_WARN("NetworkInterface::handle_request - wrong session state[%d]", cs->session_stat);
		return;
	}

	//��Էְ��Ķ�ȡ
	printf("do echo request ...\n");

	if (cs->req_stat == MESSAGE_STATUS::MS_READ_HADER) {     //��ȡͷ��
		i32  len = bufferevent_read(bev, cs->header + cs->read_header_len, MESSAGE_HEADER_LEN - cs->read_header_len);  //����:��������ȥ? ������?
		cs->read_header_len += len;

		cs->header[cs->read_header_len] = '\0';
		LOG_DEBUG("recv from client <<<< %s\n", cs->header);  //ͷ���Ѿ���ȡ���

		//����ͷ��
		if (cs->read_header_len == MESSAGE_HEADER_LEN) {     //����ȡͷ�����Ⱥ�

			if (strncmp(cs->header, MESSAGE_HEADER_ID, strlen(MESSAGE_HEADER_ID)) == 0) {   //����"FEEB"
				cs->eid = *((u16*)(cs->header + 4));
				cs->message_len = *((i32*)(cs->header + 6));
				LOG_DEBUG("NetworkInterface::handle_request - read %d bytes in header,message len: %d\n", cs->read_message_len, cs->message_len);

				if (cs->message_len < 1 || cs->message_len > MAX_MESSAGE_LEN) {     //�ж�������Ϣ�ĳ��ȷ���Ҫ��
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
				//ֱ�ӹر�����, �������κ���Ӧ, ��ֹ�ͻ��˶�����̽
				bufferevent_free(bev);
				session_free(cs);
				return;
			}
		}
	}

	//��ȡ״̬Ϊ ��ȡ���ݲ���  ��bufferevent �����buffer���������ݲ���
	if (cs->req_stat == MESSAGE_STATUS::MS_READ_MESSAGE && evbuffer_get_length(bufferevent_get_input(bev)) > 0)
	{
		i32  len = bufferevent_read(bev, cs->read_buf + cs->read_message_len, cs->message_len - cs->read_message_len);  //����:��������ȥ? ������?
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
				ev->set_args(cs);       //��Ӧ�������¼��й�ϵ , ���Թ�����
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

//��ʱ ���ӹر� ��д������쳣���ָ���Ļص�����  
//�ر�����: 1.log_debug ��ӡ��������־     2.�رտͻ��˵�socket����
void NetworkInterface::handle_error(struct bufferevent* bev, short event, void* arg) {

	ConnectSession* cs = (ConnectSession*)arg;                //��listener_cb�е�bufferevent_setcb���Զ�����cs����
	LOG_DEBUG("NetworkInterface:handler_error ...\n");

	if (event & BEV_EVENT_EOF) {                            //״̬����Ǳ�����event�е�
		LOG_DEBUG("connection is closed");

	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_READING)) {    //����ʱ
		LOG_DEBUG("NetworkInterface:: reading timeout...");     //  ���Դ�ӡ���Է���ip��ַ
	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_WRITING)) {    //д��ʱ
		LOG_DEBUG("NetworkInterface:: writing timeout...");     //  ���Դ�ӡ���Է���ip��ַ
	}
	else if (event & BEV_EVENT_ERROR) {
		LOG_DEBUG("NetworkInterface::  some other error...");      //����ҵ���һ���ͻ��˳���
	}

	//�ر��׽��� ��free��д������
	bufferevent_free(bev);
	session_free(cs);
}
//void (*bufferevent_event_cb)(struct bufferevent *bev, short what, void *ctx);

void NetworkInterface::network_event_dispatch() {
	event_base_loop(base_, EVLOOP_NONBLOCK);   //������¼��ʹ������¼���û�о����̷��أ�����ֻ�ܴ���һ�������¼�
	//������Ӧ�¼�, �ظ���Ӧ��Ϣ, δ�����...  �����¼�
	DispatchMsgService::getInstance()->handleAllResponseEvent(this);   //������Ӧ���¼�
}

void NetworkInterface::send_response_message(ConnectSession* cs)
{
	if (cs->response == nullptr) {     //�е��¼�����Ҫ��Ӧ
		bufferevent_free(cs->bev);
		if (cs->request) {
			delete cs->request;      //��Ϊsession_free��û�а�request���ٵ�,����Ҫ�Ѷ�Ӧ�������¼�Ҳ��ɾ����
		}
		session_free(cs);
	}
	else {
		//ͨ��bufferevent�����л������ݷ���ȥ��,��ʱ��ת��handle_response����
		bufferevent_write(cs->bev, cs->write_buf, cs->message_len + MESSAGE_HEADER_LEN);
		//��ʱ���ܻ����������󷢹���,���Խ��г�����,���ر�
		//���½�����������״̬
		session_reset(cs);


	}

}



