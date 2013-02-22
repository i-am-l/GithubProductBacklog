#ifndef LYGITHUBPRODUCTBACKLOGMAINWINDOW_H
#define LYGITHUBPRODUCTBACKLOGMAINWINDOW_H

#include <QtGui/QMainWindow>

#include "LYGithubProductBacklogCentralWidget.h"

class LYGithubProductBacklogMainWindow : public QMainWindow
{
Q_OBJECT

public:
	LYGithubProductBacklogMainWindow(QWidget *parent = 0);
	~LYGithubProductBacklogMainWindow();

protected slots:
	void onRequestQuit();

protected:
	LYGithubProductBacklogCentralWidget *mainWidget_;
};

#endif // LYGITHUBPRODUCTBACKLOGMAINWINDOW_H
