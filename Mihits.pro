QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ckernel.cpp \
    fiveinlinezone.cpp \
    gamepackageupdater.cpp \
    logindialog.cpp \
    main.cpp \
    dialog.cpp \
    record.cpp \
    recordlist.cpp \
    roomdialog.cpp \
    roomitem.cpp

HEADERS += \
    ckernel.h \
    dialog.h \
    fiveinlinezone.h \
    gamepackageupdater.h \
    logindialog.h \
    record.h \
    recordlist.h \
    roomdialog.h \
    roomitem.h

FORMS += \
    dialog.ui \
    fiveinlinezone.ui \
    logindialog.ui \
    record.ui \
    recordlist.ui \
    roomdialog.ui \
    roomitem.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(./netapi/netapi.pri)
include(./md5/md5.pri)
include(./fiveinline/fiveinline.pri)

INCLUDEPATH += ./netapi/net
INCLUDEPATH += ./netapi/mediator
INCLUDEPATH += ./md5/
INCLUDEPATH += ./fiveinline/

RESOURCES += \
    resource.qrc

DISTFILES += \
    bq/000.png \
    bq/001.png \
    bq/002.png \
    bq/003.png \
    bq/004.png \
    bq/005.png \
    bq/006.png \
    bq/007.png \
    bq/008.png \
    bq/009.png \
    bq/010.png \
    bq/011.png \
    bq/012.png \
    bq/013.png \
    bq/014.png \
    bq/015.png \
    bq/016.png \
    bq/017.png \
    bq/018.png \
    bq/019.png \
    bq/020.png \
    bq/021.png \
    bq/022.png \
    bq/023.png \
    bq/024.png \
    bq/025.png \
    bq/026.png \
    bq/027.png \
    bq/028.png \
    bq/029.png \
    bq/030.png \
    bq/031.png \
    bq/032.png \
    bq/033.png \
    bq/034.png \
    bq/035.png \
    bq/036.png \
    bq/037.png \
    bq/038.png \
    bq/039.png \
    bq/040.png \
    bq/041.png \
    bq/042.png \
    bq/043.png \
    bq/044.png \
    bq/045.png \
    bq/046.png \
    bq/047.png \
    bq/048.png \
    bq/049.png \
    bq/050.png \
    bq/051.png \
    bq/052.png \
    bq/053.png \
    bq/054.png \
    bq/055.png \
    bq/056.png \
    bq/057.png \
    bq/058.png \
    bq/059.png \
    bq/060.png \
    bq/061.png \
    bq/062.png \
    bq/063.png \
    bq/064.png \
    bq/065.png \
    bq/066.png \
    bq/067.png \
    bq/068.png \
    bq/069.png \
    bq/070.png \
    bq/071.png \
    bq/072.png \
    bq/073.png \
    bq/074.png \
    bq/075.png \
    bq/076.png \
    bq/077.png \
    bq/078.png \
    bq/079.png \
    bq/080.png \
    bq/081.png \
    bq/082.png \
    bq/083.png \
    bq/084.png \
    bq/085.png \
    bq/086.png \
    bq/087.png \
    bq/088.png \
    bq/089.png \
    bq/090.png \
    bq/091.png \
    bq/1.png \
    bq/10.png \
    bq/11.png \
    bq/12.png \
    bq/13.png \
    bq/14.png \
    bq/15.png \
    bq/16.png \
    bq/17.png \
    bq/18.png \
    bq/19.png \
    bq/2.png \
    bq/20.png \
    bq/21.png \
    bq/22.png \
    bq/23.png \
    bq/24.png \
    bq/25.png \
    bq/26.png \
    bq/27.png \
    bq/28.png \
    bq/29.png \
    bq/3.png \
    bq/30.png \
    bq/31.png \
    bq/32.png \
    bq/33.png \
    bq/34.png \
    bq/35.png \
    bq/36.png \
    bq/37.png \
    bq/38.png \
    bq/39.png \
    bq/4.png \
    bq/40.png \
    bq/41.png \
    bq/42.png \
    bq/43.png \
    bq/44.png \
    bq/45.png \
    bq/46.png \
    bq/47.png \
    bq/48.png \
    bq/49.png \
    bq/5.png \
    bq/50.png \
    bq/51.png \
    bq/52.png \
    bq/53.png \
    bq/54.png \
    bq/55.png \
    bq/57.png \
    bq/58.png \
    bq/59.png \
    bq/6.png \
    bq/60.png \
    bq/61.png \
    bq/62.png \
    bq/63.png \
    bq/64.png \
    bq/65.png \
    bq/66.png \
    bq/67.png \
    bq/68.png \
    bq/69.png \
    bq/7.png \
    bq/70.png \
    bq/71.png \
    bq/72.png \
    bq/73.png \
    bq/74.png \
    bq/75.png \
    bq/76.png \
    bq/77.png \
    bq/78.png \
    bq/79.png \
    bq/8.png \
    bq/80.png \
    bq/81.png \
    bq/82.png \
    bq/83.png \
    bq/84.png \
    bq/85.png \
    bq/86.png \
    bq/87.png \
    bq/88.png \
    bq/89.png \
    bq/9.png \
    bq/90.png \
    bq/91.png \
    bq/92.png \
    bq/93.png \
    bq/94.png \
    bq/95.png \
    bq/96.png \
    bq/97.png \
    bq/98.png \
    bq/back.png \
    bq/forward.png \
    bq/quit.png \
    images/1.jpg \
    images/GameHome.png \
    images/GameHome1.png \
    images/Help.png \
    images/background.jpg \
    images/bold.png \
    images/cancel.png \
    images/clear.png \
    images/color.png \
    images/down.png \
    images/file.ico \
    images/folders.png \
    images/font.png \
    images/groupChat 2.png \
    images/hat.png \
    images/history.png \
    images/ic_max_huanyuan.png \
    images/ic_max_zuida.png \
    images/ic_sysmen.png \
    images/icon1.png \
    images/icon2.png \
    images/message.ico \
    images/message.png \
    images/ok.png \
    images/q151020-01.png \
    images/qq151217-02.png \
    images/quit.png \
    images/register.png \
    images/right.png \
    images/save.png \
    images/searchbox_button.png \
    images/send.png \
    images/sett.png \
    images/share.png \
    images/style.png \
    images/tuer.png \
    images/under.png \
    tx/0.png \
    tx/1.png \
    tx/10.png \
    tx/11.png \
    tx/12.png \
    tx/13.png \
    tx/14.png \
    tx/15.png \
    tx/16.png \
    tx/17.png \
    tx/18.png \
    tx/19.png \
    tx/2.png \
    tx/20.png \
    tx/21.png \
    tx/22.png \
    tx/23.png \
    tx/24.png \
    tx/25.png \
    tx/26.png \
    tx/27.png \
    tx/28.png \
    tx/29.png \
    tx/3.png \
    tx/30.png \
    tx/31.png \
    tx/32.png \
    tx/33.png \
    tx/34.png \
    tx/35.png \
    tx/36.jpg \
    tx/4.png \
    tx/5.png \
    tx/6.png \
    tx/7.png \
    tx/8.png \
    tx/9.png
