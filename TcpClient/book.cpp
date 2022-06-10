#include "book.h"
#include "tcpclient.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include "opewidget.h"
#include "sharefile.h"

Book::Book(QWidget *parent) : QWidget(parent)
{
    m_strEnterDir.clear();
    m_pTimer = new QTimer;
    m_bDownload = false;

    m_pBookListW = new QListWidget;

    m_pReturnPB = new QPushButton("返回");
    m_pCreateDirPB = new QPushButton("创建文件夹");
    m_pDelDirPB = new QPushButton("删除文件夹");
    m_pRenamePB = new QPushButton("重命名文件夹");
    m_pFlushFilePB = new QPushButton("刷新文件夹");

    QVBoxLayout *pDirVBL = new QVBoxLayout;
    pDirVBL->addWidget(m_pReturnPB);
    pDirVBL->addWidget(m_pCreateDirPB);
    pDirVBL->addWidget(m_pDelDirPB);
    pDirVBL->addWidget(m_pRenamePB);
    pDirVBL->addWidget(m_pFlushFilePB);

    m_pUpLoadPB = new QPushButton("上传文件");
    m_pDownLoadPB = new QPushButton("下载文件");
    m_pDelFilePB = new QPushButton("删除文件");
    m_pShareFilePB = new QPushButton("分享文件");
    m_pMoveFilePB = new QPushButton("移动文件");
    m_pSelectDirPB = new QPushButton("目标目录");
    m_pSelectDirPB->setEnabled(false);

    QVBoxLayout *pFileVBL = new QVBoxLayout;
    pFileVBL->addWidget(m_pUpLoadPB);
    pFileVBL->addWidget(m_pDownLoadPB);
    pFileVBL->addWidget(m_pDelFilePB);
    pFileVBL->addWidget(m_pShareFilePB);
    pFileVBL->addWidget(m_pMoveFilePB);
    pFileVBL->addWidget(m_pSelectDirPB);

    QHBoxLayout *pMain = new QHBoxLayout;
    pMain->addWidget(m_pBookListW);
    pMain->addLayout(pDirVBL);
    pMain->addLayout(pFileVBL);

    setLayout(pMain);

    connect(m_pCreateDirPB, SIGNAL(clicked(bool)), this, SLOT(createDir()));
    connect(m_pFlushFilePB, SIGNAL(clicked(bool)), this, SLOT(flushFile()));
    connect(m_pDelDirPB, SIGNAL(clicked(bool)), this , SLOT(delDir()));
    connect(m_pRenamePB, SIGNAL(clicked(bool)), this, SLOT(renameFile()));
    //进入文件夹
    connect(m_pBookListW, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(enterDir(QModelIndex)));
    connect(m_pReturnPB, SIGNAL(clicked(bool)), this, SLOT(returnPre()));
    connect(m_pDelFilePB, SIGNAL(clicked(bool)), this, SLOT(delRegFile()));
    connect(m_pUpLoadPB, SIGNAL(clicked(bool)), this, SLOT(uploadFile()));
    //计时器，分割链接和数据上传
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(uploadFileData()));
    connect(m_pDownLoadPB, SIGNAL(clicked(bool)), this, SLOT(downloadFile()));
    connect(m_pShareFilePB, SIGNAL(clicked(bool)), this, SLOT(shareFile()));
    connect(m_pMoveFilePB, SIGNAL(clicked(bool)), this, SLOT(moveFile()));
    connect(m_pSelectDirPB, SIGNAL(clicked(bool)), this, SLOT(selectDestDir()));

}

void Book::updateFileList(const PDU *pdu)
{
    if (NULL == pdu)
    {
        return;
    }

    m_pBookListW->clear();
    FileInfo *pFileInfo = NULL;
    int iCount = (pdu->uiMsgLen/sizeof(FileInfo));
    for (int i = 0; i < iCount; ++i)
    {
        pFileInfo = (FileInfo*)(pdu->caMsg) + i;
        qDebug() << pFileInfo->caFileName << pFileInfo->uiFileType;
        QListWidgetItem *pItem = new QListWidgetItem;
        if (0 == pFileInfo->uiFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":./map/dir.png")));
        }
        else if (1 == pFileInfo->uiFileType)
        {
            pItem->setIcon(QIcon(QPixmap(":./map/reg.png")));
        }
        pItem->setText(pFileInfo->caFileName);
        m_pBookListW->addItem(pItem);
    }
}

void Book::clearEnterDir()
{
    m_strEnterDir.clear();
}

QString Book::getEnterDir()
{
    return m_strEnterDir;
}

