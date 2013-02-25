#include "LYGithubProductBacklogMainWindow.h"

#include <QStatusBar>

LYGithubProductBacklogMainWindow::LYGithubProductBacklogMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	mainWidget_ = new LYGithubProductBacklogCentralWidget();

	setCentralWidget(mainWidget_);
	connect(mainWidget_, SIGNAL(requestQuit()), this, SLOT(onRequestQuit()));
	connect(mainWidget_, SIGNAL(requestStatusBarMessage(QString,int)), this, SLOT(onRequestStatusBarMessage(QString,int)));
}

LYGithubProductBacklogMainWindow::~LYGithubProductBacklogMainWindow()
{

}

void LYGithubProductBacklogMainWindow::onRequestQuit(){
	close();
}

void LYGithubProductBacklogMainWindow::onRequestStatusBarMessage(const QString &message, int timeout){
	statusBar()->showMessage(message, timeout);
}
