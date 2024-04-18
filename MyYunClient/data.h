#ifndef DATA_H
#define DATA_H
#include <QGuiApplication>
struct FileInfo {
    QString fileName;
    qint64 fileSize;
    qint64 bytesTransferred;
    QString status;
};

#endif // DATA_H
