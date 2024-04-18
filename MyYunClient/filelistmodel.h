#ifndef FILELISTMODEL_H
#define FILELISTMODEL_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractListModel>
#include <QList>
#include <QDebug>

#include "data.h"

class FileListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum FileRoles {
        FileNameRole = Qt::UserRole + 1,
        FileSizeRole,
        BytesTransferredRole,
        StatusRole
    };

    explicit FileListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {

    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override ;

    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void addFileInfo(const QString &fileName, qint64 fileSize, qint64 bytesTransferred, const QString &status) ;
    Q_INVOKABLE void setFiles(const QList<FileInfo>& files);



signals:
    void filesChanged();


private:


    QList<FileInfo> m_files;
};

#endif // FILELISTMODEL_H
