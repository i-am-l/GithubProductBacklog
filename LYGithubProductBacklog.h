#ifndef LYGITHUBPRODUCTBACKLOG_H
#define LYGITHUBPRODUCTBACKLOG_H

#include <QStandardItemModel>

#include "LYGithubManager.h"
#include "LYProductBacklogModel.h"
#include "LYConnectionQueue.h"

class LYGithubProductBacklog : public QObject
{
Q_OBJECT

public:
	/// Constructor takes username, password, and repository
	LYGithubProductBacklog(const QString &username = QString(), const QString &password = QString(), const QString &repository = QString(), QObject *parent = 0);

	/// Returns the model (to be passed into the tree view)
	QAbstractItemModel* model() const;
	/// Returns the model as it's actual type
	LYProductBacklogModel *productBacklogModel() const;

public slots:
	/// Fixes sanity check issues (all defaults right now)
	void fixStartupIssues();
	/// Uploads changes made in the model to the remote repository
	void uploadChanges();
	/// Called to create a new issue in the repository
	void createNewIssue(const QString &title, const QString &body);
	/// Called to close an issue by issue number
	void closeIssue(int issueNumber);

	/// Sets the username
	void setUserName(const QString &username);
	/// Sets the password
	void setPassword(const QString &password);
	/// Sets the repository
	void setRepository(const QString &repository);

	/// Returns a list of "<Issue Number> - <Issue Title>" for issues not found in the product backlog
	QStringList missingIssues() const;
	/// Returns a list of "<Issue Number> - <Issue Title>" for issues found in the product backlog but not in the open issues list (for issues without children)
	QStringList orderedIssuesWithoutChildren() const;
	/// Returns a list of "<Issue Number> - <Issue Title>" for issues found in the product backlog but not in the open issues list (for issues with children)
	QStringList orderedIssuesWithChildren() const;

signals:
	/// Reports that authentication was successful (or unsuccessful)
	void authenticated(bool isAuthenticated);
	/// Reports that the backlog issues were retrieved from the github repository
	void productBacklogReturned();
	/// Reports that the ordering for the backlog was retrieved from the github repository
	void productBacklogOrderingReturned(QVariantMap comment);
	/// Reports that the changes were uploaded successfully (or unsuccessfully)
	void changesUploaded(bool successfullyUploaded);

	/// Reports that the github manager has created a new issue
	void newIssueCreated(bool successfullyCreated);
	/// Reports that the github manager has closed an issue
	void issueClosed(bool successfullyClosed);

	/// Reports that there are active changes to the model that are not yet uploaded to the remote repository
	void activeChanges(bool hasActiveChanges);
	/// Reports that changes were successfully uploaded
	void uploaded(bool successfullyUploaded);

	/// Emitted after startup once the sanity checks have been done. Used to generate the dialog for handling sanity check problems.
	void sanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck);

	/// Emitted when a network request starts (true) or finishes (false). A message is also passed along to state the nature of the network request.
	void networkRequestBusy(bool isBusy, const QString &busyText);

protected slots:
	/// Handles the return of the authentication request
	void onGitAuthenticated(bool wasAuthenticated);

	/// Handles the return of the product backlog populating request (get all open issues)
	void onPopulateProductBacklogReturned(QList<QVariantMap> issues);

	/// Handles the return of the product backlog ordering request (get the file directly)
	void onPopulateProductBacklogGetFileContentsReturned(QVariantMap fileContents);

	/// Handles the return of the product backlog ordering request (find the ordering issue)
	void onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap> issues);
	/// Handles the return of the product backlog ordering request (get the ordering comment from the ordering issue)
	void onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap comments);
	/// Handles the return of the product backlog ordering request (directly retrieving the comment if we know the commend id)
	void onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap comment);

	/// Handles checking for server side changes that may have taken place and if none are detected setting the issue number and new string for upload to the remote repository
	void onUploadChangedCheckedOrderingReturn(QVariantMap fileContents);
	/// Handles changing the activeChanged() status when changes are successfully uploaded
	void onUploadChangesReturned(bool updated, QVariantMap fileContents);

	/// Handles the return from the github manager with the success state of the newly created issue
	void onCreateNewIssueReturned(bool issueCreatedSuccessfully, QVariantMap newIssue);

	/// Handles the return from the github manager with the success state of the close issue request
	void onCloseIssueReturned(bool issueClosedSuccessfully, QVariantMap closedIssue);

	/// Emitted when the model has been refreshed
	void onProductBacklogModelRefreshed();

protected:
	/// Helper function for authentication. Won't run without values for username/password/repository
	bool authenticateHelper();

	/// Helper function for printing each level of the JSON responses from Github
	void printGithubMapRecursive(QVariantMap map, int indentation);

	/// Creates the list of connectionQueueObjects and places them in the startupConnectionQueue (clears the queue if necessary)
	void createStartupConnectionQueue();
	/// Creates the list of connectionQueueObjects and places them in the uploadChangesConnectionQueue (clears the queue if necessary)
	void createUploadChangesConnectionQueue();
	/// Creates the list of connectionQueueObjects and places them in the createNewIssueConnectionQueue (clears the queue if necessary)
	void createCreateNewIssueConnectionQueue();
	/// Creates the list of connectionQueueObjects and places them in the closeIssueConnectionQueue (clears the queue if necessary)
	void createCloseIssueConnectionQueue();

protected:
	/// Holds the object for communitcating with the github servers
	LYGithubManager *githubManager_;

	/// Holds the model object for storing issue data for views in the tree view
	LYProductBacklogModel *productBacklogModel_;

	/// String returned from the magic ordering issue in the github repository
	QString orderingInformation_;
	/// List of the open issues
	QList<QVariantMap> issues_;
	/// List of the closed issues
	QList<QVariantMap> closedIssues_;
	/// Id of the comment that holds the product backlog ordering information
	int ordingInformationCommentId_;

	/// Holds the current ordering file SHA for comparison and lookup
	QString orderingInformationSHA_;

	/// Holds whether or not there are local model changes that need to be uploaded to the remote repository
	bool activeChanges_;

	/// String to hold the user name
	QString username_;
	/// String to hold the password
	QString password_;
	/// String to hold the repository
	QString repository_;

	/// The queue of connections to startup the product backlog (authenticate, find magic issue, parse magic issue for ordering info, get all issues)
	LYConnectionQueue startupConnectionQueue_;
	/// The queue of connections to upload changes (check magic issue for server side changes, edit magic issue with new ordering info)
	LYConnectionQueue uploadChangesConnectionQueue_;
	/// The queue of connections to create a new issue (create new issue)
	LYConnectionQueue createNewIssueConnectionQueue_;
	/// The queue of connections to close an existign issue (close issue)
	LYConnectionQueue closeIssueConnectionQueue_;
};

#endif // LYGITHUBPRODUCTBACKLOG_H
