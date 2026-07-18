QT += remoteobjects
SOURCES += \
        $$PWD/systemuicommonapiclient.cpp

REPC_REPLICA += \
    ../Repcs/systemuicommonapi.rep

HEADERS += \
    $$PWD/systemuicommonapiclient.h

RESOURCES += $$PWD/common.qrc


unix {
    SRC_FILE = $$OUT_PWD/$$TARGET
    DST_FILE = $$OUT_PWD/../ui/src/apps
    QMAKE_POST_LINK += $(STRIP) $$SRC_FILE; cp $$SRC_FILE $$DST_FILE; \
}
