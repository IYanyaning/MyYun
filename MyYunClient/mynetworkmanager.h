#ifndef MYNETWORKMANAGER_H
#define MYNETWORKMANAGER_H
#include <QNetworkAccessManager>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QObject>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>
#include <QFileInfo>
#include <iostream>
class MyNetworkManager: public QObject
{
    Q_OBJECT
public:
    static MyNetworkManager* instance(); // 获取单例实例的静态方法

    int login(const QString& username, const QString& passWord);
    int regist(const QString& username, const QString& passWord);
    int uploadFileChunks(const QString& md5Temp, const int& sizeTemp, const QString& fileNameTemp,  QFile*& file,  QFileInfo*& fileinfo);
    QVariant getFilesList() const;

    int downLoadSingleFile(const QString& filename);
    int downLoadSingleFile(const QString& filename, const QString& filePath);
    int downLoadSingleFile(const QString &username, const QString &filename, const QString &filePath);
    int downLoadSingleFile(const QString& filename, const QString& username, const int& type);

    int downLoadFromSharing(const QString& uuid);

    int deleteSingleFile(const QString& filename);
    int renameFileName(const QString& oldFileName, const QString& newFileName);
    int shareFiles(const QStringList& shareFilesList, const int& day);
    //int filterFiles(const QString& types);
    //int keyWordsFilterFiles(const QString &keyWords);
    //QVariant getFilterFilesList() const;
    int getSharedFilesListFromUuid(const QString& uuid);
    int saveChangedText(const QString& filename, const QString& filecontent);



private:
    void parseJsonDataToFileList(const QJsonObject &jsonObj);
    void parseJsonToSharingList(const QJsonObject &jsonObj);
    void addFileToList(QVariantList& fileList, const QString& fileName, const int& size);

    void removeFileFromList(QVariantList& fileList, const QString& fileName);
    QStringList parseFileNames(const QJsonObject& jsonObj);
signals:
    void checkLogin(bool res);
    void checkRegist(bool res);

    void usernameReceived(const QString& username);
    void getSharedUUidList(const QStringList& uuidList);
    void getSharedFilesList(const QStringList& filesList);
    void filesListChanged();

    void canntPreview();
    void downloadForQml(QByteArray data);
    void downloadForQmlOther(QByteArray data);
    void downloadForQmlOther3(QByteArray data);

    void renameResult(bool res);
    void sharedFilesListChanged(const QStringList& list, const QString& uuid);
    //void filterFilesListChanged();

    void getUuidFilesListAndUserName(const QStringList& filesList, const QString& userName);
    void invalidUuid();
private:
    MyNetworkManager(QObject *parent = nullptr); // 构造函数私有化
    static MyNetworkManager* m_instance; // 单例实例的静态成员变量
private:
    QNetworkAccessManager* m_manager = nullptr;
    QHttpMultiPart *m_multiPart = nullptr;// 创建一个QHttpMultiPart对象作为POST请求的主体
    QString m_ipAddress = "http://172.16.1.68:10000/";
    //QString m_ipAddress = "http://192.168.43.15:10000/";
    QString m_userName;
    QString m_userPasWord;
    QVariantList m_filesList;
    //QVariantList m_FilterFilesList;
    QStringList m_sharedFilesList;
    QStringList m_uuidList;
    QVariantMap m_sharingList;

    QStringList m_musicList;
};

#endif // MYNETWORKMANAGER_H
