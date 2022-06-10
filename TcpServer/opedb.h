#ifndef OPEDB_H
#define OPEDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>

class OpeDB : public QObject
{
    Q_OBJECT
public:
    explicit OpeDB(QObject *parent = nullptr);
    ~OpeDB();

    static OpeDB& getInstance();
    void init();

    bool handleRegist(const char *name, const char *pwd);   //处理注册
    bool handleLogin(const char *name, const char *pwd);    //处理登录
    bool handleCanel(const char *name, const char *pwd);    //处理注销
    void handleOffline(const char *name);   //处理离线
    QStringList handleAllOnline();  //处理所有在线用户
    int handleSearchUser(const char *name); //查找用户
    int handleAddFriend(const char *pername, const char *name);  //加好友
    void handleAgreeAddFriend(const char* pername, const char *name);     //同意好友请求
    QStringList handleFlushFriend(const char* name);    //刷新好友列表
    bool handleDelFriend(const char *name, const char *friendName); //删除好友

private:
    QSqlDatabase m_db;  //Connecting to a Database

signals:

public slots:
};

#endif // OPEDB_H
