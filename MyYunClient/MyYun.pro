QT += quick qml network widgets multimedia


SOURCES += \
    client.cpp \
    filemanager.cpp \
    fileupdownload.cpp \
    login.cpp \
        main.cpp \
    mynetworkmanager.cpp

resources.prefix = /$${TARGET}
RESOURCES +=\
    qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    client.h \
    filemanager.h \
    fileupdownload.h \
    login.h \
    mynetworkmanager.h
