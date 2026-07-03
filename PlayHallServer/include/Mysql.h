#ifndef _MYSQL_H
#define _MYSQL_H
#include "packdef.h"
#include <mysql/mysql.h>
#include <list>
#include <string>
#include <vector>

using namespace  std;

// 预编译语句参数（与 SQL 中 ? 占位符一一对应，按顺序绑定）
struct SqlParam
{
    enum Type { String, Int, Int64, Null } type;
    string strVal;       // String 类型时持有副本，保证 bind 期间内存有效
    int intVal;
    long long int64Val;

    static SqlParam FromString(const char* s);
    static SqlParam FromString(const string& s);
    static SqlParam FromInt(int v);
    static SqlParam FromInt64(long long v);
    static SqlParam FromNull();
};

class CMysql{
public:
    int ConnectMysql(const char *server, const char *user, const char *password, const char *database);
    int SelectMysql(char* szSql,int nColumn,list<string>& lst);
    int UpdataMysql(char *szsql);
    void DisConnect();

    // 预编译查询：sql 中用 ? 作占位符，params 与 ? 顺序一致
    // nColumn：每行期望的列数，结果按行展开写入 lst（与 SelectMysql 相同）
    int SelectPrepared(const char* sql, const vector<SqlParam>& params, int nColumn, list<string>& lst);
    // 预编译写操作：INSERT / UPDATE / DELETE
    int ExecutePrepared(const char* sql, const vector<SqlParam>& params);

private:
    bool BindParams(MYSQL_STMT* stmt, const vector<SqlParam>& params,
                    vector<MYSQL_BIND>& binds, vector<unsigned long>& strLengths);

    MYSQL *conn;
    pthread_mutex_t m_lock;
};

#endif
