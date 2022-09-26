#ifndef SQLTABLES_H_
#define SOLTALBES_H_

#include <memory>   //共享指针需要包含的头文件
#include "sqlconnection.h"
#include "glo_def.h"
class SqlTables {
public:
	SqlTables(std::shared_ptr<MysqlConnection> sqlConn) : sqlconn_(sqlConn) {

	}
    
	bool createUserInfo() {
		const char* pUserInfoTable = "CREATE TABLE IF NOT EXISTS userinfo( id	int(16)  NOT NULL primary key auto_increment,\
								mobile		varchar(16)		NOT NULL default '13000000000',\
								username	varchar(128)	NOT NULL default '',\
								verify	int(4)	NOT NULL default  0, \
								registertm  timestamp		NOT NULL default CURRENT_TIMESTAMP, \
								money		int(4)			NOT NULL default 0,\
								INDEX		mobile_index(mobile) );";
		printf("bikeinfo: %s\n", pUserInfoTable);
		//
		//
		if (!sqlconn_->Execute(pUserInfoTable)) {
			LOG_ERROR("create table userinfo table failed. error msg: %s", sqlconn_->GetErrInfo());
			return false;
		}
		return true;
	}

	bool createBikeInfo() {
		const char* pBikeInfoTable = "\
								CREATE TABLE IF NOT EXISTS bikeinfo( \
								id			int				NOT NULL primary key auto_increment, devno  int  NOT NULL , \							
								status		tinyint(1)		NOT NULL default 0,	\
								trouble		int				NOT NULL default 0,	\
								tmsg		varchar(256)	NOT NULL default '',\
								latitude	double(10,6)	NOT NULL default 0, \
								longitude	double(10,6)	NOT NULL default 0,	\
								unique(devno)  \
								);";
		printf("bikeinfo: %s\n", pBikeInfoTable);
		if (!sqlconn_->Execute(pBikeInfoTable)) {
			LOG_ERROR("create table bikeinfo table failed. error msg: %s", sqlconn_->GetErrInfo());
			return false;
		}
		return true;
	}
private:
	std::shared_ptr<MysqlConnection> sqlconn_;  //当对象最后一次释放后才真正的释放
};


#endif // !SQLTABLES_H_
