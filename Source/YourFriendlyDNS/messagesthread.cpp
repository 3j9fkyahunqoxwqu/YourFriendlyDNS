#include "messagesthread.h"

AppData* AppData::instance = nullptr;

AppData::AppData()
{
    dnsServer = 0;
    httpServer = 0;
}

AppData* AppData::get()
{
    if(instance == nullptr)
        instance = new AppData();
    return instance;
}

void MessagesThread::run()
{
    data = AppData::get();
    data->dnsServer = new SmallDNSServer();
    data->httpServer = new SmallHTTPServer();

    emit serversInitialized();

    #ifdef Q_OS_ANDROID
    if(data->dnsServer->dnsServerPort == 53)
        data->dnsServer->dnsServerPort = 5333;
    if(data->dnsServer->httpServerPort == 80)
        data->dnsServer->httpServerPort = 8080;

    AndroidSU_ServerOP *suOP = new AndroidSU_ServerOP(AndroidSU_ServerOP::opcode::iptablesSet, data->dnsServer->dnsServerPort, data->dnsServer->httpServerPort);
    connect(suOP, SIGNAL(finished()), suOP, SLOT(deleteLater()));
    suOP->start();
    emit androidInit();
    #endif

    if(data->dnsServer->startServer(QHostAddress::AnyIPv4, data->dnsServer->dnsServerPort))
        qDebug() << "DNS server started on address:" << data->dnsServer->serversock.localAddress() << "and port:" << data->dnsServer->serversock.localPort();
    if(data->httpServer->startServer(QHostAddress::AnyIPv4, data->dnsServer->httpServerPort))
        qDebug() << "HTTP server started on address:" << data->httpServer->serverAddress() << "and port:" << data->httpServer->serverPort();

    qDebug() << "MessagesThread started, for handling server duties!";
    exec(); //handles the signals and slots for objects owned by this thread
    //(eh, that wasn't doing anything that was here anyway, better without it and I think solid now for real!
}
