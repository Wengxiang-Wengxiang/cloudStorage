#include "online.h"
#include "ui_online.h"
#include <QDebug>
#include "tcpclient.h"

Online::Online(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Online)
{
    ui->setupUi(this);
}

Online::~Online()
{
    delete ui;
}

void Online::showUser(PDU *pdu)
{
    if (NULL == pdu)
    {
        return;
    }
    u_int uiSize = pdu->uiMsgLen/32;
    char caTmp[32];
    ui->online_lw->clear();
    for (u_int i = 0; i < uiSize; ++i)
    {
        strncpy(caTmp, (char*)(pdu->caMsg) + i*32, 32);
        ui->online_lw->addItem(caTmp);
    }
}

void Online::on_addFriend_pb_clicked()
{
    QListWidgetItem *pItem = ui->online_lw->currentItem();
//    qDebug() << pItem->text();
    QString strPerUserName = pItem->text();
    QString strLoginName = TcpClient::getInstance().getLoginName();
    PDU *pdu = mkPDU(0);
    pdu->uiMsgType = ENUM_MSG_TYPE_ADD_FRIEND_REQUEST;
    strncpy(pdu->caData, strPerUserName.toStdString().c_str(), strPerUserName.size());
    strncpy(pdu->caData + 32, strLoginName.toStdString().c_str(), strLoginName.size());
//    qDebug() << pdu->caData << pdu->caData+32;
    TcpClient::getInstance().getTcpSocket().write((char*)pdu, pdu->uiPDULen);
    free(pdu);
    pdu = NULL;
}
