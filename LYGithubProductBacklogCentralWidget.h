#ifndef LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H
#define LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H

#include <QListView>
#include <QTreeView>
#include <QPushButton>

#include "LYGithubProductBacklog.h"
#include "LYGithubProductBacklogStatusLog.h"

#include <QDialog>
#include <QLineEdit>
#include <QProgressBar>
#include <QBoxLayout>
#include <QLabel>

class LYGithubProductBacklogAuthenticationView;
class LYGithubProductBacklogSanityCheckView;
class LYGithubProductBacklogAddIssueView;
class LYGithubProductBacklogNetworkBusyView;
class LYGithubProductBacklogStatusLogView;

class LYGithubProductBacklogCentralWidget : public QWidget
{
Q_OBJECT

public:
	/// Constructor
	LYGithubProductBacklogCentralWidget(const QString &username, const QString &repository, QWidget *parent = 0);
	/// Destructor is responsible for the statusLog, authenticationView, and networkBusyView
	~LYGithubProductBacklogCentralWidget();

public slots:
	/// Handles a request to show the status log
	void showStatusLog();
	/// Ensures that all windows are closed if the mainWindow is closed
	void ensureReadyForClose();

signals:
	/// Emitted when we decide to quit, received by the main window
	void requestQuit();
	/// Emitted when we wish to have text displayed in the main window's status bar
	void requestStatusBarMessage(const QString &message, int timeout);

protected slots:
	/// Handles the communication from the authentication view regarding new username, password, and repository
	void onSubmitAuthenticationInformationAvailable(const QString &username, const QString &password, const QString &repository);
	/// Handles the authenticated signal from the github manager
	void onAuthenticated(bool authenticated);

	/// Handles interaction with the Upload Changes button
	void onUploadChangesButtonClicked();
	/// Handles uploaded signal from the product backlog
	void onUploaded(bool successfullyUploaded);

	/// Enables and disables the uploadChangesButton based on whether the list has been modified by the user
	void onActiveChangesChanged(bool hasActiveChanges);

	/// Handles changing the text on the Close Issue button to the currently selected issue
	void onTreeViewIndexClicked(const QModelIndex &index);
	/// Handles launching the Add Issue dialog
	void onAddIssueButtonClicked();
	/// Handles the request for actually closing an issue
	void onCloseIssueButtonClicked();

	/// Handles opening up the sanity check view if the sanity checks didn't pass on startup
	void onSanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck);

	/// Handles displaying the networkBusyView when the product backlog says a network request is in process
	void onNetworkRequestBusy(bool isBusy, const QString &busyText);

	void onCustomContextMenuRequested(const QPoint &point);
	void onCustomContextMenuRequestToggle();

	void onModelAboutToBeRefreshed();
	void onModelRefreshed();

protected:
	QModelIndexList expandedIndices(const QModelIndex parent = QModelIndex());

protected:
	/// View for the tree model coming from the product backlog
	QTreeView *treeView_;
	/// Push button to upload changes that have been made
	QPushButton *uploadChangesButton_;
	/// Push button for closing issues
	QPushButton *closeIssueButton_;
	/// Push button for adding new issues
	QPushButton *addIssueButton_;

	/// The product backlog model pointer
	LYGithubProductBacklog *productBacklog_;
	/// The authentication window pointer
	LYGithubProductBacklogAuthenticationView *authenticationView_;
	/// The network busy window pointer
	LYGithubProductBacklogNetworkBusyView *networkBusyView_;
	/// The status log window pointer
	LYGithubProductBacklogStatusLogView *statusLogView_;

	QModelIndexList expandedIndexList_;
};

class LYGithubProductBacklogAuthenticationView : public QDialog
{
Q_OBJECT
public:
	/// Constructor
	LYGithubProductBacklogAuthenticationView(const QString &username, const QString &repository, QWidget *parent = 0);

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

#include <QTextEdit>
class LYGithubProductBacklogAddIssueView : public QDialog
{
Q_OBJECT
public:
	LYGithubProductBacklogAddIssueView(QWidget *parent = 0);

public slots:
	/// Helper slot handling the behaviour of the dialog if a new issue is created or not.
	void onGitIssueCreated(bool issueCreated);

signals:
	void requestCreateNewIssue(const QString &title, const QString &body);
	/// Notifier that the creation of a new issue has finished (whether it is successful or not).
	void finished();

protected slots:
	/// Helper slot handling the cancel.
	void onCancelButtonClicked();
	/// Helper slot that handles when a request to submit a new issue is submitted.
	void onSubmitIssueButtonClicked();

	/// Handles enabling the submit button based on whether there is text in the title and body of the issue.
	void onEditsChanged();

	/// Handles updating the small widget that says thanks to the user for their input and after 5 seconds it closes itself.
	void onExitCountDownTimeout();
	/// Handles hiding the dialog and emits the finished signal.
	void hideAndFinish();

protected:
	virtual void closeEvent(QCloseEvent *);

protected:
	QLineEdit *issueTitleEdit_;
	QTextEdit *issueBodyEdit_;
	QPushButton *submitIssuesButton_;
	QPushButton *cancelButton_;

	QProgressBar *waitingBar_;
	QLabel *messageLabel_;

	QTimer *exitCountDownTimer_;
	int exitCountDownCounter_;

	bool issueCreatedSuccessfully_;
};

class LYGithubProductBacklogNetworkBusyView : public QDialog
{
Q_OBJECT
public:
	/// Constructor
	LYGithubProductBacklogNetworkBusyView(const QString &interactionText, QWidget *parent = 0);

protected:
	/// Progress bar to show interaction is happening
	QProgressBar *serverInteractionProgressBar_;
	/// Label for interaction progress
	QLabel *serverInteractionLabel_;

	/// Main vertical layout
	QVBoxLayout vl_;
};

class LYGithubProductBacklogStatusLogView : public QWidget
{
Q_OBJECT
public:
	/// Constructor
	LYGithubProductBacklogStatusLogView(QWidget *parent = 0);

protected:
	/// Holds the list view to show the status log StringList model
	QListView *statusLogListView_;
};

#endif // LYGITHUBPRODUCTBACKLOGCENTRALWIDGET_H
