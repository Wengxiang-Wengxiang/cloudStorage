#include "mytcpsocket.h"
#include <QDebug>
#include "mytcpserver.h"
#include <QDir>
#include <QFileInfoList>
#include <cstdio>

MyTcpSocket::MyTcpSocket()
{
    m_bUpload = false;
    m_pTimer = new QTimer;

    connect(this, SIGNAL(readyRead()), this, SLOT(recvMsg()));
    connect(this, SIGNAL(disconnected()), this, SLOT(clientOffline()));
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(sendFileToClinet()));
}

QString MyTcpSocket::getName()
{
    return m_strName;
}

void MyTcpSocket::copyDir(QString strSrcDir, QString strDestDir)
{
    QDir dir;
    dir.mkdir(strDestDir);

    dir.setPath(strSrcDir);
    QFileInfoList fileInfoList = dir.entryInfoList();

    QString srcTmp;
    QString destTmp;
    for (int i = 0; i < fileInfoList.size(); ++i)
    {
        if (fileInfoList[i].isFile())
        {
            srcTmp = strSrcDir+ '/' +fileInfoList[i].fileName();
            destTmp = strDestDir+ '/' +fileInfoList[i].fileName();
            QFile::copy(srcTmp, destTmp);
        }
        else if (fileInfoList[i].isDir())
        {
            if (QString('.') == fileInfoList[i].fileName() || QString('..') == fileInfoList[i].fileName())
            {
                continue;
            }
            //文件夹采用递归copy
            srcTmp = strSrcDir+ '/' +fileInfoList[i].fileName();
            destTmp = strDestDir+ '/' +fileInfoList[i].fileName();
            copyDir(srcTmp, destTmp);
        }
    }
}

