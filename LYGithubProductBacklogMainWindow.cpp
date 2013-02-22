#include "LYGithubProductBacklogMainWindow.h"

LYGithubProductBacklogMainWindow::LYGithubProductBacklogMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	mainWidget_ = new LYGithubProductBacklogCentralWidget();

	setCentralWidget(mainWidget_);
	connect(mainWidget_, SIGNAL(requestQuit()), this, SLOT(onRequestQuit()));
}

LYGithubProductBacklogMainWindow::~LYGithubProductBacklogMainWindow()
{

}

void LYGithubProductBacklogMainWindow::onRequestQuit(){
	close();
}

