#include "filemanager.h"
FileManager* FileManager::m_instance = nullptr;
FileManager::FileManager(QObject *parent) : QObject(parent){
    fileNetManager = MyNetworkManager::instance();
}

FileManager* FileManager::instance(){
    if (m_instance == nullptr){
        m_instance = new FileManager();
    }
    return m_instance;
}
int FileManager::deleteFiles(const QStringList &deleteFilesList){
    for(const auto& str : deleteFilesList){
        deleteSingleFile(str);
    }
    return 0;
}

int FileManager::deleteSingleFile(const QString &filename){
    return fileNetManager->deleteSingleFile(filename);
}

int FileManager::renameFileName(const QString &oldFileName, const QString &newFileName){
    return fileNetManager->renameFileName(oldFileName, newFileName);
}

int FileManager::shareFiles(const QStringList &shareFilesList, const int &day){
    return fileNetManager->shareFiles(shareFilesList, day);
}

int FileManager::filterFiles(const QString &types){

    m_FilterFilesList.clear();
    if(types == "默 认"){
        //emit filesListChanged();
        return -1;
    }
    QVariantMap fileMap;
    QString fileName;
    QFileInfo *fileInfo;
    QString extension;
    QVariantList filesList = fileNetManager->getFilesList().toList();
    for(int i = 0; i < filesList.size(); ++i){
        fileMap = filesList[i].toMap();
        fileName =  fileMap["file_name"].toString();
        fileInfo = new QFileInfo(fileName);
        extension = fileInfo->suffix().toLower();
        if ((extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "gif") && types == "图 片"){
            m_FilterFilesList.append(fileMap);
            continue;
        }else if ((extension == "doc" || extension == "docx" || extension == "pdf" || extension == "txt") && types == "文 档"){
            m_FilterFilesList.append(fileMap);
            continue;
        }else if ((extension == "mp4" || extension == "avi" || extension == "mov" || extension == "wmv") && types == "视 频"){
            m_FilterFilesList.append(fileMap);
            continue;
        }else if ((extension == "mp3" || extension == "wav" || extension == "flac" || extension == "aac") && types == "音 频") {
            m_FilterFilesList.append(fileMap);
            continue;
        }else if(types == "其 他"){
            m_FilterFilesList.append(fileMap);
        }
    }

    emit filterFilesListChanged();
    return 0;
}

int FileManager::keyWordsFilterFiles(const QString &keyWords){

    m_FilterFilesList.clear();
    if(keyWords == ""){
        //emit filesListChanged();
        return -1;
    }
    QVariantMap fileMap;
    QString fileName;
    QVariantList filesList = fileNetManager->getFilesList().toList();
    for(int i = 0; i < filesList.size(); ++i){
        fileMap = filesList[i].toMap();
        fileName = fileMap["file_name"].toString();
        if(fileName.contains(keyWords)){
            m_FilterFilesList.append(fileMap);
        }
    }
    emit filterFilesListChanged();
    return 0;
}

QVariant FileManager::getFilesList()const{
    return QVariant::fromValue(fileNetManager->getFilesList());
}

QVariant FileManager::getFilterFilesList() const{
    return QVariant::fromValue(m_FilterFilesList);
}

int FileManager::getSharedFilesListFromUuid(const QString &uuid){
    return fileNetManager->getSharedFilesListFromUuid(uuid);
}

int FileManager::saveChangedText(const QString &filename, const QString &filecontent){
    return fileNetManager->saveChangedText(filename, filecontent);
}
