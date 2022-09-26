#include "DispatchMsgService.h"
#include "NetworkInterface.h"
#include <algorithm>
#include <vector>
#include "bike.pb.h"
#include "events_def.h"

//#define LOG_DEBUG printf
//#define LOG_WARN  printf
//#define LOG_ERROR printf

DispatchMsgService* DispatchMsgService::DMS_ = nullptr;  // 静态成员变量 需要当成全局变量进行初始化
pthread_mutex_t DispatchMsgService::queue_mutex;
std::queue<iEvent*>DispatchMsgService::response_events;  // 初始化队列
std::vector<iEvent*>DispatchMsgService::rsps;
// NetworkInterface* DispatchMsgService::NTIF_ = nullptr;

#include "bike.pb.h"      //测试LoginRsp data: 问题
using namespace std;
using namespace tutorial;

DispatchMsgService::DispatchMsgService() :tp(nullptr)
{
	//tp = NULL;      //线程池置为空
	//NTIF_ = NULL;   //非static成员变量才这样初始化

}

DispatchMsgService::~DispatchMsgService()
{

}

BOOL DispatchMsgService::open()    //表示线程池启动                                                                                                                                  
{
	svr_exit_ = FALSE;

	thread_mutex_create(&queue_mutex);     //给队列上锁
	tp = thread_pool_init();    //初始化后就可以向线程池中投递事件了  
	return tp ? TRUE : FALSE;     //线程池初始化成功为真,否则为假
}

void DispatchMsgService::close()
{
	svr_exit_ = TRUE;

	thread_pool_destroy(tp);
	thread_mutex_destroy(&queue_mutex);    //解锁
	subscribers_.clear();                  //把处理者数组给清零

	tp = NULL;                               //线程池置为空
}

void DispatchMsgService::svc(void* argv)       //由线城驱动(调用)   
{
	DispatchMsgService* dms = DispatchMsgService::getInstance();
	iEvent* ev = (iEvent*)argv;
	if (!dms->svr_exit_)
	{
		LOG_DEBUG("DispatchMsgService::svc ...\n");
		iEvent* rsp = dms->process(ev);
		//std::vector<iEvent*> rsps = dms->process(ev);
		//delete ev;   //还需斟酌删除事件
		if (rsp) {
			rsp->dump(std::cout);
			rsp->set_args(ev->get_args());

			

		}
		else {
			//send_response_message是单线程，不具备线程安全，通过handleAllResponseEvent进行响应
			/*if (NTIF_) {
				NTIF_->send_response_message((ConnectSession*)ev->get_args());
			}*/

			//生成终止响应事件，主要是做资源清理
			rsp = new ExitRspEv();
			rsp->set_args(ev->get_args());

		}

		thread_mutex_lock(&queue_mutex);
		response_events.push(rsp);
		thread_mutex_unlock(&queue_mutex);
		
	}
}

//把事件投递到线程池中进行处理
i32  DispatchMsgService::enqueue(iEvent* ev)
{
	if (NULL == ev)
	{
		return -1;
	}

	thread_task_t* task = thread_task_alloc(0);
	task->handler = DispatchMsgService::svc;    //设置该任务的回调函数
	task->ctx = ev;                             // 设置该回调函数的参数
	return thread_task_post(tp, task);          //把task任务投递到线程中去
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
	if (iter != subscribers_.end())       //根据eid找vector  T_EventHandlers  
	{
		T_EventHandlers::iterator hdl_iter = std::find(iter->second.begin(), iter->second.end(), handler);
		if (hdl_iter == iter->second.end())  //不存在handler
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
	if (iter != subscribers_.end())       //根据eid找vector  T_EventHandlers
	{
		T_EventHandlers::iterator hdl_iter = std::find(iter->second.begin(), iter->second.end(), handler);
		if (hdl_iter != iter->second.end())  //不存在handler
		{
			iter->second.erase(hdl_iter);
		}
	}
}

//调用handlerc处理并返回成事件event
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
	//没问题,继续往下执行
	
	T_EventHandlersMap::iterator handlers = subscribers_.find(eid);    //找到同一个事件id的各个客户端的订阅者的处理

	if (handlers == subscribers_.end())
	{
		LOG_WARN("DispatchMsgService : no any handler subscribed %d", eid);
		return NULL;
		
	}

	iEvent* rsp = NULL;
	for (auto iter = handlers->second.begin(); iter != handlers->second.end(); iter++)
	{
		iEventHandler* handler = *iter;      //*iter的handler是iEventHandler的子类
		LOG_DEBUG("DispatchMsgService : get handler : %s\n", handler->get_name().c_str());

		rsp = handler->handle(ev);              //多个订阅者只返回最后一个,不合理
		rsps.push_back(rsp);                    //把处理事件添加到处理事件数组中
	}
	//return rsp;
	//for (std::vector<iEvent*>::reverse_iterator rit = rsps.rbegin(); rit != rsps.rend(); ++rit) {
	//	iEvent* rsp = *rit;    //最后一个元素
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
		//未完待续
		tutorial::login_request  lr;
		if (lr.ParseFromArray(message, len)) {
			LoginReqEv* ev = new LoginReqEv(lr.mobile(), lr.icode());
			return ev;
		}
	}


	return nullptr;
}

