#ifndef LYGITHUBPRODUCTBACKLOGMAINWINDOW_H
#define LYGITHUBPRODUCTBACKLOGMAINWINDOW_H

#include <QtGui/QMainWindow>

#include "LYGithubProductBacklogCentralWidget.h"

class LYGithubProductBacklogMainWindow : public QMainWindow
{
Q_OBJECT

public:
	/// Constructor makes new main window
	LYGithubProductBacklogMainWindow(QWidget *parent = 0);
	~LYGithubProductBacklogMainWindow();

protected slots:
	/// Handles a request from the main widget to quit the application
	void onRequestQuit();

protected:
	/// Holds the central widget
	LYGithubProductBacklogCentralWidget *mainWidget_;
};

#endif // LYGITHUBPRODUCTBACKLOGMAINWINDOW_H
