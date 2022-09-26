
/***********************************************************************
*  负责分发消息服务模块,   消息  事件event   ,把event传递给线程池的消息队列,
* 线程池的process方法调用每个event的handler方法  (提前subsribe才会调用)
*************************************************************************/

#ifndef  BRKS_SERVICE_DISPATCH_EVENT_SERVICE_H_
#define  BRKS_SERVICE_DISPATCH_EVENT_SERVICE_H_

#include <map>
#include <vector>
#include <queue>            
#include "ievent.h"                              //里面包含了glo_def.h ，从而包含了"Logger.h"
#include "eventtype.h"
#include "iEventHandler.h"
#include "threadpool/thread_pool.h"
#include "NetworkInterface.h"


class DispatchMsgService
{
protected:
	DispatchMsgService();    //只有自己和子类可以进行构造,参考单例模式
public:

	virtual ~DispatchMsgService();

	virtual BOOL open();   // 打开线程池
	virtual void  close();

	virtual  void subscribe(u32 eid, iEventHandler* handler);  //  把事件和其中一个事件处理器相互绑定
	virtual  void  unsubscribe(u32 eid, iEventHandler* handler);

	//把事件投递到线程池中进行处理
	virtual i32 enqueue(iEvent* ev);

	//线程池回调函数
	static void svc(void* argv);    //用C++的成员函数时必须用static,因为参数隐含传递了this指针指向当前的实例对象

	//对事件进行分发处理 , 不同的事件用不同的事件处理器进行处理
	virtual  iEvent* process(const iEvent* ev);

	static DispatchMsgService* getInstance();   //单例模式   

	iEvent* parseEvent(const char* message, u32 len, u16 eid);

//		static	void setNetworkInterface(NetworkInterface* nwif) { NTIF_ = nwif; };   //NetworkInterface 网络接口就不在构造函数中使用了,因为单例模式

	void handleAllResponseEvent(NetworkInterface* interface);   //将响应的事件通过网络序列化数据发出去
protected:

	thread_pool_t* tp;

	static DispatchMsgService* DMS_;   //单例模式,就这样一个

	typedef std::vector<iEventHandler*>T_EventHandlers;  //处理者数组
	typedef std::map<u32, T_EventHandlers> T_EventHandlersMap;
	T_EventHandlersMap subscribers_;

	bool  svr_exit_;   // 表示线程池打开关闭后服务的状态

	//多个线程处理完请求后产生的事件 都会往该队列中投放
	//网络接口NetworkInterface 会吧响应事件序列化成网络上响应的数据message并给客户端  
	static std::queue<iEvent*> response_events;
	static pthread_mutex_t queue_mutex;                   //各个线程都会用到这个队列所以要同步互斥,专门用于给队列上锁
	static std::vector<iEvent*> rsps;
//		 static NetworkInterface* NTIF_;            //因为要被静态方法svc使用, 所以此处用静态成员变量
};













#endif // ! BRKS_SERVICE_DISPATCH_EVENT_SERVICE_H_

