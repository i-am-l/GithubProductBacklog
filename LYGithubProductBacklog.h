#ifndef LYGITHUBPRODUCTBACKLOG_H
#define LYGITHUBPRODUCTBACKLOG_H

#include <QStandardItemModel>

#include "LYGithubManager.h"
#include "LYProductBacklogModel.h"

class LYConnectionQueueObject : public QObject
{
Q_OBJECT
public:
	LYConnectionQueueObject(QObject *parent = 0);

	QString signal() const;

	void printInitiatorArguments() const;

public slots:
	void setSender(QObject *sender);
	void setSignal(const char *signal);
	void setSender(QObject *sender, const char *signal);

	void setReceiver(QObject *receiver);
	void setSlot(const char *slot);
	void setReceiver(QObject *receiver, const char *slot);


	void setInitiatorObject(QObject *initiatorObject);
	void setInitiatorSlot(const char *initiatorSlot);
	void setInitiatorArguments(QVariantList initiatorArguments);
	void setInitiatorObject(QObject *initiatorObject, const char *initiatorSlot, QVariantList initiatorArguments = QVariantList());

	void initiate();

protected slots:
	void onSignalReceived();

signals:
	void initiated(LYConnectionQueueObject *queueObject);
	void finished(LYConnectionQueueObject *queueObject);

protected:
	QObject *sender_;
	const char *signal_;
	QObject *receiver_;
	const char *slot_;
	QObject *initiatorObject_;
	const char *initiatorSlot_;
	QVariantList initiatorArguments_;
};

class LYConnectionQueue : public QObject
{
Q_OBJECT
public:
	LYConnectionQueue(QObject *parent = 0);

	int queuedObjectsCount() const;
	int waitingObjectsCount() const;

	QStringList queuedObjects() const;
	QStringList waitingObjects() const;

	LYConnectionQueueObject* first();
	LYConnectionQueueObject* objectAt(int index);

public slots:
	void startQueue();
	void stopQueue();
	void clearQueue();

	void pushFrontConnectionQueueObject(LYConnectionQueueObject *queueObject);
	void pushBackConnectionQueueObject(LYConnectionQueueObject *queueObject);

protected slots:
	void onInitiated(LYConnectionQueueObject *queueObject);
	void onFinished(LYConnectionQueueObject *queueObject);

protected:
	QList<LYConnectionQueueObject*> connetionQueue_;
	QList<LYConnectionQueueObject*> initiatedButUnfinished_;

	bool queueStopped_;
};

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

	void createStartupConnectionQueue();
	void createUploadChangesConnectionQueue();

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

	LYConnectionQueue startupConnectionQueue_;
	LYConnectionQueue uploadChangesConnectionQueue_;
};

#endif // LYGITHUBPRODUCTBACKLOG_H
