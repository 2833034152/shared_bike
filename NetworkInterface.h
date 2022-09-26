#ifndef NETWORK_INTERFACE_H_
#define NETWORK_INTERFACE_H_

#include <event.h>
#include <event2/event.h>
#include  <event2/listener.h>
#include  <string>

#include "ievent.h"
#include "glo_def.h"

#define MESSAGE_HEADER_LEN  10     //��Ϣͷ�����ܳ���  "FEEB" + 2�ֽڵ��������� + 4�ֽ����ݳ���
#define MESSAGE_HEADER_ID  "FEEB"  //��Ϣͷ����ʶ

//class �൱�ڽ���Ա�޶�������ΪSESSION_STATUS 
enum class  SESSION_STATUS {			//��������״̬ �����ڴ�����������Ӧ
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
	char remote_ip[32];   //���������Ŀͻ��˵�ip��ַ
	SESSION_STATUS  session_stat;    //��������״̬

	iEvent* request;
	MESSAGE_STATUS req_stat;

	iEvent* response;
	MESSAGE_STATUS res_stat;

	u16 eid;
	i32 fd;

	struct bufferevent* bev;  // _ConnectStat ����Ҫ�õ�

	char* read_buf;    //������Ϣ���������� 
	u32 message_len;   //��ǰ��Ϣ����
	u32 read_message_len; //�Ѿ���ȡ����Ϣ����

	char header[MESSAGE_HEADER_LEN + 1];  //����ͷ�� 10 + 1���ֽ�
	u32 read_header_len;  //�Ѿ�ȥ��ͷ���ĳ���

	u32 sent_len;   //�Ѿ����͵ĳ���
	char* write_buf;  //Ҫд�Ļ�����

} ConnectSession;




class NetworkInterface
{
public:
	NetworkInterface();
	~NetworkInterface();

	bool start(int port);    //��ʼ��������
	void close();

	static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
		struct sockaddr* sock, int socklen, void* arg);       //�����¼�

	static void handle_request(struct bufferevent* bev, void*);   //������ص�
	static void handle_response(struct bufferevent* bev, void*);   //д,������Ӧ�ص�
	static void handle_error(struct bufferevent* bev, short event, void* arg);  //���˴�����������쳣
	//void (*bufferevent_event_cb)(struct bufferevent *bev, short what, void *ctx);
	void network_event_dispatch();    //
	void send_response_message(ConnectSession* cs);  //��������ͻ����������һ���״̬��ConnectSession������ʾ


private:
	struct event_base* base_;     //libevent ���õ����¼���
	struct evconnlistener* listener_;
};



#endif

