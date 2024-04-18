#include "client.h"

Client::Client(QObject *parent) : QObject(parent){
    m_login = Login::instance();
    m_fileManager = FileManager::instance();
    m_fileUpDownLoad = new FileUpDownLoad();
    m_myNetorkManager = MyNetworkManager::instance();
}

void Client::uploadFileChunks(const QString &filePath){
    m_fileUpDownLoad->uploadFileChunks(filePath);
}

void Client::uploadFilesList(const QStringList &filesPathList){
    m_fileUpDownLoad->uploadFilesList(filesPathList);
}

void Client::login(const QString &username, const QString &passWord){
    if( m_login->login(username, passWord) == 0){
        m_userName = username;
        m_userPasWord = passWord;
    }
}

void Client::regist(const QString &username, const QString &passWord){
    m_login->regist(username, passWord);
}

QVariant Client::getFilesList() const{
    return QVariant::fromValue(m_fileManager->getFilesList());
}

void Client::downLoadFiles(const QStringList &downLoadFilesList, const QString& filePath){
    m_fileUpDownLoad->downLoadFiles(downLoadFilesList, filePath);
}

void Client::downLoadSingleFile(const QString &filename){
    m_fileUpDownLoad->downLoadSingleFile(filename);
}

void Client::downLoadSingleFile(const QString& filename, const QString& username, const int& type){
    m_fileUpDownLoad->downLoadSingleFile(filename, username, type);
}

void Client::downLoadSingleFile(const QString &filename, const QString& filepath){
    m_fileUpDownLoad->downLoadSingleFile(filename, filepath);
}

void Client::downLoadSingleFile(const QString &username, const QString &filename, const QString &filePath){
    m_fileUpDownLoad->downLoadSingleFile(username, filename, filePath);
}

void Client::deleteFiles(const QStringList &deleteFilesList){
    m_fileManager->deleteFiles(deleteFilesList);
}

void Client::deleteSingleFile(const QString &filename){
    m_fileManager->deleteSingleFile(filename);
}

//重命名，前端检测新、旧名字是否一致
void Client::renameFileName(const QString &oldFileName, const QString &newFileName){
    m_fileManager->renameFileName(oldFileName, newFileName);
}


void Client::shareFiles(const QStringList &shareFilesList, const int& day){
    m_fileManager->shareFiles(shareFilesList, day);
}

void Client::downLoadFromSharing(const QString &uuid){
    m_fileUpDownLoad->downLoadFromSharing(uuid);
}

void Client::filterFiles(const QString &types){
    m_fileManager->filterFiles(types);
}

void Client::keyWordsFilterFiles(const QString &keyWords){
    m_fileManager->keyWordsFilterFiles(keyWords);
}

QVariant Client::getFilterFilesList()const{

    return QVariant::fromValue(m_fileManager->getFilterFilesList());
}

void Client::getSharedFilesListFromUuid(const QString &uuid){
    m_fileManager->getSharedFilesListFromUuid(uuid);
}


void Client::saveChangedText(const QString& filename, const QString &filecontent){
    m_fileManager->saveChangedText(filename, filecontent);
}
void Client::saveUsernameAndPassword(bool rememberPassword){
    if(rememberPassword){
        QFile file("./credentials.txt");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to open file";
            return;
        }
        QTextStream stream(&file);
        stream << "Username: " << m_userName << "\n";
        stream << "Password: " << m_userPasWord << "\n";

        file.close();
    }
}

void Client::loadSaveCredentials(){
    QFile file("credentials.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file";
        return;
    }

    // 读取数据
    QTextStream stream(&file);
    QString line;
    while (!stream.atEnd()) {
        line = stream.readLine();
        if (line.startsWith("Username: ")) {
            m_userName = line.mid(10);
        } else if (line.startsWith("Password: ")) {
            m_userPasWord = line.mid(10);
        }
    }

    file.close();

    // 如果存在已保存的账号和密码，则解密密码
    if (!m_userName.isEmpty() && !m_userPasWord.isEmpty()) {
        qDebug() << m_userName << " " << m_userPasWord;

        // 发射信号，将已保存的账号和密码传递给QML界面
        emit credentialsLoaded(m_userName, m_userPasWord);
    }
}
