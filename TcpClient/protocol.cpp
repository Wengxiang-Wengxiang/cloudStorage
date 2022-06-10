#include "protocol.h"

PDU *mkPDU(u_int uiMsgLen)
{
    u_int uiPDULen = sizeof(PDU) + uiMsgLen;
    PDU *pdu = (PDU*)malloc(uiPDULen);
    if (NULL == pdu)
    {
        exit(EXIT_FAILURE);
    }
    memset(pdu, 0, uiPDULen);
    pdu->uiPDULen = uiPDULen;
    pdu->uiMsgLen = uiMsgLen;

    return pdu;
}
