#include "mynetworkmanager.h"

MyNetworkManager* MyNetworkManager::m_instance = nullptr;
MyNetworkManager::MyNetworkManager(QObject *parent) : QObject(parent){
    m_manager = new QNetworkAccessManager(this);
    m_multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);// 创建一个QHttpMultiPart对象作为POST请求的主体
}

MyNetworkManager* MyNetworkManager::instance(){
    if (m_instance == nullptr){
        m_instance = new MyNetworkManager();
    }
    return m_instance;
}

int MyNetworkManager::login(const QString &username, const QString &passWord){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("user",username);
    postData.addQueryItem("passwd",passWord);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "login success";
            //处理登录成功
            QByteArray responseData = reply->readAll();
            //qDebug() << responseData;
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);

            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse JSON:" << error.errorString();
                emit checkLogin(false);
                return -1;
            }

            if (!jsonDoc.isObject()) {
                qWarning() << "JSON document is not an object";
                emit checkLogin(false);
            }

            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();

            //qDebug() << "Parsed keys and values:";
            for (const QString &key : keys) {
                //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                if(key == "CODE" && jsonObj.value(key).toVariant() == 20){//登录成功

                    m_userName = username;
                    m_userPasWord = passWord;
                    emit usernameReceived(username);

                    parseJsonDataToFileList(jsonObj);

                    parseJsonToSharingList(jsonObj);

                    emit getSharedUUidList(m_uuidList);

                    emit getSharedFilesList(m_sharedFilesList);

                    emit filesListChanged();

                    emit checkLogin(true);

                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 21){
                    emit checkLogin(false);
                }

            }

        }else{
            qDebug() << "login failed:" <<reply->errorString();
        }
        reply->deleteLater();
    });
    return 0;
}

void MyNetworkManager::parseJsonDataToFileList(const QJsonObject &jsonObj){
    if (jsonObj.contains("files") && jsonObj.value("files").isArray() && jsonObj.value("files").toArray().size() != 0) {
        QJsonArray filesArray = jsonObj.value("files").toArray();
        QVariantMap fileMap;
        for (const QJsonValue &fileValue : filesArray) {
            if (fileValue.isObject()) {
                fileMap = fileValue.toObject().toVariantMap();
                m_filesList.append(fileMap);
            }
        }
    }
}

void MyNetworkManager::parseJsonToSharingList(const QJsonObject &jsonObj){
    if(jsonObj.contains("file_shared") && jsonObj.value("file_shared").isArray() &&  jsonObj.value("file_shared").toArray().size() != 0 ){

        QJsonArray filesArray = jsonObj.value("file_shared").toArray();
        // 遍历文件列表
        QString tmpUuid = "";
        QStringList tmpList;
        for (const auto& fileData : filesArray) {

            QJsonObject fileObj = fileData.toObject();
            QString fileName = fileObj["file_name"].toString();
            QString uuid = fileObj["uuid"].toString();

            if(tmpUuid != uuid){
                if(tmpList.size() > 0){
                    m_uuidList.append(tmpUuid);
                    m_sharingList.insert(tmpUuid, tmpList);
                    tmpList.clear();
                }
                tmpUuid = uuid;
            }
            tmpList.append(fileName);
        }
        m_uuidList.append(tmpUuid);
        m_sharingList.insert(tmpUuid, tmpList);
        QVariantList fileList = m_sharingList.first().toList();
        //qDebug() << m_sharingList.first().toList().size();
        for(const auto& filename : fileList){
            //qDebug() << filename.toString();
            m_sharedFilesList.append(filename.toString());
        }
    }
}

int MyNetworkManager::regist(const QString &username, const QString &passWord){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "register"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("user",username);
    postData.addQueryItem("passwd",passWord);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "register success";
            //处理注册成功
            QByteArray responseData = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);

            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse JSON:" << error.errorString();
                emit checkRegist(false);
            }

            if (!jsonDoc.isObject()) {
                qWarning() << "JSON document is not an object";
                emit checkRegist(false);
            }

            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();
            //qDebug() << "Parsed keys and values:";
            for (const QString &key : keys) {
                //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                if(key == "CODE" && jsonObj.value(key).toVariant() == 10){//注册成功
                    emit checkRegist(true);
                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 11){
                    emit checkRegist(false);
                }

            }

        }else{
            qDebug() << "regist failed:" <<reply->errorString();
        }
        reply->deleteLater();
    });
    return 0;
}

