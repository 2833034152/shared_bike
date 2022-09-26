#include "BusProcessor.h"

#include "SqlTables.h"
#include "sqlconnection.h"

BusinessProcessor::BusinessProcessor(std::shared_ptr<MysqlConnection> conn)
	:mysqlconn_(conn) ,ueh_(new UserEventHandler())
{
}

bool BusinessProcessor::init()
{
	SqlTables tables(mysqlconn_);
	tables.createUserInfo();
	tables.createBikeInfo();
	return true;
}

BusinessProcessor::~BusinessProcessor()
{
	ueh_.reset();
}
