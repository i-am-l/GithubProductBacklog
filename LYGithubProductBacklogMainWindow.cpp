#include "LYGithubProductBacklogMainWindow.h"

#include <QStatusBar>
#include <QMenuBar>

LYGithubProductBacklogMainWindow::LYGithubProductBacklogMainWindow(const QString &username, const QString &repository, QWidget *parent) :
	QMainWindow(parent)
{
	mainWidget_ = new LYGithubProductBacklogCentralWidget(username, repository);

	setCentralWidget(mainWidget_);
	connect(mainWidget_, SIGNAL(requestQuit()), this, SLOT(onRequestQuit()));
	connect(mainWidget_, SIGNAL(requestStatusBarMessage(QString,int)), this, SLOT(onRequestStatusBarMessage(QString,int)));

	QAction *statugLogViewAction = new QAction("Status Log", this);
	connect(statugLogViewAction, SIGNAL(triggered()), mainWidget_, SLOT(showStatusLog()));

#ifdef Q_WS_MAC
	menuBar_ = new QMenuBar(0);
#else

#endif
	helpMenu_ = menuBar_->addMenu("Help");
	helpMenu_->addAction(statugLogViewAction);
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

#include <QCloseEvent>
void LYGithubProductBacklogMainWindow::closeEvent(QCloseEvent *e){
	mainWidget_->ensureReadyForClose();
	QMainWindow::closeEvent(e);
}