int MyNetworkManager::uploadFileChunks(const QString &md5Temp, const int &sizeTemp, const QString &fileNameTemp, QFile*& file,  QFileInfo* &fileinfo){
    QString md5SizeJsonRequest = QString("{ \"md5\":\"%1\",\"size\":%2,\"file_name\":\"%3\",\"username\":\"%4\"}").arg(md5Temp).arg(sizeTemp).arg(fileNameTemp).arg(m_userName);
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "requestUpload"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "Keep-Alive");
    QNetworkReply *reply = m_manager->post(request, md5SizeJsonRequest.toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            std::cout << "request ask  md5_size  success\n";
            QByteArray responseData = reply->readAll();
            // 释放资源
            reply->deleteLater();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);
            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {qWarning() << "Failed to parse JSON:" << error.errorString();return 1;}
            if (!jsonDoc.isObject()) {qWarning() << "JSON document is not an object";return 1;}
            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();
            bool uploading = false;
            for (const QString &key : keys) {
                //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                if(key == "CODE" && jsonObj.value(key).toVariant() == 30){//请求上传的文件在服务器已经存在了，不需要再由客户端上传
                    addFileToList(m_filesList, fileinfo->fileName(), sizeTemp);
                    emit filesListChanged();
                    std::cout <<"不需要客户端上传文件" << std::endl;
                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 31){
                    if(!uploading){
                        uploading = true;
                        std::cout <<"需要客户端上传文件" << std::endl;
                        std::cout<< fileNameTemp.toStdString() << "   " << sizeTemp << "  " << md5Temp.toStdString() << std::endl;
                        std::cout << "upload file" << std::endl;
                        QHttpPart filePart;
                        QNetworkRequest request;
                        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
                        request.setRawHeader("Connection", "Keep-Alive");
                        QString contentDisHeader = QString("form-data; filename=\"%1\"; username=\"%2\"; size=%3; md5=\"%4\"").arg(fileNameTemp).arg(m_userName).arg(sizeTemp).arg(md5Temp);
                        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(contentDisHeader));
                        std::cout<< "start stbodt" << std::endl;
                        filePart.setBodyDevice(file);
                        std::cout<< "end stbodt" << std::endl;
                        request.setUrl(QUrl(m_ipAddress + "upload"));
                        QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);// 创建一个QHttpMultiPart对象作为POST请求的主体
                        multiPart->append(filePart);
                        QNetworkReply *reply  = m_manager->post(request, multiPart);
                        std::cout<< "after post" << std::endl;
                        multiPart->setParent(reply); // 设置multiPart的父对象为reply，确保在reply完成后multiPart会被自动释放
                        QObject::connect(reply, &QNetworkReply::uploadProgress, [](qint64 bytesSent, qint64 bytesTotal) {
                            if( bytesTotal != 0){
                                std::cout << "Upload progress:" << bytesSent << "/" << bytesTotal << std::endl;
                            }

                        });
                        QObject::connect(reply, &QNetworkReply::finished, [=]() {
                            if (reply->error() == QNetworkReply::NoError) {

                                QByteArray responseData = reply->readAll();
                                // 释放资源
                                reply->deleteLater();
                                QJsonParseError error;
                                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);
                                if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {qWarning() << "Failed to parse JSON:" << error.errorString();return 1;}

                                if (!jsonDoc.isObject()) {qWarning() << "JSON document is not an object";return 1;}
                                QJsonObject jsonObj = jsonDoc.object();
                                QStringList keys = jsonObj.keys();
                                std::cout << "read All size : " << responseData.size() << std::endl;
                                for (const QString &key : keys) {
                                    //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                                    if(key == "CODE" && jsonObj.value(key).toVariant() == 40){//上传文件成功
                                        QVariantMap tmp;
                                        tmp.insert("file_name", fileNameTemp);
                                        tmp.insert("size", sizeTemp);
                                        qDebug() << "upload success";
                                        m_filesList.append(tmp);
                                        emit filesListChanged();
                                        file->close();
                                    }else if(key == "CODE" && jsonObj.value(key).toVariant() == 41){//上传文件失败
                                        std::cout << "upload failed" << std::endl;
                                        file->close();
                                    }
                                }
                            } else {
                                std::cout << "upload file Error: " << reply->error();
                                // 释放资源
                                //reply->deleteLater();
                                return 1;
                            }


                        });
                    }

                }

            }
        } else {
            std::cout << "requestUpload failed:" << reply->errorString().toStdString() << std::endl;
            // 释放资源
            reply->deleteLater();
            return 1;
        }

    });
    return 0;
}

