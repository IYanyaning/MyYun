#ifndef FILEUPDOWNLOAD_H
#define FILEUPDOWNLOAD_H

#include "mynetworkmanager.h"
#include <QFile>
#include <QFileInfo>
class FileUpDownLoad{
public:
    FileUpDownLoad();
    int uploadFileChunks(const QString& filePath);
    int uploadFilesList(const QStringList& filesPathList);
    QVariant getFilesList() const;

    int downLoadFiles(const QStringList& downLoadFilesList, const QString& filePath);
    int downLoadSingleFile(const QString& filename);
    int downLoadSingleFile(const QString& filename, const QString& username, const int& type);
    int downLoadSingleFile(const QString& filename, const QString& filePath);
    int downLoadSingleFile(const QString &username, const QString &filename, const QString &filePath);
    //int downLoadSingleFile(const QString& filename, const QString& username, const int& type);

    int downLoadFromSharing(const QString& uuid);
private:
    MyNetworkManager* updownLoadManager = nullptr;
};

#endif // FILEUPDOWNLOAD_H
