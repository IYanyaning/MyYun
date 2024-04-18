#include "filelistmodel.h"


int FileListModel::rowCount(const QModelIndex &parent) const  {
    return m_files.size();
}

QVariant FileListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_files.size())
        return QVariant();

    const FileInfo &fileInfo = m_files.at(index.row());

    switch (role) {
    case FileNameRole:
        return fileInfo.fileName;
    case FileSizeRole:
        return fileInfo.fileSize;
    case BytesTransferredRole:
        return fileInfo.bytesTransferred;
    case StatusRole:
        return fileInfo.status;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FileListModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[FileNameRole] = "file_name";
    roles[FileSizeRole] = "file_size";
    roles[BytesTransferredRole] = "transferred_size";
    roles[StatusRole] = "file_status";
    return roles;
}

void FileListModel::addFileInfo(const QString &fileName, qint64 fileSize, qint64 bytesTransferred, const QString &status){
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_files.append({fileName, fileSize, bytesTransferred, status});
    endInsertRows();
}

void FileListModel::setFiles(const QList<FileInfo> &files){
//    if(m_files != files){
//        beginResetModel();
//        m_files = files;
//        emit filesChanged();
//        endResetModel();
//    }
}