void MyNetworkManager::addFileToList(QVariantList &fileList, const QString &fileName, const int &size){
    QVariantMap fileMap;
    fileMap["file_name"] = fileName;
    fileMap["size"] = size;
    m_filesList.append(fileMap);
}

QVariant MyNetworkManager::getFilesList() const{
    return QVariant::fromValue(m_filesList);
}

int MyNetworkManager::downLoadSingleFile(const QString &filename){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "download"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("username", m_userName);
    postData.addQueryItem("file_name", filename);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request download success";

            QByteArray data = reply->readAll();
            emit downloadForQml(data);
            //            //std::cout<< data.toStdString() << std::endl;
            //            if(types == "文 档"){
            //                emit downloadForQml(data);
            //            }else if(types == "图 片"){
            //                QString imageData = QString::fromLatin1(data.toBase64());
            //                emit imageReady(imageData);
            //            }else if(types == "音 频"){
            //                QString musicData = QString::fromLatin1(data.toBase64());
            //                emit musicReady(musicData);
            //            }
        }else{
            qDebug() << "request download failed";
        }
        reply->deleteLater();
    });
    return 0;
}

int MyNetworkManager::downLoadSingleFile(const QString &filename, const QString &filePath){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "download"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("username", m_userName);
    postData.addQueryItem("file_name", filename);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request download success";

            QByteArray data = reply->readAll();
            qDebug() << data.size();
            QString localFilePath = filePath + "/" + filename;

            QFile *file = new QFile(localFilePath);
            if(file->open(QIODevice::WriteOnly)){
                file->write(data);
                file->close();
                qDebug() << "File: " << filename << " save to " << localFilePath;
            }else{
                qDebug() << "File: " << filename << " not save to " << localFilePath;
            }
        }else{
            qDebug() << "request download failed";
        }
        reply->deleteLater();
    });
    return 0;
}

int MyNetworkManager::downLoadSingleFile(const QString &username, const QString &filename, const QString &filePath){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "download"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("username", username);
    postData.addQueryItem("file_name", filename);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request download success";

            QByteArray data = reply->readAll();
            qDebug() << data.size();
            QString localFilePath = filePath + "/" + filename;

            QFile *file = new QFile(localFilePath);
            if(file->open(QIODevice::WriteOnly)){
                file->write(data);
                file->close();
                qDebug() << "File: " << filename << " save to " << localFilePath;
            }else{
                qDebug() << "File: " << filename << " not save to " << localFilePath;
            }
        }else{
            qDebug() << "request download failed";
        }
        reply->deleteLater();
    });
    return 0;
}

int MyNetworkManager::downLoadSingleFile(const QString &filename, const QString &username, const int &type){
    //    QFileInfo* fileInfo = new QFileInfo(filename);
    //    QString extension = fileInfo->suffix().toLower();
    //    QString types;
    //    if ((extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "gif") ){
    //       types = "图 片";
    //    }else if ((extension == "doc" || extension == "docx" || extension == "pdf" || extension == "txt") ){
    //       types = "文 档";
    //    }else if ((extension == "mp4" || extension == "avi" || extension == "mov" || extension == "wmv") ){
    //       types = "视 频";
    //    }else if ((extension == "mp3" || extension == "wav" || extension == "flac" || extension == "aac") ) {
    //       types = "音 频";
    //    }else{
    //       types = "其 他";
    //       emit canntPreview();
    //       return -1;
    //    }
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "download"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("username", username);
    postData.addQueryItem("file_name", filename);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request download success";

            QByteArray data = reply->readAll();
            //std::cout<< data.toStdString() << std::endl;
            //if(types == "文 档"){
            if(type == 1){//"read only"
                emit downloadForQmlOther(data);
            }else if(type == 0){
                emit downloadForQml(data);
            }else if(type == 3){
                emit downloadForQmlOther3(data);
            }

            //            }else if(types == "图 片"){
            //                QString imageData = QString::fromLatin1(data.toBase64());
            //                emit imageReady(imageData);
            //            }else if(types == "音 频"){
            //                QString musicData = QString::fromLatin1(data.toBase64());
            //                emit musicReady(musicData);
            //            }
        }else{
            qDebug() << "request download failed";
        }
        reply->deleteLater();
    });
    return 0;
}