void MyTcpSocket::recvMsg()
{
    if (!m_bUpload)
    {
        qDebug() << this->bytesAvailable();
        u_int uiPDULen = 0;
        this->read((char*)&uiPDULen, sizeof(u_int));
        u_int uiMsgLen = uiPDULen - sizeof(PDU);
        PDU *pdu = mkPDU(uiMsgLen);
        this->read((char*)pdu + sizeof(u_int), uiPDULen - sizeof(u_int));

    //    qDebug() << pdu->uiMsgType << pdu->caMsg;
        switch (pdu->uiMsgType)
        {
        //注册请求
        case ENUM_MSG_TYPE_REGIST_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            bool ret = OpeDB::getInstance().handleRegist(caName, caPwd);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_REGIST_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, REGIST_OK);

                //注册成功后给用户创建根目录
                QDir dir;
                qDebug() << "create dir : " << dir.mkdir(QString("./%1").arg(caName));
            }
            else
            {
                strcpy(respdu->caData, REGIST_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //登录请求
        case ENUM_MSG_TYPE_LOGIN_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            bool ret = OpeDB::getInstance().handleLogin(caName, caPwd);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_LOGIN_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, LOGIN_OK);
                strcpy(respdu->caData + 32, caName);
                m_strName = caName;
            }
            else
            {
                strcpy(respdu->caData, LOGIN_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //注销账号请求
        case ENUM_MSG_TYPE_CANCEL_REQUEST:
        {
            char caName[32] = {'\0'};
            char caPwd[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            strncpy(caPwd, pdu->caData + 32, 32);
            bool ret = OpeDB::getInstance().handleCanel(caName, caPwd);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_CANCEL_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, CANCEL_OK);
                //注销成功后删除用户根目录
                QDir dir(QString("./%1").arg(caName));
                qDebug() << "delete dir : " << dir.removeRecursively();
            }
            else
            {
                strcpy(respdu->caData, CANCEL_FAILED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //在线用户请求
        case ENUM_MSG_TYPE_ALL_ONLINE_REQUEST:
        {
            QStringList ret = OpeDB::getInstance().handleAllOnline();
            u_int uiMsgLen = ret.size() * 32;
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_ALL_ONLINE_RESPOND;
            for (int i = 0; i < ret.size(); ++i)
            {
                memcpy((char*)(respdu->caMsg) + i*32,
                       ret.at(i).toStdString().c_str(),
                       ret.at(i).size());
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //搜素用户请求
        case ENUM_MSG_TYPE_SEARCH_USR_REQUEST:
        {
            int ret = OpeDB::getInstance().handleSearchUser(pdu->caData);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SEARCH_USR_RESPOND;
            if (-1 == ret)
            {
                strcpy(respdu->caData, SEARCH_USER_NO);
            }
            else if (0 == ret)
            {
                strcpy(respdu->caData, SEARCH_USER_OFFLINE);
            }
            else if (1 == ret)
            {
                strcpy(respdu->caData, SEARCH_USER_ONLINE);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //加好友请求
        case ENUM_MSG_TYPE_ADD_FRIEND_REQUEST:
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};
            strncpy(caPerName, pdu->caData, 32);
            strncpy(caName, pdu->caData + 32, 32);
            int ret = OpeDB::getInstance().handleAddFriend(caPerName, caName);

            PDU *respdu = NULL;
            if (-1 == ret)
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, UNKNOW_ERROR);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else if (0 == ret)  //已经是好友
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, EXISTED_FRIEND);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else if (1 == ret)  //在线
            {
                MyTcpServer::getInstance().resend(caPerName, pdu);
            }
            else if (2 == ret)  //不在线
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, ADD_FRIEND_OFFLINE);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }
            else if (3 == ret)  //不存在
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_RESPOND;
                strcpy(respdu->caData, ADD_FRIEND_NO_EXIST);
                write((char*)respdu, respdu->uiPDULen);
                free(respdu);
                respdu = NULL;
            }

            break;
        }

        //好友请求同意与否
        case ENUM_MSG_TYPE_ADD_FRIEND_AGGREE:
        {
            char caPerName[32] = {'\0'};
            char caName[32] = {'\0'};
            strncpy(caPerName, pdu->caData, 32);
            strncpy(caName, pdu->caData+32, 32);
            OpeDB::getInstance().handleAgreeAddFriend(caPerName, caName);

            MyTcpServer::getInstance().resend(caName, pdu);

            break;
        }
        case ENUM_MSG_TYPE_ADD_FRIEND_REFUSE:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData+32, 32);
            MyTcpServer::getInstance().resend(caName, pdu);

            break;
        }

        //刷新好友请求
        case ENUM_MSG_TYPE_FLUSH_FRIEND_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            QStringList ret = OpeDB::getInstance().handleFlushFriend(caName);
            u_int uiMsgLen = ret.size() * 32;
            PDU *respdu = mkPDU(uiMsgLen);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FRIEND_RESPOND;
            for (int i = 0; i < ret.size(); ++i)
            {
                memcpy((char*)(respdu->caMsg) + i*32,
                       ret.at(i).toStdString().c_str(),
                       ret.at(i).size());
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //删除好友请求
        case ENUM_MSG_TYPE_DELETE_FRIEND_REQUEST:
        {
            char caSelfName[32] = {'\0'};
            char caFriendName[32] = {'\0'};
            strncpy(caSelfName, pdu->caData, 32);
            strncpy(caFriendName, pdu->caData + 32, 32);
            OpeDB::getInstance().handleDelFriend(caSelfName, caFriendName);

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DELETE_FRIEND_RESPOND;
            strcpy(respdu->caData, DEL_FRIEND_OK);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            MyTcpServer::getInstance().resend(caFriendName, pdu);

            break;
        }

        //私聊请求
        case ENUM_MSG_TYPE_PRIVATE_CHAT_REQUEST:
        {
            char caPerName[32] = {'\0'};
            strncpy(caPerName, pdu->caData+32, 32);

            MyTcpServer::getInstance().resend(caPerName, pdu);

            break;
        }

        //群聊请求
        case ENUM_MSG_TYPE_GROUP_CHAT_REQUEST:
        {
            char caName[32] = {'\0'};
            strncpy(caName, pdu->caData, 32);
            QStringList onlineFriend = OpeDB::getInstance().handleFlushFriend(caName);

            QString tmpName;
            for (int i = 0; i < onlineFriend.size(); ++i)
            {
                tmpName = onlineFriend.at(i);
                MyTcpServer::getInstance().resend(tmpName.toStdString().c_str(), pdu);
            }

            break;
        }

        //创建文件夹请求
        case ENUM_MSG_TYPE_CREATE_DIR_REQUEST:
        {
            QDir dir;
            QString strCurPath = QString("%1").arg(pdu->caMsg);
            qDebug() << strCurPath;

            //判断当前目录是否存在
            bool ret = dir.exists(strCurPath);
            PDU *respdu = NULL;
            if (ret)
            {
                char caNewDir[32] = {'\0'};
                strncpy(caNewDir, pdu->caData + 32, 32);
                QString strNewPath = strCurPath + "/" +caNewDir;
                qDebug() << strNewPath;

                //判断新建文件夹是否存在
                ret = dir.exists(strNewPath);
                if (ret)
                {
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData, FILE_NAME_EXIST);
                }
                else
                {
                    dir.mkdir(strNewPath);
                    respdu = mkPDU(0);
                    respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                    strcpy(respdu->caData, CREAT_DIR_OK);
                }
            }
            else
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_RESPOND;
                strcpy(respdu->caData, DIR_NO_EXIST);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //刷新文件请求
        case ENUM_MSG_TYPE_FLUSH_FILE_REQUEST:
        {
            char *pCurPath = new char[pdu->uiMsgLen];
            strncpy(pCurPath, pdu->caMsg, pdu->uiMsgLen);
            QDir dir(pCurPath);
            QFileInfoList fileInfoList = dir.entryInfoList();
            int iFileCount = fileInfoList.size();
            PDU *respdu = mkPDU(sizeof(FileInfo) * iFileCount);
            respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;

            FileInfo *pFileInfo = NULL;
            QString strFileName;
            for (int i = 0; i < iFileCount; ++i)
            {
                pFileInfo = (FileInfo*)(respdu->caMsg) + i;
                strFileName = fileInfoList[i].fileName();
                strncpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                if (fileInfoList[i].isDir())
                {
                    pFileInfo->uiFileType = 0;
                }
                else if (fileInfoList[i].isFile())
                {
                    pFileInfo->uiFileType = 1;
                }
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //删除目录请求
        case ENUM_MSG_TYPE_DEL_DIR_REQUEST:
        {
            char caName[32] = {'\0'};
            strcpy(caName, pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);
    //        qDebug() << strPath;

            QFileInfo fileInfo(strPath);
            bool ret = false;
            if (fileInfo.isDir())
            {
                QDir dir;
                dir.setPath(strPath);
                ret = dir.removeRecursively();
            }
            else if (fileInfo.isFile()) //常规文件不适用目录删除功能删除
            {
                ret = false;
            }

            PDU *respdu = NULL;
            if (ret)
            {
                respdu = mkPDU(strlen(DEL_DIR_OK) + 1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                strncpy(respdu->caData, DEL_DIR_OK, strlen(DEL_DIR_OK));
            }
            else
            {
                respdu = mkPDU(strlen(DEL_DIR_FAILURED) + 1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_RESPOND;
                strncpy(respdu->caData, DEL_DIR_FAILURED, strlen(DEL_DIR_FAILURED));
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //重命名请求
        case ENUM_MSG_TYPE_RENAME_FILE_REQUEST:
        {
            char caOldName[32] = {'\0'};
            char caNewName[32] = {'\0'};
            strncpy(caOldName, pdu->caData, 32);
            strncpy(caNewName, pdu->caData + 32, 32);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);

            QString strOldPath = QString("%1/%2").arg(pPath).arg(caOldName);
            QString strNewPath = QString("%1/%2").arg(pPath).arg(caNewName);
    //        qDebug() << strOldPath << "\n" << strNewPath;

            QDir dir;
            bool ret =  dir.rename(strOldPath, strNewPath);
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_RESPOND;
            if (ret)
            {
                strcpy(respdu->caData, RENAME_FILE_OK);
            }
            else
            {
                strcpy(respdu->caData, RENAME_FILE_FAILURED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //进入文件夹请求
        case ENUM_MSG_TYPE_ENTER_DIR_REQUEST:
        {
            char caEnterName[32] = {'\0'};
            strncpy(caEnterName, pdu->caData, 32);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caEnterName);

            QFileInfo fileInfo(strPath);
            PDU *respdu = NULL;
            if (fileInfo.isDir())
            {
                QDir dir(strPath);
                QFileInfoList fileInfoList = dir.entryInfoList();
                int iFileCount = fileInfoList.size();
                respdu = mkPDU(sizeof(FileInfo) * iFileCount);
                //进入文件夹后刷新出新的文件列表
                respdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_RESPOND;

                FileInfo *pFileInfo = NULL;
                QString strFileName;
                for (int i = 0; i < iFileCount; ++i)
                {
                    pFileInfo = (FileInfo*)(respdu->caMsg) + i;
                    strFileName = fileInfoList[i].fileName();
                    strncpy(pFileInfo->caFileName, strFileName.toStdString().c_str(), strFileName.size());
                    if (fileInfoList[i].isDir())
                    {
                        pFileInfo->uiFileType = 0;
                    }
                    else if (fileInfoList[i].isFile())
                    {
                        pFileInfo->uiFileType = 1;
                    }
                }

            }
            else if (fileInfo.isFile())
            {
                respdu = mkPDU(0);
                respdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_RESPOND;
                strcpy(respdu->caData, ENTER_DIR_FAILURED);
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        //删除文件
        case ENUM_MSG_TYPE_DEL_FILE_REQUEST:
        {
            char caName[32] = {'\0'};
            strcpy(caName, pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caName);

            QFileInfo fileInfo(strPath);
            bool ret = false;
            if (fileInfo.isDir())   //目录不适用常规文件删除功能删除
            {
                ret = false;
            }
            else if (fileInfo.isFile())
            {
                QDir dir;
                ret = dir.remove(strPath);
            }

            PDU *respdu = NULL;
            if (ret)
            {
                respdu = mkPDU(strlen(DEL_FILE_OK) + 1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                strcpy(respdu->caData, DEL_FILE_OK);
            }
            else
            {
                respdu = mkPDU(strlen(DEL_FILE_FAILURED) + 1);
                respdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_RESPOND;
                strcpy(respdu->caData, DEL_FILE_FAILURED);
            }

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }

        //上传文件请求
        case ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            qint64 fileSize = 0;
            std::sscanf(pdu->caData, "%s %lld", caFileName, &fileSize);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            qDebug() << strPath;
            delete []pPath;
            pPath = NULL;

            m_file.setFileName(strPath);
            //以只写方式打开文件，若文件不存在，则会自动创建文件
            if (m_file.open(QIODevice::WriteOnly))
            {
                m_bUpload = true;
                m_iTotal = fileSize;
                m_iRecved = 0;
            }

            break;
        }

        //下载文件请求
        case ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            strcpy(caFileName, pdu->caData);
            char *pPath = new char[pdu->uiMsgLen];
            strncpy(pPath, pdu->caMsg, pdu->uiMsgLen);
            QString strPath = QString("%1/%2").arg(pPath).arg(caFileName);
            qDebug() << strPath;
            delete []pPath;
            pPath = NULL;

            QFileInfo fileInfo(strPath);
            qint64 fileSize = fileInfo.size();
            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_RESPOND;
            std::sprintf(respdu->caData, "%s %lld", caFileName, fileSize);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            m_file.setFileName(strPath);
            m_file.open(QIODevice::ReadOnly);
            m_pTimer->start(1000);

            break;
        }

        //共享文件请求
        case ENUM_MSG_TYPE_SHARE_FILE_REQUEST:
        {
            char caSendName[32] = {'\0'};
            int num = 0;
            std::sscanf(pdu->caData, "%s%d", caSendName, &num);
            int size = num  * 32;
            PDU *respdu = mkPDU(pdu->uiMsgLen - size);
            respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;
            strcpy(respdu->caData, caSendName);
            strncpy(respdu->caMsg, (char*)(pdu->caMsg) + size, pdu->uiMsgLen - size);

            char caRecvName[32] = {'\0'};
            for (int i = 0; i < num; ++i)
            {
                strncpy(caRecvName, (char*)(pdu->caMsg) + i*32, 32);
                MyTcpServer::getInstance().resend(caRecvName, respdu);
            }
            free(respdu);
            respdu = NULL;

            respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_SHARE_FILE_NOTE;
            strcpy(respdu->caData, SHARE_FILE_OK);
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }
        case ENUM_MSG_TYPE_SHARE_FILE_NOTE_RESPOND:
        {
            QString strRecvPath = QString("./%1").arg(pdu->caData);
            QString strShareFilePath = QString("%1").arg((char*)pdu->caMsg);
            int index = strShareFilePath.lastIndexOf('/');
            QString strFileName = strShareFilePath.right(strShareFilePath.size() - index);
            strRecvPath = strRecvPath + strFileName;

            QFileInfo fileInfo(strShareFilePath);
            if (fileInfo.isFile())
            {
                QFile::copy(strShareFilePath, strRecvPath);
            }
            else if (fileInfo.isDir())
            {
                copyDir(strShareFilePath, strRecvPath);
            }

            break;
        }

        //移动文件请求
        case ENUM_MSG_TYPE_MOVE_FILE_REQUEST:
        {
            char caFileName[32] = {'\0'};
            int srcLen = 0;
            int destLen = 0;
            std::sscanf(pdu->caData, "%d%d%s", &srcLen, &destLen,caFileName);

            char *pSrcPath = new char[srcLen+1];
            char *pDestPath = new char[destLen+1+32];
            memset(pSrcPath, '\0', srcLen+1);
            memset(pDestPath, '\0', destLen+1+32);

            memcpy(pSrcPath, pdu->caMsg, srcLen);
            memcpy(pDestPath, (char*)(pdu->caMsg) + (srcLen+1), destLen);

            PDU *respdu = mkPDU(0);
            respdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_RESPOND;
            QFileInfo fileInfo(pDestPath);
            if (fileInfo.isDir())
            {
                strcat(pDestPath, "/");
                strcat(pDestPath, caFileName);

                bool ret = QFile::rename(pSrcPath, pDestPath);
                if (ret)
                {
                    strcpy(respdu->caData, MOVE_FILE_OK);
                }
                else
                {
                    strcpy(respdu->caData, COMMON_ERR);
                }
            }
            else if (fileInfo.isFile())
            {
                strcpy(respdu->caData, MOVE_FILE_FAILURED);
            }
            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;

            break;
        }

        default:
            break;
        }

        free(pdu);
        pdu = NULL;
    }
    else
    {
        PDU *respdu;
        respdu = mkPDU(0);
        respdu->uiMsgType  = ENUM_MSG_TYPE_UPLOAD_FILE_RESPOND;

        QByteArray buff = readAll();
        m_file.write(buff);
        m_iRecved += buff.size();
        if (m_iTotal == m_iRecved)
        {
            m_file.close();
            m_bUpload = false;

            strcpy(respdu->caData, UPLOAD_FILE_OK);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }
        else if (m_iTotal < m_iRecved)
        {   m_file.close();
            m_bUpload = false;

            strcpy(respdu->caData, UPLOAD_FILE_FAILURED);

            write((char*)respdu, respdu->uiPDULen);
            free(respdu);
            respdu = NULL;
        }

    }
}

void MyTcpSocket::clientOffline()
{
    OpeDB::getInstance().handleOffline(m_strName.toStdString().c_str());
    emit offlien(this);
}

void MyTcpSocket::sendFileToClinet()
{
    char *pData = new char[4096];
    qint64 ret = 0;
    while (true)
    {
        ret = m_file.read(pData, 4096);
        if (ret > 0 && ret <= 4096)
        {
            write(pData, ret);
        }
        else if (0 == ret)
        {
            m_file.close();
            break;
        }
        else if (ret < 0)
        {
            qDebug() << "发送文件内容给客户端过程中失败";
            m_file.close();
            break;
        }
    }
    delete []pData;
    pData = NULL;
}