//将响应的事件通过网络序列化数据发出去 , 各种进程将响应事件放进队列，主进程读取队列中的响应事件
void DispatchMsgService::handleAllResponseEvent(NetworkInterface* interface) {
	bool done = false;         //从队列中取事件是否成功标志

	while (!done) {
		iEvent* ev = nullptr;

		//因为线程池所在线程不断把响应事件放在响应队列中,主进程从队列中读取  ,此时必须要做互斥
		thread_mutex_lock(&queue_mutex);
		if (!response_events.empty()) {
			ev = response_events.front();     //event在这里已经被子类赋值，多态的用法
			response_events.pop();
		}
		else {
			done = true;
		}
		thread_mutex_unlock(&queue_mutex);    //解锁，读取完队列中队首元素后就要尽快解锁，不要耽误时间！

		if (!done) {      //把事件拿出来处理
			if (ev->get_eid() == EVENTID_GET_MOBILE_CODE_RSP) {
				MobileCodeRspEv* mcre = static_cast<Mwrite_bufobileCodeRspEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - id , EVENTID_GET_MOBILE_CODE_RSP\n");

				ConnectSession* cs = (ConnectSession*)ev->get_args();  //ev的这个参数专门是sessin会话
				cs->response = ev;         //事件的会话变量为cs,同时cs的响应事件变量为ev

				//序列化请求数据
				cs->message_len = mcre->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				//组装头部
				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EVENTID_GET_MOBILE_CODE_RSP;
				*(u32*)(cs->write_buf + 6) = cs->message_len;

				mcre->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);

				interface->send_response_message(cs);
			}
			else  if (ev->get_eid() == EVENTID_EXIT_RSP) {

				ConnectSession* cs = (ConnectSession*)ev->get_args();  //ev的这个参数专门是sessin会话
				cs->response = ev;         //事件的会话变量为cs,同时cs的响应事件变量为ev
				interface-> send_response_message(cs);
			}
			else  if (ev->get_eid() == EVENTID_LOGIN_RSP) {
				LoginResEv* mcre = static_cast<LoginResEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - id , EVENTID_LOGIN_RSP\n");

				ConnectSession* cs = (ConnectSession*)ev->get_args();  //ev的这个参数专门是sessin会话
				cs->response = ev;         //事件的会话变量为cs,同时cs的响应事件变量为ev

				LOG_DEBUG("DispatchMsgSerivice: code: %d ,data: %s", mcre->get_code(), mcre->get_data().c_str());
				//序列化请求数据
				cs->message_len = mcre->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				//组装头部
				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EVENTID_LOGIN_RSP;
				*(u32*)(cs->write_buf + 6) = cs->message_len;

				mcre->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);

				
				//测试LoginRsp传输过程中丢失data： 的问题
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
						printf("DispatchMsgSerivice - login_response: lr - code: %d, data: %s    ，   mcre - code: %d, data:%s \n", lr.code(), lr.desc().c_str(), mcre->get_code(), mcre->get_data().c_str());
					}

				}

				interface->send_response_message(cs);  //等测试完再把这句话发出去
			
			}
		}
	}



}
