#ifndef BOOK_H
#define BOOK_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "protocol.h"
#include <QTimer>

class Book : public QWidget
{
    Q_OBJECT
public:
    explicit Book(QWidget *parent = nullptr);

    void updateFileList(const PDU *pdu);
    void clearEnterDir();
    QString getEnterDir();
    void setDownloadStatus(bool status);
    bool getDownloadStatus();
    QString getSaveFilePath();
    QString getShareFileName();

    qint64 m_iToal;     //文件总大小
    qint64 m_iRecved;   //已下载大小

signals:

public slots:
    void createDir();
    void flushFile();
    void delDir();
    void renameFile();
    void enterDir(const QModelIndex &index);
    void returnPre();
    void delRegFile();
    void uploadFile();
    void downloadFile();
    void shareFile();
    void moveFile();
    void selectDestDir();

    void uploadFileData();  //定时器指定时间后传送数据

private:
    QListWidget *m_pBookListW;  //文件名列表

    QPushButton *m_pReturnPB;   //返回
    QPushButton *m_pCreateDirPB;//创建文件夹
    QPushButton *m_pDelDirPB;   //删除文件夹
    QPushButton *m_pRenamePB;   //重命名文件夹
    QPushButton *m_pFlushFilePB;//刷新文件夹
    QPushButton *m_pUpLoadPB;   //上传文件
    QPushButton *m_pDownLoadPB; //下载文件
    QPushButton *m_pDelFilePB;  //删除文件
    QPushButton *m_pShareFilePB;//分享文件
    QPushButton *m_pMoveFilePB; //移动文件
    QPushButton *m_pSelectDirPB;//目标目录

    QString m_strEnterDir;  //进入文件夹后的新路径
    QString m_strUploadFilePath;

    QTimer *m_pTimer;   //定时器

    QString m_strSaveFilePath;
    bool m_bDownload;

    QString m_strShareFileName;
    QString m_strMoveFileName;
    QString m_strMoveFilePath;
    QString m_strDestDir;
};

#endif // BOOK_H
