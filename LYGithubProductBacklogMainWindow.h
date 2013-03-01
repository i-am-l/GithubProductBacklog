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
	/// Reimplements the closeEvent to ask the mainWidget to ensure that its windows have been closed then calls the parent routine
	void closeEvent(QCloseEvent *);

protected:
	/// Holds the central widget
	LYGithubProductBacklogCentralWidget *mainWidget_;

	/// Holds the menuBar pointer
	QMenuBar *menuBar_;
	/// Holds the help menu pointer (only action is to launch the status log view)
	QMenu *helpMenu_;
};

#endif // LYGITHUBPRODUCTBACKLOGMAINWINDOW_H
