#ifndef LYGITHUBPRODUCTBACKLOG_H
#define LYGITHUBPRODUCTBACKLOG_H

#include <QStandardItemModel>

#include "LYGithubManager.h"
#include "LYProductBacklogModel.h"

class LYConnectionQueueObject;


class LYGithubProductBacklog : public QObject
{
Q_OBJECT

public:
	LYGithubProductBacklog(const QString &username = QString(), const QString &password = QString(), const QString &repository = QString(), QObject *parent = 0);

	QStandardItemModel* model() const { return productBacklogModel_; }

	QAbstractItemModel* newModel() const;

public slots:
	void uploadChanges();

	/// Sets the username
	void setUserName(const QString &username);
	/// Sets the password
	void setPassword(const QString &password);
	/// Sets the repository
	void setRepository(const QString &repository);

signals:
	/// Reports that authentication was successful (or unsuccessful)
	void authenticated(bool isAuthenticated);
	/// Reports that the backlog issues were retrieved from the github repository
	void productBacklogReturned();
	/// Reports that the ordering for the backlog was retrieved from the github repository
	void productBacklogOrderingReturned(QVariantMap comment);
	/// Reports that the changes were uploaded successfully (or unsuccessfully)
	void changesUploaded(bool successfullyUploaded);

	void activeChanges(bool hasActiveChanges);

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

	void onUploadChangedCheckedOrderingReturn(QVariantMap comment);
	void onUploadChangesReturned(QVariantMap comment);

	void onItemChanged(QStandardItem *item);

	/// Get the issues from the Github repository to populate the list
	void populateProductBacklog();
	/// Get the current ordering for the backlog from the github repository, may be a multi-stage process
	void retrieveProductBacklogOrdering();

protected:
	/// Helper function for authentication. Won't run without values for username/password/repository
	bool authenticateHelper();


	void printGithubMapRecursive(QVariantMap map, int indentation);

protected:
	LYGithubManager *githubManager_;

	QStandardItemModel *productBacklogModel_;

	LYProductBacklogModel *newProductBacklogModel_;

	QString orderingInformation_;
	/// Id of the comment that holds the product backlog ordering information
	int ordingInformationCommentId_;

	bool activeChanges_;

	/// String to hold the user name
	QString username_;
	/// String to hold the password
	QString password_;
	/// String to hold the repository
	QString repository_;

	QList<LYConnectionQueueObject*> connectionQueue_;
};

class LYConnectionQueueObject : public QObject
{
Q_OBJECT
public:
	LYConnectionQueueObject(QObject *sender, const char *signal, QObject *receiver, const char *slot, QObject *initiatorObject, const char *initiatorSlot, QObject *parent = 0);

public slots:
	void initiate();

protected slots:
	void onSignalReceived();

protected:
	QObject *sender_;
	const char *signal_;
	QObject *receiver_;
	const char *slot_;
	QObject *initiatorObject_;
	const char *initiatorSlot_;
};

#endif // LYGITHUBPRODUCTBACKLOG_H
