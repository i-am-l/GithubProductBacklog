#ifndef LYGITHUBPRODUCTBACKLOGMAINWINDOW_H
#define LYGITHUBPRODUCTBACKLOGMAINWINDOW_H

#include <QtGui/QMainWindow>

#include "LYGithubProductBacklogCentralWidget.h"

class LYGithubProductBacklogMainWindow : public QMainWindow
{
Q_OBJECT

public:
	/// Constructor makes new main window
	LYGithubProductBacklogMainWindow(const QString &username = "i-am-l", const QString &repository = "i-am-l/GithubProductBacklog", QWidget *parent = 0);
	~LYGithubProductBacklogMainWindow();

protected slots:
	/// Handles a request from the main widget to quit the application
	void onRequestQuit();
	/// Handles a request from the main widget to display message to the status bar
	void onRequestStatusBarMessage(const QString &message, int timeout);

protected:
	/// Holds the central widget
	LYGithubProductBacklogCentralWidget *mainWidget_;
};

#endif // LYGITHUBPRODUCTBACKLOGMAINWINDOW_H
