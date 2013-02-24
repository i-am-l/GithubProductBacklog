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
	LYGithubProductBacklog(const QString &username = QString(), const QString &password = QString(), const QString &repository = QString(), QObject *parent = 0);

	QAbstractItemModel* model() const;

public slots:
	void fixStartupIssues();
	/// Uploads changes made in the model to the remote repository
	void uploadChanges();

	/// Sets the username
	void setUserName(const QString &username);
	/// Sets the password
	void setPassword(const QString &password);
	/// Sets the repository
	void setRepository(const QString &repository);

	QStringList missingIssues() const;
	QStringList orderedIssuesWithoutChildren() const;
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

	/// Reports that there are active changes to the model that are not yet uploaded to the remote repository
	void activeChanges(bool hasActiveChanges);

	void sanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck);

protected slots:
	/// Handles the return of the authentication request
	void onGitAuthenticated(bool wasAuthenticated);

	/// Handles the return of the product backlog populating request (get all open issues)
	void onPopulateProductBacklogReturned(QList<QVariantMap> issues);

	/// Handles the return of the product backlog ordering request (find the ordering issue)
	void onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap> issues);
	/// Handles the return of the product backlog ordering request (get the ordering comment from the ordering issue)
	void onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap comments);
	/// Handles the return of the product backlog ordering request (directly retrieving the comment if we know the commend id)
	void onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap comment);

	/// Handles checking for server side changes that may have taken place and if none are detected setting the issue number and new string for upload to the remote repository
	void onUploadChangedCheckedOrderingReturn(QVariantMap comment);
	/// Handles changing the activeChanged() status when changes are successfully uploaded
	void onUploadChangesReturned(QVariantMap comment);

	void onProductBacklogModelRefreshed();

protected:
	/// Helper function for authentication. Won't run without values for username/password/repository
	bool authenticateHelper();

	void printGithubMapRecursive(QVariantMap map, int indentation);

	/// Creates the list of connectionQueueObjects and places them in the startupConnectionQueue (clears the queue if necessary)
	void createStartupConnectionQueue();
	/// Creates the list of connectionQueueObjects and places them in the uploadChangesConnectionQueue (clears the queue if necessary)
	void createUploadChangesConnectionQueue();

protected:
	LYGithubManager *githubManager_;

	LYProductBacklogModel *productBacklogModel_;

	QString orderingInformation_;
	QList<QVariantMap> issues_;
	QList<QVariantMap> closedIssues_;
	/// Id of the comment that holds the product backlog ordering information
	int ordingInformationCommentId_;

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
};

#endif // LYGITHUBPRODUCTBACKLOG_H