void Book::setDownloadStatus(bool status)
{
    m_bDownload = status;
}

bool Book::getDownloadStatus()
{
    return m_bDownload;
}

QString Book::getSaveFilePath()
{
    return m_strSaveFilePath;
}

QString Book::getShareFileName()
{
    return m_strShareFileName;
}

void Book::returnPre()
{
    QString strCurPath = TcpClient::getInstance().getCurPath();
    QString strRootPath = "./" + TcpClient::getInstance().getLoginName();
    if (strCurPath == strRootPath)
    {
        QMessageBox::warning(this, "返回", "返回失败：已在根目录");
    }
    else
    {
        int index = strCurPath.lastIndexOf('/');
        strCurPath.remove(index, strCurPath.size() - index);
        qDebug() << "return" << strCurPath;
        TcpClient::getInstance().setCurPath(strCurPath);
        //调用刷新请求
        flushFile();
    }
}

void Book::delRegFile()
{
    QString strCurPath = TcpClient::getInstance().getCurPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, "删除文件", "请选择要删除的文件");
    }
    else
    {
        QString strDelName = pItem->text();
        PDU *pdu = mkPDU(strCurPath.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_FILE_REQUEST;
        strncpy(pdu->caData, strDelName.toStdString().c_str(), strDelName.size());
        strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;

    }
}

void Book::uploadFile()
{
    QString strCurPath = TcpClient::getInstance().getCurPath();
    m_strUploadFilePath = QFileDialog::getOpenFileName();

    if (!m_strUploadFilePath.isEmpty())
    {
        int index = m_strUploadFilePath.lastIndexOf('/');
        QString strFileName = m_strUploadFilePath.right(m_strUploadFilePath.size() - index - 1);

        QFile file(m_strUploadFilePath);
        qint64 fileSize = file.size();  //文件大小

        PDU *pdu = mkPDU(strCurPath.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_UPLOAD_FILE_REQUEST;
        strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
        std::sprintf(pdu->caData, "%s %lld", strFileName.toStdString().c_str(), fileSize);

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;

        m_pTimer->start(1000);
    }
    else
    {
        QMessageBox::warning(this, "上传文件", "上传文件不能为空");
    }
}

void Book::downloadFile()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, "下载文件", "请选择要下载的文件");
    }
    else
    {        
        QString strSaveFilePath = QFileDialog::getSaveFileName();
        if (strSaveFilePath.isEmpty())
        {
            QMessageBox::warning(this, "下载文件", "请指定保存路径");
            m_strSaveFilePath.clear();
        }
        else
        {
            m_strSaveFilePath = strSaveFilePath;
        }

        QString strCurPath = TcpClient::getInstance().getCurPath();
        PDU *pdu = mkPDU(strCurPath.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DOWNLOAD_FILE_REQUEST;
        QString strFileName = pItem->text();
        strncpy(pdu->caData, strFileName.toStdString().c_str(), strFileName.size());
        strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);

        free(pdu);
        pdu = NULL;
    }
}

void Book::shareFile()
{
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, "共享文件", "请选择要共享的文件");
        return;
    }
    else
    {
        m_strShareFileName = pItem->text();
    }

    Friend *pFriend = OpeWidget::getInstance().getFriend();
    QListWidget *pFriendList = pFriend->getFriendList();
    ShareFile::getInstance().updateFriend(pFriendList);
    if (ShareFile::getInstance().isHidden())
    {
        ShareFile::getInstance().show();
    }

}

void Book::moveFile()
{
    QListWidgetItem *pCurItem = m_pBookListW->currentItem();
    if (NULL != pCurItem)
    {
        m_strMoveFileName = pCurItem->text();
        QString strCurPath = TcpClient::getInstance().getCurPath();
        m_strMoveFilePath = strCurPath+ '/' +m_strMoveFileName;

        m_pSelectDirPB->setEnabled(true);

    }
    else
    {
        QMessageBox::information(this, "移动文件", "请选择要移动的文件");
    }
}

void Book::selectDestDir()
{
    QListWidgetItem *pCurItem = m_pBookListW->currentItem();
    if (NULL != pCurItem)
    {
        QString strDestDir = pCurItem->text();
        QString strCurPath = TcpClient::getInstance().getCurPath();
        m_strDestDir = strCurPath+ '/' +strDestDir;

        int srcLen = m_strMoveFilePath.size();
        int destLen = m_strDestDir.size();
        PDU *pdu = mkPDU(srcLen + destLen + 2);
        pdu->uiMsgType = ENUM_MSG_TYPE_MOVE_FILE_REQUEST;
        std::sprintf(pdu->caData, "%d %d %s", srcLen, destLen, m_strMoveFileName.toStdString().c_str());

        strncpy(pdu->caMsg, m_strMoveFilePath.toStdString().c_str(), srcLen);
        strncpy(pdu->caMsg + srcLen + 1, m_strDestDir.toStdString().c_str(), destLen);

        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;
    }
    else
    {
        QMessageBox::information(this, "移动文件", "请选择要移动的文件");
    }
    m_pSelectDirPB->setEnabled(false);
}

