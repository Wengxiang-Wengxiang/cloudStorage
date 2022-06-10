#ifndef FRIEND_H
#define FRIEND_H

#include <QWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "online.h"
#include "privatechat.h"

class Friend : public QWidget
{
    Q_OBJECT
public:
    explicit Friend(QWidget *parent = nullptr);

    void showAllOnlineUser(PDU *pdu);
    void updateFriendList(PDU *pdu);
    void updateGroupMsg(PDU *pdu);

    QString m_strSearchName;
    QListWidget *getFriendList();

private:
    QTextEdit   *m_pShowMsgTE;      //信息显示框
    QListWidget *m_pFriendListWidget;//好友列表
    QLineEdit   *m_pInputMsgLE;     //信息输入框

    QPushButton *m_pDelFriendPB;    //删除好友
    QPushButton *m_FlushFriendPB;   //刷新好友列表
    QPushButton *m_pShowOnlineUsrPB;//显示在线用户
    QPushButton *m_pSearchUsrPB;    //查找用户
    QPushButton *m_pMsgSendPB;      //信息发送
    QPushButton *m_pPrivateChatPB;  //私聊

    Online *m_pOnline;  //在线用户窗口对象


signals:

public slots:
    void showOnline();  //显示在线用户窗口
    void searchUser();  //查找用户
    void flushFriend(); //刷新好友列表
    void delFriend();   //删除好友
    void privateChat(); //私聊
    void groupChat();   //群聊
};

#endif // FRIEND_H
