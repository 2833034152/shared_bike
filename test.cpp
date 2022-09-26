#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include "bike.pb.h"
#include "event.h"
#include "events_def.h"

#include "user_event_handler.h"
#include "DispatchMsgService.h"

#include "NetworkInterface.h"

#include "iniconfig.h"
#include "Logger.h"
#include "sqlconnection.h"
#include "BusProcessor.h"
#include <memory>
using namespace std;



int main(int argc, char ** argv) {
	if (argc != 3) {
		printf("Please input  sbk <config file path> <log file config>!\n");
		return -1;
	}

	if (!Logger::instance()->init(std::string(argv[2])))
	{
		fprintf(stderr, "init log module failed.\n");
		return  -2;
	}
//	Iniconfig config;  Iniconfig 采用了单例模式就去掉了
	Iniconfig* config = Iniconfig::getInstance();
	if (!config->loadfile(std::string(argv[1])))
	{
		//printf("load  %s file failed\n" , argv[1]);
		LOG_ERROR("load %s failed.", argv[1]);
		Logger::instance()->GetHandle()->error("load %s failed.", argv[1]);
		return -3;
	}

	st_env_config conf_args = config->getconfig();
	LOG_INFO("[database] ip:%s, port:%d, user:%s, pwd:%s, db:%s, [server] port:%d\n", conf_args.db_ip.c_str(), conf_args.db_port,
		conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), conf_args.db_name.c_str(), conf_args.svr_port);

/*
   //测试日志注释掉了
	cout << "hello" << endl;

	tutorial::mobile_request msg;
	
	msg.set_mobile("15831677178");
	cout << msg.mobile() << endl;

	//测试MobileCodeReqEv
//	iEvent* ie = new iEvent(EVENTID_GET_MOBILE_CODE_REQ, 1);
	MobileCodeReqEv me("15831677178");
	me.dump(cout);

	//测试MobileCodeRspEv
	MobileCodeRspEv mcre(200, 666666);  //错误码   验证码
	mcre.dump(cout);
*/


	std::shared_ptr<MysqlConnection>mysqlconn(new MysqlConnection());
	if (!mysqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port, conf_args.db_user.c_str(),
		conf_args.db_pwd.c_str(), conf_args.db_name.c_str())) {
		LOG_ERROR("Database init failed. exit!\n");
		return -4;
	}

	BusinessProcessor busPro(mysqlconn);  //此时可以注释掉UserEventHandler us了！
	busPro.init();                          //初始化表格，没有就创建

	//测试user_event_handler
//	UserEventHandler us;
//	us.handle(&me);

	//测试DispatchMsgService
	DispatchMsgService* DMS = DispatchMsgService::getInstance();
	DMS->open();

	NetworkInterface* NTIF = new NetworkInterface();
	NTIF->start(8888);

//	MobileCodeReqEv* pmcre = new MobileCodeReqEv("158");
//	DMS->enqueue(pmcre);

	while (1 == 1) {
		NTIF->network_event_dispatch();
		sleep(1);
		LOG_DEBUG("network_event_dispatch ...\n");
	}
	sleep(2);
	DMS->close();
	sleep(2);


	return 0;

}