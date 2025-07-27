#pragma once
#include "Idb.h"
#include "rd/rd.h"
#include <algorithm>
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>

using namespace std;
using namespace soci;
class DbBase
{
public:
	std::mutex mutex_;
	/***
	* 单例使用下面对应创建和销毁方法
	***/
	static DbBase* getDbInstance(int dbType, string dbhost, string dbuser, string dbpwd, string dbname, int port,int poolnums) {
		if (dbInstance_ == NULL) {
			dbInstance_ = new DbBase(dbType, dbhost.c_str(), dbuser.c_str(), dbpwd.c_str(), dbname.c_str(), port, poolnums);
			return dbInstance_;
		}
		else {
			return dbInstance_;
		}
	}
	//**必须保证之前已经初始化才能调用；
	static DbBase* getDbInstance() {
			return dbInstance_;
	}

	static void closeDbInstance() {
		if (dbInstance_ != NULL) {
			delete dbInstance_;
			dbInstance_ = NULL;
		}
	}

public:
	/***
	* 非单例使用下面对应创建和销毁方法
	***/
	DbBase(int dbType, string dbhost, string dbuser, string dbpwd, string dbname, int port, int maxconn) {
		std::lock_guard<std::mutex> mylockguard(mutex_);
		if (db_ == NULL) {
			dbType_ = dbType;
			dbhost_ = dbhost;
			dbuser_ = dbuser;
			dbpwd_ = dbpwd;
			dbname_ = dbname;
			port_ = port;
			db_ = new Rd(dbType, dbhost.c_str(), dbuser.c_str(), dbpwd.c_str(), dbname.c_str(), port, maxconn);
			db_->init();
		}
	}

	~DbBase() {
		if (db_) {
			delete db_;
			db_ = NULL;
		}
	};
	void init()
	{
		db_->init();
	}
public:

	soci::rowset<soci::row> select(string sql) {
		return db_->select(sql);
	};

	//soci::rowset<soci::row> select(string sql, session* session_ptr) {
	//	return db_->select(sql, session_ptr);
	//};

	int excuteSql(string sql) {
		return db_->excuteSql(sql);
	};

	//soci::session* getSession(session* session_ptr)
	//{
	//	return db_->getSession(session_ptr);
	//}; 

private:
	static DbBase* dbInstance_;
	Idb* db_ = NULL;

public:
	int dbType_;   //0=sqlite,1=mysql,2=orace,3=pg,4=td
	string dbhost_;
	string dbuser_;
	string dbpwd_;
	string dbname_;
	int port_;
};



