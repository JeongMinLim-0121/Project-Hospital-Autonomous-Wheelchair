QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    account_admin.cpp \
    kiosk_back.cpp \
    kiosk_container.cpp \
    kiosk_login.cpp \
    kiosk_main.cpp \
    kiosk_search.cpp \
    kiosk_wheel.cpp \
    main.cpp \
    mainwindow.cpp \
    search_medical.cpp \
    tab_admin.cpp \
    tab_medical.cpp \
    wheelchair_admin.cpp \
    wheelchair_map.cpp \
    wheelchair_medical.cpp \
    keyboard.cpp \
    login.cpp \
    account_manage.cpp \
    database_manager.cpp

HEADERS += \
    account_admin.h \
    add_robot_dialog.h \
    kiosk_back.h \
    kiosk_container.h \
    kiosk_login.h \
    kiosk_main.h \
    kiosk_search.h \
    kiosk_wheel.h \
    live_monitor_dialog.h \
    mainwindow.h \
    search_medical.h \
    tab_admin.h \
    tab_medical.h \
    wheelchair_admin.h \
    wheelchair_map.h \
    wheelchair_medical.h \
    keyboard.h \
    login.h \
    account_manage.h \
    database_manager.h

FORMS += \
    account_admin.ui \
    kiosk_back.ui \
    kiosk_container.ui \
    kiosk_login.ui \
    kiosk_main.ui \
    kiosk_search.ui \
    kiosk_wheel.ui \
    mainwindow.ui \
    search_medical.ui \
    tab_admin.ui \
    tab_medical.ui \
    wheelchair_admin.ui \
    wheelchair_map.ui \
    wheelchair_medical.ui \
    keyboard.ui \
    login.ui \
    account_manage.ui \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    icons/wifi.png

RESOURCES += \
    resources.qrc
