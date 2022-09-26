#ifndef SHBK_COMMON_CONFIGDEG_H_
#define SHBK_COMMON_CONFIGDEG_H_

#include <string>
typedef struct st_env_config{
	
	//数据库的配置
	std::string db_ip;
	unsigned short  db_port;
	std::string db_user;
	std::string db_pwd;
	std::string db_name;
	
	//服务器的配置
	unsigned short svr_port;
	
	st_env_config (){
		
	};
	
	st_env_config(const std::string& db_ip, unsigned short  db_port, const std::string& db_user,\
				  const std::string& db_pwd, const std::string& db_name ,unsigned short svr_port){
					  
	    this->db_ip   = db_ip;
		this->db_port = db_port;
		this->db_user =  db_user;
		this->db_pwd  = db_pwd;
		this->db_name = db_name;
		this->svr_port = svr_port;
					  
	};
				  
	st_env_config& operator = (const st_env_config& config){
		if(this != &config){
			this->db_ip   = config.db_ip;
			this->db_port = config.db_port;
			this->db_user =  config.db_user;
			this->db_pwd  = config.db_pwd;
			this->db_name = config.db_name;
			this->svr_port = config.svr_port;
		}
		return *this;
	}
	
	
	
	
	
	
	
}_st_env_config;







#endif