#ifndef LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H
#define LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H

#include <QListView>
#include <QTreeView>
#include <QPushButton>

#include "LYGithubProductBacklog.h"

#include <QDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QBoxLayout>
#include <QLabel>

class LYGithubProductBacklogAuthenticationView : public QDialog
{
Q_OBJECT
public:
	/// Constructor
	LYGithubProductBacklogAuthenticationView(QWidget *parent = 0);

public slots:
	/// Notifies this widget that authentication failed or succeeded
	void setAuthenticated(bool authenticated);

signals:
	/// Emitted when this widget has submitted new authentication information
	void submitAuthenticationInformation(const QString &username, const QString &password, const QString &repository);

protected slots:
	/// Handles any editting to the three line edits
	void onLineEditsEditted();
	/// Handles when the submission button is clicked (or enter is pressed)
	void onSubmitButtonClicked();
	/// Handles setting the initial state for the progress bar and label for server interaction
	void prepareServerInteractionWidgets();

protected:
	/// Line edit for username
	QLineEdit *usernameLineEdit_;
	/// Line edit for password
	QLineEdit *passwordLineEdit_;
	/// Line edit for repository
	QLineEdit *repositoryLineEdit_;

	/// Push button for submit
	QPushButton *submitButton_;
	/// Progress bar to show interaction is happening
	QProgressBar *serverInteractionProgressBar_;
	/// Label for interaction progress
	QLabel *serverInteractionLabel_;

	/// Main vertical layout
	QVBoxLayout vl_;
};

class LYGithubProductBacklogCentralWidget : public QWidget
{
Q_OBJECT

public:
	/// Constructor
	LYGithubProductBacklogCentralWidget(QWidget *parent = 0);

signals:
	void requestQuit();

protected slots:
	/// Handles the communication from the authentication view regarding new username, password, and repository
	void onSubmitAuthenticationInformationAvailable(const QString &username, const QString &password, const QString &repository);
	/// Handles the authenticated signal from the github manager
	void onAuthenticated(bool authenticated);

	/// Handles interaction with the Upload Changes button
	void onUploadChangesButtonClicked();

	/// Enables and disables the uploadChangesButton based on whether the list has been modified by the user
	void onActiveChangesChanged(bool hasActiveChanges);

	void onDetectedMissingIssues(QList<int> missingIssuesNumbers);
	void onDetectedClosedIssuesWithoutChildren(QList<int> closedIssuesWithoutChildren);
	void onDetectedClosedIssuesWithChildren(QList<int> closedIssuesWithChildren);

protected:
	/// View for the tree model coming from the product backlog
	QTreeView *treeView_;
	/// Push button to upload changes that have been made
	QPushButton *uploadChangesButton_;

	/// The product backlog model pointer
	LYGithubProductBacklog *productBacklog_;
	/// The authentication window pointer
	LYGithubProductBacklogAuthenticationView *authenticationView_;
};

#endif // LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H
