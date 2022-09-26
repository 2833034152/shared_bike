#include "user_event_handler.h"
#include "DispatchMsgService.h"


//#include <arpa/inet.h>
#include <assert.h>>
#include <errno.h>
//#include <netinet/in.h>
//#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/wait.h>
//#include <netdb.h>
//#include <unistd.h>

#include "threadpool/thread.h"

#include "iniconfig.h"
#include "user_service.h"
UserEventHandler::UserEventHandler() :iEventHandler("UserEventHandler")
{
	//未来等dispatch类实现后再实现订阅着模式
	DispatchMsgService::getInstance()->subscribe(EVENTID_GET_MOBILE_CODE_REQ, this);
	DispatchMsgService::getInstance()->subscribe(EVENTID_LOIGIN_REQ, this);
	DispatchMsgService::getInstance()->subscribe(EVENTID_RECHARGE_REQ, this);
	DispatchMsgService::getInstance()->subscribe(EVENTID_GET_ACCOUNT_BALANCE_REQ, this);
	DispatchMsgService::getInstance()->subscribe(EVENTID_LIST_ACCOUNT_RECORDS_REQ, this);

	thread_mutex_create(&pm_);

}


UserEventHandler::~UserEventHandler()
{
	DispatchMsgService::getInstance()->unsubscribe(EVENTID_GET_MOBILE_CODE_REQ, this);
	DispatchMsgService::getInstance()->unsubscribe(EVENTID_LOIGIN_REQ, this);
	DispatchMsgService::getInstance()->unsubscribe(EVENTID_RECHARGE_REQ, this);
	DispatchMsgService::getInstance()->unsubscribe(EVENTID_GET_ACCOUNT_BALANCE_REQ, this);
	DispatchMsgService::getInstance()->unsubscribe(EVENTID_LIST_ACCOUNT_RECORDS_REQ, this);
	thread_mutex_destroy(&pm_);
}

iEvent* UserEventHandler::handle(const iEvent* ev)
{
	if (ev == NULL)
	{
		printf("input ev is NULL");
		//以后用日志
	}

	u32 eid = ev->get_eid();
	if (eid == EVENTID_GET_MOBILE_CODE_REQ)
	{
		return handle_mobile_code_req((MobileCodeReqEv*)ev);
	}

	if (eid == EVENTID_LOIGIN_REQ)
	{
		return handel_login_req((LoginReqEv*)ev);
	}

	if (eid == EVENTID_RECHARGE_REQ)
	{

	}

	if (eid == EVENTID_GET_ACCOUNT_BALANCE_REQ)
	{

	}
	if (eid == EVENTID_LIST_ACCOUNT_RECORDS_REQ)
	{

	}

	return NULL;
}


MobileCodeRspEv* UserEventHandler::handle_mobile_code_req(MobileCodeReqEv* ev) {
	i32 icode = 0;  // 临时变量 ,用来存放m2c_[mobile] ,防止每次都从这个数组中查找
	std::string mobile_ = ev->get_mobile();

	icode = code_gen();    //产生验证码


	printf("try to get mobile phone %s validate code (根据手机号产生验证码):\n", mobile_.c_str());

	thread_mutex_lock(&pm_);
	m2c_[mobile_] = code_gen();
	thread_mutex_unlock(&pm_);

	printf("mobile: %s, code: %d\n\n", mobile_.c_str(), icode);

	return new MobileCodeRspEv(200, icode);    //产生验证码之后，产生响应事件
}

LoginResEv* UserEventHandler::handel_login_req(LoginReqEv* ev) {

	LoginResEv* loginEv = nullptr;
	std::string mobile = ev->get_mobile();
	i32 code = ev->get_icode();

	LOG_DEBUG("try to handle login ev , mobile: %s , code: %d\n", mobile.c_str(), code);

	thread_mutex_lock(&pm_);
	auto iter = m2c_.find(mobile);
	if ( ((iter != m2c_.end()) && (code != iter->second)) 
		|| (iter == m2c_.end())) {
		loginEv =  new LoginResEv(ERRC_INVAID_DATA);
	}
//	m2c_[mobile_] = code_gen();  不需要
	thread_mutex_unlock(&pm_);
	if(loginEv) return loginEv;

	//如果验证成功，判断用户是否在数据库存在，不存在则插入用户记录
	std::shared_ptr<MysqlConnection> mysqlconn(new MysqlConnection);
	st_env_config conf_args = Iniconfig::getInstance()->getconfig();
	if (!mysqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port, conf_args.db_user.c_str(),
		conf_args.db_pwd.c_str(), conf_args.db_name.c_str())) {
		LOG_ERROR("Database init failed. exit!\n");
		return new LoginResEv(ERRO_PROCESS_FAILED);
	}
	UserSerivce us(mysqlconn);
	bool result = false;
	if (!us.exist(mobile)) {
		result = us.insert(mobile);
	}
	if (!result) {
		LOG_ERROR("insert user(%s) to db failed.", mobile.c_str());
		return new LoginResEv(ERRO_PROCESS_FAILED);

	}
	return new LoginResEv(ERRC_SUCCESS);

}
i32 UserEventHandler::code_gen() {
	i32 code = 0;
	srand((unsigned int)time(NULL));

	code = (unsigned int)(rand() % (999999 - 100000) + 1000000);

	return code;
}