#include "fileupdownload.h"

FileUpDownLoad::FileUpDownLoad(){
    updownLoadManager = MyNetworkManager::instance();
}

int FileUpDownLoad::uploadFileChunks(const QString &filePath){
    std::cout << "start file" << filePath.toStdString() << std::endl;
    QFile *file = new QFile(filePath);
    std::cout << "end file" << std::endl;
    QFileInfo* fileInfo = new QFileInfo(filePath);

    QString fileNameTemp = fileInfo->fileName();
    int sizeTemp = file->size();
    QCryptographicHash hash(QCryptographicHash::Md5);//MD5加密
    if (file->open(QIODevice::ReadOnly)) {
        while (!file->atEnd()) {
            QByteArray content = file->read(100 * 1024 * 1024);
            hash.addData(content);
        }
    }else{qDebug() << "Failed to open the file\n" ;return -1;}
    QByteArray  md5 = hash.result();
    QString md5Temp = md5.toHex();
    if(!file->reset()){std::cout<<"reset failed"<<std::endl;}

    return updownLoadManager->uploadFileChunks(md5Temp, sizeTemp, fileNameTemp, file, fileInfo);

}

int FileUpDownLoad::uploadFilesList(const QStringList &filesPathList){
    for(const QString& filePath : filesPathList){
        std::cout<< filePath.toStdString() << std::endl;
        uploadFileChunks(filePath);
    }
    return 0;
}

QVariant FileUpDownLoad::getFilesList() const{
    return QVariant::fromValue(updownLoadManager->getFilesList());
}


int FileUpDownLoad::downLoadFiles(const QStringList &downLoadFilesList, const QString &filePath){
    for(const auto& str : downLoadFilesList){
        downLoadSingleFile(str, filePath);
    }
    return 0;
}

int FileUpDownLoad::downLoadSingleFile(const QString &filename){
    return updownLoadManager->downLoadSingleFile(filename);
}

int FileUpDownLoad::downLoadSingleFile(const QString &filename, const QString &filePath){
    return updownLoadManager->downLoadSingleFile(filename, filePath);
}

int FileUpDownLoad::downLoadSingleFile(const QString &username, const QString &filename, const QString &filePath){
    return updownLoadManager->downLoadSingleFile(username, filename, filePath);
}

int FileUpDownLoad::downLoadFromSharing(const QString &uuid){
    return updownLoadManager->downLoadFromSharing(uuid);
}

int FileUpDownLoad::downLoadSingleFile(const QString &filename, const QString &username, const int &type){
    return updownLoadManager->downLoadSingleFile(filename, username, type);
}
