#ifndef ANDROIDSUOP_H
#define ANDROIDSUOP_H

#include <QThread>
#include <QProcess>

class AndroidSU_ServerOP : public QThread
{
public:
    enum class opcode
    {
        iptablesSet, iptablesRemove
    };

    opcode op;
    quint16 dnsServerPort,httpServerPort;
    AndroidSU_ServerOP(opcode op, quint16 dnsServerPort, quint16 httpServerPort = 0)
    {
        this->op = op;
        this->dnsServerPort = dnsServerPort;
        this->httpServerPort = httpServerPort;
    }
    void run()
    {
        QProcess su;
        QProcess du;
        if(op == opcode::iptablesSet)
        {
            //Run su, enable ipv4 forwarding, and do iptables redirect from port 53(dns) to 5333 (where this server is binded on android by default), also 80 to 8080 now
            su.start("su"); //First time running it, accept the root prompt
            if(!su.waitForStarted())
                return;
            du.start("su");
            du.waitForStarted();
            du.write(QString("iptables -t nat --list | grep \"ports %1\"\n").arg(dnsServerPort).toUtf8());
            du.waitForFinished();
            QByteArray listresult = du.read(4096);

            qDebug() << "iptables dns port list result:" << listresult << "list size:" << listresult.size();
            if(listresult.size() == 0)
            {
                qDebug() << "iptables dns not set yet, setting dns iptables now...";
                su.write("sysctl -w net.ipv4.ip_forward=1\n");
                su.write(QString("iptables -A INPUT -p udp --dport %1 -j ACCEPT\n").arg(dnsServerPort).toUtf8());
                su.write(QString("iptables -t nat -A PREROUTING -p udp --dport 53 -j REDIRECT --to-ports %1\n").arg(dnsServerPort).toUtf8());
            }

            if(httpServerPort != 0)
            {
                du.write(QString("iptables -t nat --list | grep \"ports %1\"\n").arg(httpServerPort).toUtf8());
                du.closeWriteChannel();
                du.waitForFinished();
                QByteArray listresult = du.read(4096);

                qDebug() << "iptables http port list result:" << listresult << "list size:" << listresult.size();
                if(listresult.size() == 0)
                {
                    qDebug() << "iptables http not set yet, setting http iptables now...";
                    su.write(QString("iptables -A INPUT -p tcp --dport %1 -j ACCEPT\n").arg(httpServerPort).toUtf8());
                    su.write(QString("iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports %1\n").arg(httpServerPort).toUtf8());
                }
            }

            su.closeWriteChannel();
            if(!su.waitForFinished())
                return;
        }
        else if(op == opcode::iptablesRemove)
        {
            su.start("su");
            if(!su.waitForStarted())
                return;

            su.write(QString("iptables -D INPUT -p udp --dport %1 -j ACCEPT\n").arg(dnsServerPort).toUtf8());
            su.write(QString("iptables -t nat -D PREROUTING -p udp --dport 53 -j REDIRECT --to-ports %1\n").arg(dnsServerPort).toUtf8());
            if(httpServerPort != 0)
            {
                su.write(QString("iptables -D INPUT -p tcp --dport %1 -j ACCEPT\n").arg(httpServerPort).toUtf8());
                su.write(QString("iptables -t nat -D PREROUTING -p tcp --dport 80 -j REDIRECT --to-ports %1\n").arg(httpServerPort).toUtf8());
            }
            su.closeWriteChannel();

            if(!su.waitForFinished())
                return;
        }
    }
};

#endif // ANDROIDSUOP_H
