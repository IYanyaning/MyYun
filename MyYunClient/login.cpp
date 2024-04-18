#include "login.h"

Login* Login::m_instance = nullptr;
Login::Login(QObject *parent) : QObject(parent){
    loginNetwork = MyNetworkManager::instance();
}

Login* Login::instance(){
    if(m_instance == nullptr){
        m_instance = new Login();
    }
    return m_instance;
}

bool Login::validateAccount(const QString &username){
    QRegularExpression re("[a-zA-Z]{10,20}");
    return re.match(username).hasMatch();
}

bool Login::validatePassword(const QString &password){
    QRegularExpression re("[a-zA-Z0-9]{10,20}");
    return re.match(password).hasMatch();
}

int Login::login(const QString &username, const QString &passWord){
    if(!validateAccount(username)){
        emit invalid("用户名格式不对，仅支持长度为10-20位的英文");
        return -1;
    }
    if(!validatePassword(passWord)){
        emit invalid("密码格式不对，仅支持长度10-20位的英文、数字");
        return -1;
    }
    return loginNetwork->login(username, passWord);


}

int Login::regist(const QString &username, const QString &passWord){
    if(!validateAccount(username)){
        emit invalid("用户名格式不对，仅支持长度为10-20位的英文");
        return -1;
    }
    if(!validatePassword(passWord)){
        emit invalid("密码格式不对，仅支持长度10-20位的英文、数字");
        return -1;
    }
    return loginNetwork->regist(username, passWord);
}
