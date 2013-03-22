#-------------------------------------------------
#
# Project created by QtCreator 2013-01-19T10:20:45
#
#-------------------------------------------------

QT       += core gui network

TARGET = GithubProductBacklog
TEMPLATE = app


SOURCES += main.cpp\
	LYGithubProductBacklogMainWindow.cpp \
	LYGithubManager.cpp \
	LYGithubProductBacklog.cpp \
	LYGithubProductBacklogCentralWidget.cpp \
	qjson/json_parser.cc \
	qjson/json_scanner.cpp \
	qjson/parser.cpp \
	qjson/parserrunnable.cpp \
	qjson/qobjecthelper.cpp \
	qjson/serializer.cpp \
	qjson/serializerrunnable.cpp \
    LYProductBacklogModel.cpp \
    LYConnectionQueueObject.cpp \
    LYConnectionQueue.cpp \
    LYGithubProductBacklogStatusLog.cpp

HEADERS  += LYGithubProductBacklogMainWindow.h \
	LYGithubManager.h \
	LYGithubProductBacklog.h \
	LYGithubProductBacklogCentralWidget.h \
	qjson/json_parser.hh \
	qjson/json_scanner.h \
	qjson/location.hh \
	qjson/parser_p.h \
	qjson/parser.h \
	qjson/parserrunnable.h \
	qjson/position.hh \
	qjson/qjson_debug.h \
	qjson/qjson_export.h \
	qjson/qobjecthelper.h \
	qjson/serializer.h \
	qjson/serializerrunnable.h \
	qjson/stack.hh \
    LYProductBacklogModel.h \
    LYConnectionQueueObject.h \
    LYConnectionQueue.h \
    LYGithubProductBacklogStatusLog.h

RESOURCES += \
    icons.qrc



















