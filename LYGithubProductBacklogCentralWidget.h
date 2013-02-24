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

class LYGithubProductBacklogAuthenticationView;
class LYGithubProductBacklogSanityCheckView;

class LYGithubProductBacklogCentralWidget : public QWidget
{
Q_OBJECT

public:
	/// Constructor
	LYGithubProductBacklogCentralWidget(QWidget *parent = 0);

signals:
	/// Emitted when we decide to quit, received by the main window
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

	/// Handles opening up the sanity check view if the sanity checks didn't pass on startup
	void onSanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck);

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


#include <QStringListModel>
#include <QGroupBox>
#include <QCheckBox>
class LYGithubProductBacklogSanityCheckView : public QDialog
{
Q_OBJECT
public:
	/// Constructor takes the sanity check flags and string lists for each of the possible failure modes
	LYGithubProductBacklogSanityCheckView(LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck, QStringList missingIssues, QStringList closedIssuesWithoutChildren, QStringList closedIssuesWithChildren, QWidget *parent = 0);

protected slots:
	/// Handles any of the check boxes being toggled
	void onCheckBoxToggled();

protected:
	/// List view for missing issues
	QListView *missingIssuesListView_;
	/// List view for closed (or non-existant) issues in the product backlog without children
	QListView *closedIssuesWithoutChildrenListView_;
	/// List view for closed (or non-existant) issues in the product backlog with children
	QListView *closedIssuesWithChildrenListView_;

	/// Check box for fixing missing issues sanity check
	QCheckBox *fixMissingIssuesCheckBox_;
	/// Check box for fixing closed issues without children sanity check
	QCheckBox *fixClosedIssuesWithoutChildrenCheckBox_;
	/// Check box for fixing closed issues with children sanity check (not doing anything right now)
	QCheckBox *fixClosedIssuesWithChildrenCheckBox_;

	/// Default accept() button to fix issues
	QPushButton *fixButton_;
	/// reject() button to quit because we're not going to attemp to fix issues
	QPushButton *quitButton_;
};

#endif // LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H