int MyNetworkManager::downLoadFromSharing(const QString &uuid){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "downloadSharing"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("uuid",uuid);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request downloadsharing success";
            //处理登录成功
            QByteArray responseData = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);

            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse JSON:" << error.errorString();

            }

            if (!jsonDoc.isObject()) {
                qWarning() << "JSON document is not an object";

            }

            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();
            for (const QString &key : keys) {

                if(key == "CODE" && jsonObj.value(key).toVariant() == 90){//
                    QString username = jsonObj["username"].toString();
                    QStringList filesList = parseFileNames(jsonObj);
                    emit getUuidFilesListAndUserName(filesList, username);
                    //                    for(const auto& str: filesList){
                    //                        downLoadSingleFile(username, str,"/root/chen");
                    //                    }

                    qDebug() << "request downloadSharing success";
                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 91){
                    emit invalidUuid();
                    qDebug() << "request downloadSharing failed";
                }

            }

        }else{
            qDebug() << "request downloadsharing  failed:" <<reply->errorString();
        }
        reply->deleteLater();
    });
    return 0;
}

QStringList MyNetworkManager::parseFileNames(const QJsonObject &jsonObj){
    QStringList tmp;
    QJsonArray filesArray = jsonObj["files"].toArray();
    for(const QJsonValue& fileValue : filesArray){
        QJsonObject fileObj = fileValue.toObject();
        QString fileName  = fileObj["file_name"].toString();
        tmp.append(fileName);
    }
    return tmp;
}
int MyNetworkManager::deleteSingleFile(const QString &filename){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "delete"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("username", m_userName);
    postData.addQueryItem("file_name", filename);

    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request delete success";
            QByteArray responseData = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);

            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse JSON:" << error.errorString();

            }

            if (!jsonDoc.isObject()) {
                qWarning() << "JSON document is not an object";

            }

            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();
            //qDebug() << "Parsed keys and values:";
            for (const QString &key : keys) {
                //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                if(key == "CODE" && jsonObj.value(key).toVariant() == 60){//删除成功
                    qDebug() << "delete success";
                    removeFileFromList(m_filesList, filename);
                    emit filesListChanged();

                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 61){
                    qDebug() << "delete falied";


                }
            }
        }else{
            qDebug() << "request delete failed";
        }
        reply->deleteLater();
    });
    return 0;
}

void MyNetworkManager::removeFileFromList(QVariantList &fileList, const QString &fileName){
    for (int i = 0; i < fileList.size(); ++i) {
        QVariantMap fileMap = fileList[i].toMap();
        if (fileMap["file_name"].toString() == fileName) {
            fileList.removeAt(i);
            qDebug() << "File with name" << fileName << "removed from the list.";
            return;
        }
    }
}

int MyNetworkManager::renameFileName(const QString &oldFileName, const QString &newFileName){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "rename"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Connection", "Keep-Alive");

    QUrlQuery postData;
    postData.addQueryItem("username", m_userName);
    postData.addQueryItem("old_file_name", oldFileName);
    postData.addQueryItem("new_file_name", newFileName);
    QNetworkReply *reply = m_manager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request rename success";
            QByteArray responseData = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);

            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse JSON:" << error.errorString();

            }
            if (!jsonDoc.isObject()) {
                qWarning() << "JSON document is not an object";

            }
            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();
            //qDebug() << "Parsed keys and values:";
            for (const QString &key : keys) {
                //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                if(key == "CODE" && jsonObj.value(key).toVariant() == 70){//重命名成功
                    qDebug() << "rename success";
                    for(int i = 0; i < m_filesList.length(); ++i){
                        QVariantMap fileMap = m_filesList[i].toMap();
                        if(fileMap["file_name"].toString() == oldFileName){
                            fileMap["file_name"] = newFileName;
                            m_filesList.replace(i, fileMap);
                            qDebug() << "OldFileName: " << oldFileName << "to NewFileName: " << newFileName;
                            emit renameResult(true);
                            break;
                        }
                    }
                    emit filesListChanged();

                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 71){
                    emit renameResult(false);
                    qDebug() << "rename falied";
                }
            }
        }else{
            qDebug() << "request delete failed";
        }
        reply->deleteLater();
    });
    return 0;
}

