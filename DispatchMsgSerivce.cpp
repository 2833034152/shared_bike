#include "DispatchMsgService.h"
#include "NetworkInterface.h"
#include <algorithm>
#include <vector>
#include "bike.pb.h"
#include "events_def.h"

//#define LOG_DEBUG printf
//#define LOG_WARN  printf
//#define LOG_ERROR printf

DispatchMsgService* DispatchMsgService::DMS_ = nullptr;  // ��̬��Ա���� ��Ҫ����ȫ�ֱ������г�ʼ��
pthread_mutex_t DispatchMsgService::queue_mutex;
std::queue<iEvent*>DispatchMsgService::response_events;  // ��ʼ������
std::vector<iEvent*>DispatchMsgService::rsps;
// NetworkInterface* DispatchMsgService::NTIF_ = nullptr;

#include "bike.pb.h"      //����LoginRsp data: ����
using namespace std;
using namespace tutorial;

DispatchMsgService::DispatchMsgService() :tp(nullptr)
{
	//tp = NULL;      //�̳߳���Ϊ��
	//NTIF_ = NULL;   //��static��Ա������������ʼ��

}

DispatchMsgService::~DispatchMsgService()
{

}

BOOL DispatchMsgService::open()    //��ʾ�̳߳�����                                                                                                                                  
{
	svr_exit_ = FALSE;

	thread_mutex_create(&queue_mutex);     //����������
	tp = thread_pool_init();    //��ʼ����Ϳ������̳߳���Ͷ���¼���  
	return tp ? TRUE : FALSE;     //�̳߳س�ʼ���ɹ�Ϊ��,����Ϊ��
}

void DispatchMsgService::close()
{
	svr_exit_ = TRUE;

	thread_pool_destroy(tp);
	thread_mutex_destroy(&queue_mutex);    //����
	subscribers_.clear();                  //�Ѵ��������������

	tp = NULL;                               //�̳߳���Ϊ��
}

void DispatchMsgService::svc(void* argv)       //���߳�����(����)   
{
	DispatchMsgService* dms = DispatchMsgService::getInstance();
	iEvent* ev = (iEvent*)argv;
	if (!dms->svr_exit_)
	{
		LOG_DEBUG("DispatchMsgService::svc ...\n");
		iEvent* rsp = dms->process(ev);
		//std::vector<iEvent*> rsps = dms->process(ev);
		//delete ev;   //��������ɾ���¼�
		if (rsp) {
			rsp->dump(std::cout);
			rsp->set_args(ev->get_args());

			

		}
		else {
			//send_response_message�ǵ��̣߳����߱��̰߳�ȫ��ͨ��handleAllResponseEvent������Ӧ
			/*if (NTIF_) {
				NTIF_->send_response_message((ConnectSession*)ev->get_args());
			}*/

			//������ֹ��Ӧ�¼�����Ҫ������Դ����
			rsp = new ExitRspEv();
			rsp->set_args(ev->get_args());

		}

		thread_mutex_lock(&queue_mutex);
		response_events.push(rsp);
		thread_mutex_unlock(&queue_mutex);
		
	}
}

//���¼�Ͷ�ݵ��̳߳��н��д���
i32  DispatchMsgService::enqueue(iEvent* ev)
{
	if (NULL == ev)
	{
		return -1;
	}

	thread_task_t* task = thread_task_alloc(0);
	task->handler = DispatchMsgService::svc;    //���ø�����Ļص�����
	task->ctx = ev;                             // ���øûص������Ĳ���
	return thread_task_post(tp, task);          //��task����Ͷ�ݵ��߳���ȥ
}

DispatchMsgService* DispatchMsgService::getInstance()
{
	if (DMS_ == nullptr) {
		DMS_ = new DispatchMsgService();
	}
	return DMS_;
}

