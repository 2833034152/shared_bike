#ifndef BUSPROCESSOR_H_
#define BUFPROCESSOR_H_

#include <memory>
#include "user_event_handler.h"
#include "sqlconnection.h"
class BusinessProcessor {
public:
	BusinessProcessor(std::shared_ptr<MysqlConnection> conn);

	bool init();

	virtual ~BusinessProcessor();

private:
	std::shared_ptr<MysqlConnection> mysqlconn_;
	std::shared_ptr<UserEventHandler> ueh_;
};




#endif