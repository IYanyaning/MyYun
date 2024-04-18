#include "include/sql_connection_pool.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

connection_pool::connection_pool() {
  this->CurConn = 0;
  this->FreeConn = 0;
}

connection_pool *connection_pool::GetInstance() {
  static connection_pool connPool;
  return &connPool;
}

void connection_pool::init(string url, string User, string PassWord,
                           string DBName, int Port, unsigned int MaxConn) {
  this->url = url;
  this->Port = Port;
  this->User = User;
  this->PassWord = PassWord;
  this->DatabaseName = DBName;
  lock.lock();
  for (int i = 0; i < MaxConn; i++) {
    MYSQL *con = NULL;
    con = mysql_init(con);

    if (con == NULL) {
      cout << "Error:" << mysql_error(con);
      exit(1);
    }

    con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(),
                             DBName.c_str(), Port, NULL, 0);
    if (con == NULL) {
      cout << "Error: " << mysql_error(con);

      exit(1);
    }

    connList.push_back(con);

    ++FreeConn;
  }
  reserve = sem(FreeConn);

  this->MaxConn = FreeConn;

  lock.unlock();
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection() {
  MYSQL *con = NULL;

  if (0 == connList.size()) {
    return NULL;
  }
  reserve.wait();

  lock.lock();

  con = connList.front();
  connList.pop_front();

  --FreeConn;
  ++CurConn;

  lock.unlock();
  return con;
}

//释放当期使用的连接
bool connection_pool::ReleaseConnection(MYSQL *conn) {
  if (NULL == conn) {
    return false;
  }

  lock.lock();

  connList.push_back(conn);
  ++FreeConn;
  --CurConn;
  lock.unlock();
  reserve.post();
  return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool() {
  lock.lock();

  if (connList.size() > 0) {
    for (auto &con : connList) {
      mysql_close(con);
    }

    CurConn = 0;
    FreeConn = 0;
    connList.clear();
    lock.unlock();
  }

  lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn() { return this->FreeConn; }

connection_pool::~connection_pool() { DestroyPool(); }

connectionRAII::connectionRAII(MYSQL **con, connection_pool *connPool) {
  *con = connPool->GetConnection();
  conRAII = *con;
  poolRAII = connPool;
}

connectionRAII::~connectionRAII() { poolRAII->ReleaseConnection(conRAII); }
