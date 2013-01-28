#include "LYGithubProductBacklogMainWindow.h"

LYGithubProductBacklogMainWindow::LYGithubProductBacklogMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	mainWidget_ = new LYGithubProductBacklogCentralWidget();

	setCentralWidget(mainWidget_);
}

LYGithubProductBacklogMainWindow::~LYGithubProductBacklogMainWindow()
{

}


