#ifndef BRKS_BUS_USERM_HANDLER_H_
#define BRKS_BUS_USERM_HANDLER_H_

#include "glo_def.h"
#include "iEventHandler.h"
#include "events_def.h"


class  UserEventHandler :public iEventHandler
{
public:
	UserEventHandler();
	virtual ~UserEventHandler(); 
	virtual iEvent* handle(const iEvent* ev);
private:
	MobileCodeRspEv* handle_mobile_code_req(MobileCodeReqEv* ev);
	LoginResEv* handel_login_req(LoginReqEv* ev);
	i32  code_gen();         //产生验证码

private:
	std::map<std::string, i32> m2c_;    //根据手机号产生验证码
	std::string mobile_;
	pthread_mutex_t  pm_;
};


#endif