#include "Mysql.h"
#include <cstring>
#include <cstdio>

namespace {
const unsigned long kResultBufSize = 4096;
}

SqlParam SqlParam::FromString(const char* s)
{
    SqlParam p;
    p.type = String;
    p.strVal = (s != nullptr) ? s : "";
    return p;
}

SqlParam SqlParam::FromString(const string& s)
{
    SqlParam p;
    p.type = String;
    p.strVal = s;
    return p;
}

SqlParam SqlParam::FromInt(int v)
{
    SqlParam p;
    p.type = Int;
    p.intVal = v;
    return p;
}

SqlParam SqlParam::FromInt64(long long v)
{
    SqlParam p;
    p.type = Int64;
    p.int64Val = v;
    return p;
}

SqlParam SqlParam::FromNull()
{
    SqlParam p;
    p.type = Null;
    return p;
}

bool CMysql::BindParams(MYSQL_STMT* stmt, const vector<SqlParam>& params,
                        vector<MYSQL_BIND>& binds, vector<unsigned long>& strLengths)
{
    if (params.empty())
        return true;

    binds.assign(params.size(), MYSQL_BIND{});
    strLengths.assign(params.size(), 0);

    for (size_t i = 0; i < params.size(); ++i)
    {
        MYSQL_BIND& bind = binds[i];
        memset(&bind, 0, sizeof(bind));

        switch (params[i].type)
        {
        case SqlParam::String:
            bind.buffer_type = MYSQL_TYPE_STRING;
            bind.buffer = const_cast<char*>(params[i].strVal.c_str());
            strLengths[i] = static_cast<unsigned long>(params[i].strVal.size());
            bind.buffer_length = strLengths[i];
            bind.length = &strLengths[i];
            break;
        case SqlParam::Int:
            bind.buffer_type = MYSQL_TYPE_LONG;
            bind.buffer = const_cast<int*>(&params[i].intVal);
            break;
        case SqlParam::Int64:
            bind.buffer_type = MYSQL_TYPE_LONGLONG;
            bind.buffer = const_cast<long long*>(&params[i].int64Val);
            break;
        case SqlParam::Null:
            bind.buffer_type = MYSQL_TYPE_NULL;
            break;
        }
    }

    if (mysql_stmt_bind_param(stmt, binds.data()) != 0)
    {
        printf("mysql_stmt_bind_param error: %s\n", mysql_stmt_error(stmt));
        return false;
    }
    return true;
}

int CMysql::ConnectMysql(const char* server,const char* user,const char* password, const char* database)
{
    conn = mysql_init(NULL);
    mysql_set_character_set(conn,"utf8");
    if(!mysql_real_connect(conn,server,user,password,database,0,NULL,0))
    {
        return FALSE;
    }
    pthread_mutex_init(&m_lock , NULL);
    return TRUE;
}

int CMysql::SelectMysql(char* szSql,int nColumn,list<string>& lst)
{
    MYSQL_RES * results = NULL;

    pthread_mutex_lock(&m_lock );

    if(mysql_query(conn,szSql)) {
        pthread_mutex_unlock(&m_lock );
        return FALSE;
    }
    results = mysql_store_result(conn);
    pthread_mutex_unlock(&m_lock );

    if(NULL == results)return FALSE;
    MYSQL_ROW record;
    while((record = mysql_fetch_row(results)))
    {
        for(int i=0; i<nColumn; i++)
        {
            lst.push_back( record[i] );
        }
    }
    mysql_free_result(results);
    return TRUE;
}

int CMysql::UpdataMysql(char *szsql)
{
    if(!szsql)return FALSE;
    pthread_mutex_lock(&m_lock );
    if(mysql_query(conn,szsql)){
        pthread_mutex_unlock(&m_lock );
        return FALSE;
    }
    pthread_mutex_unlock(&m_lock );
    return TRUE;
}

