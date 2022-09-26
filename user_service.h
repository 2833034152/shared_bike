#ifndef  USERSERIVICE_H_
#define  USERSERVICE_H_

#include <memory>
#include "sqlconnection.h"
class UserSerivce {
public:
	UserSerivce(std::shared_ptr<MysqlConnection> sql_conn);
	bool exist(const std::string& mobile);
	bool insert(const std::string& mobile);

private:
	std::shared_ptr<MysqlConnection> sql_conn_;
};


#endif