void  DispatchMsgService::subscribe(u32 eid, iEventHandler* handler)
{
	LOG_DEBUG("DispatchMsgService::subscribe eid: %u\n\n", eid);
	T_EventHandlersMap::iterator iter = subscribers_.find(eid);
	if (iter != subscribers_.end())       //����eid��vector  T_EventHandlers  
	{
		T_EventHandlers::iterator hdl_iter = std::find(iter->second.begin(), iter->second.end(), handler);
		if (hdl_iter == iter->second.end())  //������handler
		{
			iter->second.push_back(handler);
		}
	}
	else
	{
		subscribers_[eid].push_back(handler);
	}

}

void DispatchMsgService::unsubscribe(u32 eid, iEventHandler* handler)
{
	T_EventHandlersMap::iterator iter = subscribers_.find(eid);
	if (iter != subscribers_.end())       //����eid��vector  T_EventHandlers
	{
		T_EventHandlers::iterator hdl_iter = std::find(iter->second.begin(), iter->second.end(), handler);
		if (hdl_iter != iter->second.end())  //������handler
		{
			iter->second.erase(hdl_iter);
		}
	}
}

//����handlerc�������س��¼�event
iEvent* DispatchMsgService::process(const iEvent* ev)
{
	LOG_DEBUG("DispatchMsgService::process -ev: %p\n", ev);
	if (NULL == ev)
	{
		return NULL;
		
	}

	u32 eid = ev->get_eid();
	LOG_DEBUG("DispatchMsgService::process -eid: %u\n", eid);

	if (eid == EVENTID_UNKNOW)
	{
		LOG_WARN("DispatchMsgService::unknow event id: %d\n", eid);
		return NULL;
	
	}
	//û����,��������ִ��
	
	T_EventHandlersMap::iterator handlers = subscribers_.find(eid);    //�ҵ�ͬһ���¼�id�ĸ����ͻ��˵Ķ����ߵĴ���

	if (handlers == subscribers_.end())
	{
		LOG_WARN("DispatchMsgService : no any handler subscribed %d", eid);
		return NULL;
		
	}

	iEvent* rsp = NULL;
	for (auto iter = handlers->second.begin(); iter != handlers->second.end(); iter++)
	{
		iEventHandler* handler = *iter;      //*iter��handler��iEventHandler������
		LOG_DEBUG("DispatchMsgService : get handler : %s\n", handler->get_name().c_str());

		rsp = handler->handle(ev);              //���������ֻ�������һ��,������
		rsps.push_back(rsp);                    //�Ѵ����¼���ӵ������¼�������
	}
	//return rsp;
	//for (std::vector<iEvent*>::reverse_iterator rit = rsps.rbegin(); rit != rsps.rend(); ++rit) {
	//	iEvent* rsp = *rit;    //���һ��Ԫ��
	//	rsps.pop_back();
	//}
	return rsp;
}

iEvent* DispatchMsgService::parseEvent(const char* message, u32 len, u16 eid)
{
	if (!message) {
		LOG_ERROR("DispatchMsgService::parseEvent - message is null[eid].\n", eid);
		return nullptr;
	}

	if (eid == EVENTID_GET_MOBILE_CODE_REQ) {
		tutorial::mobile_request  mr;
		if (mr.ParseFromArray(message, len)) {
			MobileCodeReqEv* ev = new MobileCodeReqEv(mr.mobile());
			return ev;
		}
	}
	else if (eid == EVENTID_LOIGIN_REQ) {
		//δ�����
		tutorial::login_request  lr;
		if (lr.ParseFromArray(message, len)) {
			LoginReqEv* ev = new LoginReqEv(lr.mobile(), lr.icode());
			return ev;
		}
	}


	return nullptr;
}