int MyNetworkManager::shareFiles(const QStringList &shareFilesList, const int &day){
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "share"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "Keep-Alive");

    QJsonObject requestData;
    requestData["username"] = m_userName;
    requestData["keep_time"] = day;
    QJsonArray filesArray;
    for (const auto& file : shareFilesList) {
        std::cout<< file.toStdString() << std::endl;
        filesArray.append(file);
    }
    requestData["shareFilesList"] = filesArray;

    QJsonDocument doc(requestData);
    QByteArray jsonData = doc.toJson();

    QNetworkReply *reply = m_manager->post(request, jsonData);

    QObject::connect(reply, &QNetworkReply::finished, [=](){
        if(reply->error() == QNetworkReply::NoError){
            qDebug() << "request sharing success";

            QByteArray responseData = reply->readAll();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);

            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse JSON:" << error.errorString();

            }

            if (!jsonDoc.isObject()) {
                qWarning() << "JSON document is not an object";

            }

            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();
            for(const QString &key : keys){
                if(key == "CODE" && jsonObj.value(key).toVariant() == 80){
                    QString uuid = jsonObj.value("uuid").toString();
                    //downLoadFromSharing(uuid);
                    m_sharingList.insert(uuid, shareFilesList);
                    m_uuidList.append(uuid);
                    emit getSharedUUidList(m_uuidList);
                    emit sharedFilesListChanged(shareFilesList, uuid);
                    qDebug() << "share success";
                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 81){
                    qDebug() << "share failed";
                }

            }
        }else{
            qDebug() << "request sharing failed";
        }
        reply->deleteLater();
    });
    return 0;
}

//int MyNetworkManager::filterFiles(const QString &types){
//    m_FilterFilesList.clear();
//    if(types == "默 认"){
//        emit filesListChanged();
//        return -1;
//    }
//    QVariantMap fileMap;
//    QString fileName;
//    QFileInfo *fileInfo;
//    QString extension;
//    for(int i = 0; i < m_filesList.size(); ++i){
//        fileMap = m_filesList[i].toMap();
//        fileName =  fileMap["file_name"].toString();
//        fileInfo = new QFileInfo(fileName);
//        extension = fileInfo->suffix().toLower();
//        if ((extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "gif") && types == "图 片"){
//            m_FilterFilesList.append(fileMap);
//            continue;
//        }else if ((extension == "doc" || extension == "docx" || extension == "pdf" || extension == "txt") && types == "文 档"){
//            m_FilterFilesList.append(fileMap);
//            continue;
//        }else if ((extension == "mp4" || extension == "avi" || extension == "mov" || extension == "wmv") && types == "视 频"){
//            m_FilterFilesList.append(fileMap);
//            continue;
//        }else if ((extension == "mp3" || extension == "wav" || extension == "flac" || extension == "aac") && types == "音 频") {
//            m_FilterFilesList.append(fileMap);
//            continue;
//        }else if(types == "其 他"){
//            m_FilterFilesList.append(fileMap);
//        }
//    }

//    emit filterFilesListChanged();
//    return 0;
//}

//int MyNetworkManager::keyWordsFilterFiles(const QString &keyWords){
//    m_FilterFilesList.clear();
//    if(keyWords == ""){
//        emit filesListChanged();
//        return -1;
//    }
//    QVariantMap fileMap;
//    QString fileName;
//    for(int i = 0; i < m_filesList.size(); ++i){
//        fileMap = m_filesList[i].toMap();
//        fileName = fileMap["file_name"].toString();
//        if(fileName.contains(keyWords)){
//            m_FilterFilesList.append(fileMap);
//        }
//    }
//    emit filterFilesListChanged();
//    return 0;
//}

//QVariant MyNetworkManager::getFilterFilesList()const{
//    return QVariant::fromValue(m_FilterFilesList);
//}

int MyNetworkManager::getSharedFilesListFromUuid(const QString &uuid){
    if(m_sharingList.contains(uuid)){
        m_sharedFilesList.clear();
        QVariantList fileList = m_sharingList[uuid].toList();
        for(const auto& filename : fileList){
            //qDebug() << filename.toString();
            m_sharedFilesList.append(filename.toString());
        }
        emit getSharedFilesList(m_sharedFilesList);
    }
    return 0;
}

