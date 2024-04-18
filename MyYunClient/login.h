#ifndef LOGIN_H
#define LOGIN_H
#include <QString>
#include <QObject>
#include <QRegularExpression>
#include "mynetworkmanager.h"
class Login : public QObject{
    Q_OBJECT
public:
    static Login* instance();
    int login(const QString &username, const QString &passWord);
    int regist(const QString& username, const QString& passWord);



signals:
    void invalid(const QString& msg);

private:
    Login(QObject* parent = nullptr);
    static Login* m_instance;
    MyNetworkManager* loginNetwork = nullptr;

    bool validateAccount(const QString& username);
    bool validatePassword(const QString& password);

    QString m_userName;
    QString m_userPasWord;
};

#endif // LOGIN_H