//����Ӧ���¼�ͨ���������л����ݷ���ȥ , ���ֽ��̽���Ӧ�¼��Ž����У������̶�ȡ�����е���Ӧ�¼�
void DispatchMsgService::handleAllResponseEvent(NetworkInterface* interface) {
	bool done = false;         //�Ӷ�����ȡ�¼��Ƿ�ɹ���־

	while (!done) {
		iEvent* ev = nullptr;

		//��Ϊ�̳߳������̲߳��ϰ���Ӧ�¼�������Ӧ������,�����̴Ӷ����ж�ȡ  ,��ʱ����Ҫ������
		thread_mutex_lock(&queue_mutex);
		if (!response_events.empty()) {
			ev = response_events.front();     //event�������Ѿ������ำֵ����̬���÷�
			response_events.pop();
		}
		else {
			done = true;
		}
		thread_mutex_unlock(&queue_mutex);    //��������ȡ������ж���Ԫ�غ��Ҫ�����������Ҫ����ʱ�䣡

		if (!done) {      //���¼��ó�������
			if (ev->get_eid() == EVENTID_GET_MOBILE_CODE_RSP) {
				MobileCodeRspEv* mcre = static_cast<Mwrite_bufobileCodeRspEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - id , EVENTID_GET_MOBILE_CODE_RSP\n");

				ConnectSession* cs = (ConnectSession*)ev->get_args();  //ev���������ר����sessin�Ự
				cs->response = ev;         //�¼��ĻỰ����Ϊcs,ͬʱcs����Ӧ�¼�����Ϊev

				//���л���������
				cs->message_len = mcre->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				//��װͷ��
				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EVENTID_GET_MOBILE_CODE_RSP;
				*(u32*)(cs->write_buf + 6) = cs->message_len;

				mcre->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);

				interface->send_response_message(cs);
			}
			else  if (ev->get_eid() == EVENTID_EXIT_RSP) {

				ConnectSession* cs = (ConnectSession*)ev->get_args();  //ev���������ר����sessin�Ự
				cs->response = ev;         //�¼��ĻỰ����Ϊcs,ͬʱcs����Ӧ�¼�����Ϊev
				interface-> send_response_message(cs);
			}
			else  if (ev->get_eid() == EVENTID_LOGIN_RSP) {
				LoginResEv* mcre = static_cast<LoginResEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - id , EVENTID_LOGIN_RSP\n");

				ConnectSession* cs = (ConnectSession*)ev->get_args();  //ev���������ר����sessin�Ự
				cs->response = ev;         //�¼��ĻỰ����Ϊcs,ͬʱcs����Ӧ�¼�����Ϊev

				LOG_DEBUG("DispatchMsgSerivice: code: %d ,data: %s", mcre->get_code(), mcre->get_data().c_str());
				//���л���������
				cs->message_len = mcre->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				//��װͷ��
				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EVENTID_LOGIN_RSP;
				*(u32*)(cs->write_buf + 6) = cs->message_len;

				mcre->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);

				
				//����LoginRsp��������ж�ʧdata�� ������
				i32 icode;

				//char msg[1024];
				//size_t len = bufferevent_read(bev, msg, sizeof(msg));
				//memcpy(msg, cs->write_buf, MESSAGE_HEADER_LEN + cs->message_len);
				//msg[MESSAGE_HEADER_LEN + cs->message_len + 1] = '\0';

				//printf("DispatchMsgSerivice - recv %s from server, len: %d\n", msg, len);
				if (strncmp(cs->write_buf, "FEEB", 4) == 0) {

					u16 code = *(u16*)(cs->write_buf + 4);
					i32 len = *(i32*)(cs->write_buf + 6);
					if (code == 2) {
						mobile_response mr;
						mr.ParseFromArray(cs->write_buf + 10, len);
						icode = mr.icode();
						printf("DispatchMsgSerivice - mobile_response: code: %d, icode: %d, data: %s\n", mr.code(), mr.icode(), mr.data().c_str());
					}
					else if (code == 4) {
						login_response lr;
						lr.ParseFromArray(cs->write_buf + 10, len);
						code = lr.code();
						printf("DispatchMsgSerivice - login_response: lr - code: %d, data: %s    ��   mcre - code: %d, data:%s \n", lr.code(), lr.desc().c_str(), mcre->get_code(), mcre->get_data().c_str());
					}

				}

				interface->send_response_message(cs);  //�Ȳ������ٰ���仰����ȥ
			
			}
		}
	}



}