int MyNetworkManager::saveChangedText(const QString &filename, const QString &filecontent){
    QByteArray content = filecontent.toUtf8();
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(content);
    QByteArray md5 = hash.result();
    QString md5Temp = md5.toHex();
    int sizeTemp = content.size();
    QString fileNameTemp = filename;
    QString md5SizeJsonRequest = QString("{ \"md5\":\"%1\",\"size\":%2,\"file_name\":\"%3\",\"username\":\"%4\"}").arg(md5Temp).arg(sizeTemp).arg(fileNameTemp).arg(m_userName);
    QNetworkRequest request;
    request.setUrl(QUrl(m_ipAddress + "requestUpload"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "Keep-Alive");
    QNetworkReply *reply = m_manager->post(request, md5SizeJsonRequest.toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            std::cout << "request ask  md5_size  success\n";
            QByteArray responseData = reply->readAll();
            // 释放资源
            reply->deleteLater();
            QJsonParseError error;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);
            if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {qWarning() << "Failed to parse JSON:" << error.errorString();return 1;}
            if (!jsonDoc.isObject()) {qWarning() << "JSON document is not an object";return 1;}
            QJsonObject jsonObj = jsonDoc.object();
            QStringList keys = jsonObj.keys();
            bool uploading = false;
            for (const QString &key : keys) {
                //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                if(key == "CODE" && jsonObj.value(key).toVariant() == 30){//请求上传的文件在服务器已经存在了，不需要再由客户端上传
                    addFileToList(m_filesList, fileNameTemp, sizeTemp);
                    emit filesListChanged();
                    std::cout <<"不需要客户端上传文件" << std::endl;
                }else if(key == "CODE" && jsonObj.value(key).toVariant() == 31){
                    if(!uploading){
                        uploading = true;
                        std::cout <<"需要客户端上传文件" << std::endl;
                        std::cout<< fileNameTemp.toStdString() << "   " << sizeTemp << "  " << md5Temp.toStdString() << std::endl;
                        std::cout << "upload file" << std::endl;
                        QHttpPart filePart;
                        QNetworkRequest request;
                        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
                        request.setRawHeader("Connection", "Keep-Alive");
                        QString contentDisHeader = QString("form-data; filename=\"%1\"; username=\"%2\"; size=%3; md5=\"%4\"; savechangedaction=1").arg(fileNameTemp).arg(m_userName).arg(sizeTemp).arg(md5Temp);
                        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(contentDisHeader));
                        std::cout<< "start stbodt" << std::endl;
                        filePart.setBody(content);
                        std::cout<< "end stbodt" << std::endl;
                        request.setUrl(QUrl(m_ipAddress + "upload"));
                        QHttpMultiPart *multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
                        multipart->append(filePart);
                        QNetworkReply *reply  = m_manager->post(request, multipart);
                        std::cout<< "after post" << std::endl;
                        multipart->setParent(reply); // 设置multiPart的父对象为reply，确保在reply完成后multiPart会被自动释放
                        QObject::connect(reply, &QNetworkReply::uploadProgress, [](qint64 bytesSent, qint64 bytesTotal) {
                            if( bytesTotal != 0){
                                std::cout << "Upload progress:" << bytesSent << "/" << bytesTotal << std::endl;
                            }

                        });
                        QObject::connect(reply, &QNetworkReply::finished, [=]() {
                            if (reply->error() == QNetworkReply::NoError) {

                                QByteArray responseData = reply->readAll();
                                // 释放资源
                                reply->deleteLater();
                                QJsonParseError error;
                                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);
                                if (jsonDoc.isNull() || error.error != QJsonParseError::NoError) {qWarning() << "Failed to parse JSON:" << error.errorString();return 1;}

                                if (!jsonDoc.isObject()) {qWarning() << "JSON document is not an object";return 1;}
                                QJsonObject jsonObj = jsonDoc.object();
                                QStringList keys = jsonObj.keys();
                                std::cout << "read All size : " << responseData.size() << std::endl;
                                for (const QString &key : keys) {
                                    //qDebug() << key << ":" << jsonObj.value(key).toVariant();
                                    if(key == "CODE" && jsonObj.value(key).toVariant() == 40){//上传文件成功
                                        for(int i = 0; i < m_filesList.length(); ++i){
                                            QVariantMap fileMap = m_filesList[i].toMap();
                                            if(fileMap["file_name"].toString() == fileNameTemp){
                                                QVariantMap temp;
                                                temp.insert("file_name", fileNameTemp);
                                                temp.insert("size", sizeTemp);
                                                m_filesList.replace(i, temp);
                                                break;
                                            }
                                        }
                                        qDebug() << "upload success";
                                        emit filesListChanged();

                                    }else if(key == "CODE" && jsonObj.value(key).toVariant() == 41){//上传文件失败
                                        std::cout << "upload failed" << std::endl;

                                    }
                                }
                            } else {
                                std::cout << "upload file Error: " << reply->error();
                                // 释放资源
                                reply->deleteLater();
                                return 1;
                            }


                        });
                    }

                }

            }
        } else {
            std::cout << "requestUpload failed:" << reply->errorString().toStdString() << std::endl;
            // 释放资源
            reply->deleteLater();
            return 1;
        }

    });
    return 0;
}