int CMysql::SelectPrepared(const char* sql, const vector<SqlParam>& params,
                           int nColumn, list<string>& lst)
{
    if (!conn || !sql || nColumn <= 0)
        return FALSE;

    pthread_mutex_lock(&m_lock);

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt)
    {
        printf("mysql_stmt_init error: %s\n", mysql_error(conn));
        pthread_mutex_unlock(&m_lock);
        return FALSE;
    }

    int ok = FALSE;
    unsigned long sqlLen = static_cast<unsigned long>(strlen(sql));

    if (mysql_stmt_prepare(stmt, sql, sqlLen) != 0)
    {
        printf("mysql_stmt_prepare error: %s\n", mysql_stmt_error(stmt));
        goto cleanup;
    }

    {
        vector<MYSQL_BIND> paramBinds;
        vector<unsigned long> strLengths;
        if (!BindParams(stmt, params, paramBinds, strLengths))
            goto cleanup;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        printf("mysql_stmt_execute error: %s\n", mysql_stmt_error(stmt));
        goto cleanup;
    }

    if (mysql_stmt_store_result(stmt) != 0)
    {
        printf("mysql_stmt_store_result error: %s\n", mysql_stmt_error(stmt));
        goto cleanup;
    }

    {
        vector<MYSQL_BIND> resultBinds(nColumn);
        vector<vector<char>> resultBufs(nColumn, vector<char>(kResultBufSize));
        vector<unsigned long> resultLengths(nColumn, 0);
        vector<my_bool> isNull(nColumn, 0);

        for (int i = 0; i < nColumn; ++i)
        {
            memset(&resultBinds[i], 0, sizeof(MYSQL_BIND));
            resultBinds[i].buffer_type = MYSQL_TYPE_STRING;
            resultBinds[i].buffer = resultBufs[i].data();
            resultBinds[i].buffer_length = kResultBufSize;
            resultBinds[i].length = &resultLengths[i];
            resultBinds[i].is_null = &isNull[i];
        }

        if (mysql_stmt_bind_result(stmt, resultBinds.data()) != 0)
        {
            printf("mysql_stmt_bind_result error: %s\n", mysql_stmt_error(stmt));
            goto cleanup;
        }

        int fetchStatus = 0;
        while ((fetchStatus = mysql_stmt_fetch(stmt)) == 0)
        {
            for (int i = 0; i < nColumn; ++i)
            {
                if (isNull[i])
                    lst.push_back("");
                else
                    lst.push_back(string(resultBufs[i].data(), resultLengths[i]));
            }
        }

        if (fetchStatus != MYSQL_NO_DATA && fetchStatus != MYSQL_DATA_TRUNCATED)
        {
            printf("mysql_stmt_fetch error: %s\n", mysql_stmt_error(stmt));
            goto cleanup;
        }
    }

    ok = TRUE;

cleanup:
    mysql_stmt_close(stmt);
    pthread_mutex_unlock(&m_lock);
    return ok;
}

int CMysql::ExecutePrepared(const char* sql, const vector<SqlParam>& params)
{
    if (!conn || !sql)
        return FALSE;

    pthread_mutex_lock(&m_lock);

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt)
    {
        printf("mysql_stmt_init error: %s\n", mysql_error(conn));
        pthread_mutex_unlock(&m_lock);
        return FALSE;
    }

    int ok = FALSE;
    unsigned long sqlLen = static_cast<unsigned long>(strlen(sql));

    if (mysql_stmt_prepare(stmt, sql, sqlLen) != 0)
    {
        printf("mysql_stmt_prepare error: %s\n", mysql_stmt_error(stmt));
        goto cleanup;
    }

    {
        vector<MYSQL_BIND> paramBinds;
        vector<unsigned long> strLengths;
        if (!BindParams(stmt, params, paramBinds, strLengths))
            goto cleanup;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        printf("mysql_stmt_execute error: %s\n", mysql_stmt_error(stmt));
        goto cleanup;
    }

    ok = TRUE;

cleanup:
    mysql_stmt_close(stmt);
    pthread_mutex_unlock(&m_lock);
    return ok;
}

void CMysql::DisConnect()
{
    mysql_close(conn);
}
