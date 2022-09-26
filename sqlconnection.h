#ifndef SQLCONNECTION_H_
#define SQLCONNECTION_H_

#include <mysql/mysql.h>
#include <string>
#include <mysql/errmsg.h>
#include <assert.h>

#include "glo_def.h"

class SqlRecordSet {
public:
	SqlRecordSet() : m_pRes(NULL) {

	}

	explicit SqlRecordSet(MYSQL_RES* pRes)  {
		m_pRes = pRes;
	}

	MYSQL_RES* MysqlRes() {
		return m_pRes;
	}

	~SqlRecordSet() {
		if (m_pRes) {
			mysql_free_result(m_pRes);
		}
	}

	//���ý����
	inline void SetResult(MYSQL_RES* pRes) {
		assert(m_pRes == NULL);  //�����ʱ�Ѿ������˽��������ôӦ���ó��򱨴���ֹ�ڴ�й©

		if (m_pRes) {
			LOG_WARN("the MYSQL_RES has already stored result,maybe will cause memory leak");

		}
		m_pRes = pRes;
	}
	inline MYSQL_RES* GetResult() {
		return m_pRes;

	}

	void FetchRow(MYSQL_ROW& row) {
		row = mysql_fetch_row(m_pRes);
	}

	inline i32 GetRowCount() {
		return m_pRes->row_count;
	}
private:
	MYSQL_RES* m_pRes;
};

class MysqlConnection {
public:
	MysqlConnection();
	~MysqlConnection();

	MYSQL* Mysql() {
		return mysql_;
	}
	bool Init(const char* szHost, int nPort, const char* szUser, const char* szPasswd, const char* szDb);

	bool Execute(const char* szSql);

	bool Execute(const char* szSql, SqlRecordSet& recordSet);

	int EscapeString(const char* pSrc, int nSrcLen, char* pDest);
	void Close();

	const char* GetErrInfo();

	void Reconnect();


private:
	MYSQL* mysql_;
};
#endif