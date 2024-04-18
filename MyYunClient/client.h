#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QNetworkAccessManager>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QBuffer>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include <QList>
#include <QMap>
#include "mynetworkmanager.h"
#include "login.h"
#include "filemanager.h"
#include "fileupdownload.h"
class Client : public QObject{
    Q_OBJECT
public:
    explicit Client(QObject *parent=nullptr);
    ~Client(){
        delete m_login;
        delete m_fileManager;
        delete m_fileUpDownLoad;
        delete m_myNetorkManager;
    }

public:
    Q_INVOKABLE void uploadFileChunks(const QString& filePath);
    Q_INVOKABLE void uploadFilesList(const QStringList& filesPathList);
    Q_INVOKABLE void login(const QString &username, const QString &passWord);
    Q_INVOKABLE void regist(const QString& username, const QString& passWord);
    Q_INVOKABLE QVariant getFilesList() const;
    Q_INVOKABLE void downLoadFiles(const QStringList& downLoadFilesList, const QString& filePath);
    Q_INVOKABLE void deleteFiles(const QStringList& deleteFilesList);
    Q_INVOKABLE void renameFileName(const QString& oldFileName, const QString& newFileName);
    Q_INVOKABLE void shareFiles(const QStringList& shareFilesList, const int& day);
    Q_INVOKABLE void downLoadFromSharing(const QString& uuid);
    Q_INVOKABLE void filterFiles(const QString& types);
    Q_INVOKABLE void keyWordsFilterFiles(const QString &keyWords);
    Q_INVOKABLE QVariant getFilterFilesList() const;//
    Q_INVOKABLE void downLoadSingleFile(const QString& filename);
    Q_INVOKABLE void downLoadSingleFile(const QString& filename, const QString& username, const int& type);
    Q_INVOKABLE void getSharedFilesListFromUuid(const QString& uuid);
    Q_INVOKABLE void saveChangedText(const QString& filename, const QString& filecontent);
    Q_INVOKABLE void saveUsernameAndPassword(bool rememberPassword);
    Q_INVOKABLE void loadSaveCredentials();


signals:
    void credentialsLoaded(const QString& username, const QString& password);

private:
    void downLoadSingleFile(const QString& filename, const QString& filePath);
    void downLoadSingleFile(const QString& username, const QString& filename, const QString& filePath);
    void deleteSingleFile(const QString& filename);

private:
    QString m_userName;
    QString m_userPasWord;
    Login* m_login = nullptr;
    FileManager* m_fileManager = nullptr;
    FileUpDownLoad* m_fileUpDownLoad = nullptr;
    MyNetworkManager* m_myNetorkManager = nullptr;
};

#endif // CLIENT_H
