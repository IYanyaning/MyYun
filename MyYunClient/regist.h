#ifndef REGIST_H
#define REGIST_H
#include "mynetworkmanager.h"
#include <QString>
#include <QObject>
#include <QRegularExpression>
class Regist{
    Q_OBJECT
public:
    static Regist* instance();

};

#endif // REGIST_H
