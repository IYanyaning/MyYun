#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "mynetworkmanager.h"
class FileManager: public QObject
{
    Q_OBJECT
public:
    static FileManager* instance(); // 获取单例实例的静态方法
    int deleteFiles(const QStringList& deleteFilesList);
    int deleteSingleFile(const QString& filename);
    int renameFileName(const QString& oldFileName, const QString& newFileName);
    int shareFiles(const QStringList& shareFilesList, const int& day);


    int filterFiles(const QString& types);
    int keyWordsFilterFiles(const QString &keyWords);
    QVariant getFilesList() const;
    QVariant getFilterFilesList() const;

    int getSharedFilesListFromUuid(const QString& uuid);
    int saveChangedText(const QString& filename, const QString& filecontent);

signals:
    //void filesListChanged();
    void filterFilesListChanged();

private:
    FileManager(QObject *parent = nullptr); // 构造函数私有化
    static FileManager* m_instance; // 单例实例的静态成员变量
    MyNetworkManager* fileNetManager = nullptr;

    QVariantList m_FilterFilesList;
};

#endif // FILEMANAGER_H
