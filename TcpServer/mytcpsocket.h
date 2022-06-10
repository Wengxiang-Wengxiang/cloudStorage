#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H

#include <QTcpSocket>
#include "protocol.h"
#include "opedb.h"
#include <QDir>
#include <QFile>
#include <QTimer>

class MyTcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    MyTcpSocket();
    QString getName();
    void copyDir(QString strSrcDir, QString strDestDir);

private:
    QString m_strName;

    QFile m_file;
    qint64 m_iTotal;    //文件总大小
    qint64 m_iRecved;   //文件接收了多少
    bool m_bUpload;

    QTimer *m_pTimer;

signals:
    void offlien(MyTcpSocket *mysocket);

public slots:
    void recvMsg();
    void clientOffline();
    void sendFileToClinet();
};

#endif // MYTCPSOCKET_H
