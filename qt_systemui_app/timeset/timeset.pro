QT += quick remoteobjects network

SOURCES += main.cpp

RESOURCES += qml.qrc

QML_IMPORT_PATH =
QML_DESIGNER_IMPORT_PATH =

qnx: target.path = /tmp/\$\/bin
else: unix:!android: target.path = /opt/ui/src/apps
!isEmpty(target.path): INSTALLS += target

include(../client/client.pri)
INCLUDEPATH += ../client

HEADERS += timecontrol.h
