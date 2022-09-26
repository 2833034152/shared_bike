#ifndef  NS_EVENT_H
#define  NS_EVENT_H
#include "glo_def.h"
#include "eventtype.h"
#include <string>
#include <iostream>

class iEvent
{
public:
	iEvent(u32 eid, u32 sn);     //事件ID  和序列号(给请求事件编个号)

	virtual ~iEvent();

	virtual std::ostream& dump(std::ostream& out) const   //为了方便测试输出
	{
		return out;
	}

	virtual i32 ByteSize() { return 0; };              //方便子类获取信息序列化的长度
	virtual bool SerializeToArray(char* buf, int len) { return true; };  //序列化到一个数组


	u32 generateSeqNo();

	u32 get_eid()  const
	{
		return eid_;
	};

	u32 get_sn() const
	{
		return sn_;
	}

	void set_eid(u32 eid)
	{	
		eid_ = eid;
	}
	void* set_args(void* args) { args_ = args; };
	void* get_args() { return args_; };
private:
	u32 eid_;
	u32 sn_;
	void* args_;
};

#endif

