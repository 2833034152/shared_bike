
/***********************************************************************
*  ����ַ���Ϣ����ģ��,   ��Ϣ  �¼�event   ,��event���ݸ��̳߳ص���Ϣ����,
* �̳߳ص�process��������ÿ��event��handler����  (��ǰsubsribe�Ż����)
*************************************************************************/

#ifndef  BRKS_SERVICE_DISPATCH_EVENT_SERVICE_H_
#define  BRKS_SERVICE_DISPATCH_EVENT_SERVICE_H_

#include <map>
#include <vector>
#include <queue>            
#include "ievent.h"                              //���������glo_def.h ���Ӷ�������"Logger.h"
#include "eventtype.h"
#include "iEventHandler.h"
#include "threadpool/thread_pool.h"
#include "NetworkInterface.h"


class DispatchMsgService
{
protected:
	DispatchMsgService();    //ֻ���Լ���������Խ��й���,�ο�����ģʽ
public:

	virtual ~DispatchMsgService();

	virtual BOOL open();   // ���̳߳�
	virtual void  close();

	virtual  void subscribe(u32 eid, iEventHandler* handler);  //  ���¼�������һ���¼��������໥��
	virtual  void  unsubscribe(u32 eid, iEventHandler* handler);

	//���¼�Ͷ�ݵ��̳߳��н��д���
	virtual i32 enqueue(iEvent* ev);

	//�̳߳ػص�����
	static void svc(void* argv);    //��C++�ĳ�Ա����ʱ������static,��Ϊ��������������thisָ��ָ��ǰ��ʵ������

	//���¼����зַ����� , ��ͬ���¼��ò�ͬ���¼����������д���
	virtual  iEvent* process(const iEvent* ev);

	static DispatchMsgService* getInstance();   //����ģʽ   

	iEvent* parseEvent(const char* message, u32 len, u16 eid);

//		static	void setNetworkInterface(NetworkInterface* nwif) { NTIF_ = nwif; };   //NetworkInterface ����ӿھͲ��ڹ��캯����ʹ����,��Ϊ����ģʽ

	void handleAllResponseEvent(NetworkInterface* interface);   //����Ӧ���¼�ͨ���������л����ݷ���ȥ
protected:

	thread_pool_t* tp;

	static DispatchMsgService* DMS_;   //����ģʽ,������һ��

	typedef std::vector<iEventHandler*>T_EventHandlers;  //����������
	typedef std::map<u32, T_EventHandlers> T_EventHandlersMap;
	T_EventHandlersMap subscribers_;

	bool  svr_exit_;   // ��ʾ�̳߳ش򿪹رպ�����״̬

	//����̴߳����������������¼� �������ö�����Ͷ��
	//����ӿ�NetworkInterface �����Ӧ�¼����л�����������Ӧ������message�����ͻ���  
	static std::queue<iEvent*> response_events;
	static pthread_mutex_t queue_mutex;                   //�����̶߳����õ������������Ҫͬ������,ר�����ڸ���������
	static std::vector<iEvent*> rsps;
//		 static NetworkInterface* NTIF_;            //��ΪҪ����̬����svcʹ��, ���Դ˴��þ�̬��Ա����
};













#endif // ! BRKS_SERVICE_DISPATCH_EVENT_SERVICE_H_