void Book::uploadFileData()
{
    m_pTimer->stop();
    QFile file(m_strUploadFilePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "上传文件", "打开文件失败");
        return;
    }

    //据说4096效率较高
    char *pBuffer = new char[4096];
    while (true)
    {
        qint64 ret = file.read(pBuffer, 4096);
        if (ret > 0 && ret <= 4096)
        {
            TcpClient::getInstance().getTcpSocket().write(pBuffer, ret);
        }
        else if (0 == ret)
        {
            break;
        }
        else
        {
            QMessageBox::warning(this, "上传文件", "上传文件失败：读文件失败");
            break;
        }
    }
    file.close();
    delete []pBuffer;
    pBuffer =  NULL;

}

void Book::createDir()
{
    QString strNewDir = QInputDialog::getText(this, "新建文件夹", "新建文件夹名");
    //此处简单限制(可遍历文件名限制长度和特殊字符)
    if (!strNewDir.isEmpty())
    {
        if (strNewDir.size() <= 32)
        {
            QString strName = TcpClient::getInstance().getLoginName();
            QString strCurPath = TcpClient::getInstance().getCurPath();
            PDU *pdu = mkPDU(strCurPath.size() + 1);

            pdu->uiMsgType = ENUM_MSG_TYPE_CREATE_DIR_REQUEST;
            strncpy(pdu->caData, strName.toStdString().c_str(), strName.size());
            strncpy(pdu->caData + 32, strNewDir.toStdString().c_str(), strCurPath.size());
            strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());

            TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
        else
        {
            QMessageBox::warning(this, "新建文件夹", "新文件夹名不能超过32个字节");
        }
    }
    else
    {
        QMessageBox::warning(this, "新建文件夹", "新文件夹名不能为空");
    }

}

void Book::flushFile()
{
    QString strCurPath = TcpClient::getInstance().getCurPath();
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_FLUSH_FILE_REQUEST;
    strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;

}

void Book::delDir()
{
    QString strCurPath = TcpClient::getInstance().getCurPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, "删除文件", "请选择要删除的文件");
    }
    else
    {
        QString strDelName = pItem->text();
        PDU *pdu = mkPDU(strCurPath.size() + 1);
        pdu->uiMsgType = ENUM_MSG_TYPE_DEL_DIR_REQUEST;
        strncpy(pdu->caData, strDelName.toStdString().c_str(), strDelName.size());
        strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
        TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
        free(pdu);
        pdu = NULL;

    }
}

void Book::renameFile()
{
    QString strCurPath = TcpClient::getInstance().getCurPath();
    QListWidgetItem *pItem = m_pBookListW->currentItem();
    if (NULL == pItem)
    {
        QMessageBox::warning(this, "重命名文件", "请选择要重命名的文件");
    }
    else
    {
        QString strOldName = pItem->text();
        QString strNewName = QInputDialog::getText(this, "重命名文件", "请输入新文件名");
        if (!strNewName.isEmpty())
        {
            PDU *pdu = mkPDU(strCurPath.size() + 1);
            pdu->uiMsgType = ENUM_MSG_TYPE_RENAME_FILE_REQUEST;
            strncpy(pdu->caData, strOldName.toStdString().c_str(), strOldName.size());
            strncpy(pdu->caData + 32, strNewName.toStdString().c_str(), strNewName.size());
            strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
            TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
            free(pdu);
            pdu = NULL;
        }
        else
        {
            QMessageBox::warning(this, "重命名文件", "新文件名不能为空");
        }
    }
}

void Book::enterDir(const QModelIndex &index)
{
    QString strDirName = index.data().toString();
    m_strEnterDir = strDirName;

    QString strCurPath =TcpClient::getInstance().getCurPath();
    PDU *pdu = mkPDU(strCurPath.size() + 1);
    pdu->uiMsgType = ENUM_MSG_TYPE_ENTER_DIR_REQUEST;
    strncpy(pdu->caData, strDirName.toStdString().c_str(), strDirName.size());
    strncpy(pdu->caMsg, strCurPath.toStdString().c_str(), strCurPath.size());
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}
