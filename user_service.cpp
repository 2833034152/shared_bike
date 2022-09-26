#include "user_service.h"

UserSerivce::UserSerivce(std::shared_ptr<MysqlConnection> sql_conn):sql_conn_(sql_conn)
{

}

bool UserSerivce::exist(const std::string& mobile)
{
	char sql_content[1024] = { 0 };
	sprintf(sql_content, \
		"select * from userinfo where mobile = %s", \
		mobile.c_str());
	SqlRecordSet record_set;
	if (!sql_conn_->Execute(sql_content, record_set)) {
		return false;
	}
	
	return (record_set.GetRowCount() != 0);  //�ҵ��û��ˣ� �����Ͳ�Ϊ0
}

bool UserSerivce::insert(const std::string& mobile)
{
	char sql_content[1024] = { 0 };
	sprintf(sql_content, \
		"insert into userinfo (mobile) values (%s)", \
		mobile.c_str());
	
	return sql_conn_->Execute(sql_content);  //�ҵ��û��ˣ� �����Ͳ�Ϊ0
}
