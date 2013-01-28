#ifndef LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H
#define LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H

#include <QListView>
#include <QPushButton>

#include "LYGithubProductBacklog.h"

#include <QLineEdit>

class LYGithubProductBacklogAuthenticationView : public QWidget
{
Q_OBJECT
public:
	LYGithubProductBacklogAuthenticationView(QWidget *parent = 0);

signals:
	void submitAuthenticationInformation(const QString &username, const QString &password, const QString &repository);

protected slots:
	void onLineEditsEditted();
	void onSubmitButtonClicked();

protected:
	/// Line edit for username
	QLineEdit *usernameLineEdit_;
	/// Line edit for password
	QLineEdit *passwordLineEdit_;
	/// Line edit for repository
	QLineEdit *repositoryLineEdit_;

	/// Push button for submit
	QPushButton *submitButton_;
};

class LYGithubProductBacklogCentralWidget : public QWidget
{
Q_OBJECT

public:
	LYGithubProductBacklogCentralWidget(QWidget *parent = 0);

protected slots:
	void onSubmitAuthenticationInformationAvailable(const QString &username, const QString &password, const QString &repository);

	void onUploadChangesButtonClicked();

	void onActiveChangesChanged(bool hasActiveChanges);

protected:
	/// View for the list model coming from the product backlog
	QListView *listView_;
	/// Push button to upload changes that have been made
	QPushButton *uploadChangesButton_;

	/// The product backlog model pointer
	LYGithubProductBacklog *productBacklog_;
	/// The authentication window pointer
	LYGithubProductBacklogAuthenticationView *authenticationView_;
};

#endif // LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H
