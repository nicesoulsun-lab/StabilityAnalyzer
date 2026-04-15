#include "inc/encryptionAlgorithm/encryptionalgorithm.h"

encryptionAlgorithm::encryptionAlgorithm(QObject *parent) : QObject(parent)
{

}
//加密
QByteArray encryptionAlgorithm::encryption(QByteArray originalData)
{
    QByteArray decrypData;
    decrypData=originalData.toBase64();
    return decrypData;
}

//解密
QByteArray encryptionAlgorithm::decryption(QByteArray originalData)
{
    QByteArray decrypData;
    decrypData=QByteArray::fromBase64(originalData);
    return decrypData;
}

encryptionAlgorithm *getCryptionInstance()
{
    static encryptionAlgorithm *instance = nullptr;
    if(instance == nullptr)
    {
        instance = new encryptionAlgorithm;
    }
    return instance;
}
