#ifndef LYGITHUBPRODUCTBACKLOG_H
#define LYGITHUBPRODUCTBACKLOG_H

#include <QStandardItemModel>

#include "LYGithubManager.h"

class LYProductBacklogModel;

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
	void onNewItemChanged(QStandardItem *item);

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
};

#include <QAbstractItemModel>

class LYProductBacklogItem {
public:
	LYProductBacklogItem(const QString &issueTitle, int issueNumber, int parentIssueNumber = -1);

	QString issueTitle() const;
	int issueNumber() const;
	int parentIssueNumber() const;

protected:
	QString issueTitle_;
	int issueNumber_;
	int parentIssueNumber_;
};

class LYProductBacklogModel : public QAbstractItemModel
{
Q_OBJECT
public:
	LYProductBacklogModel(QObject *parent = 0);

	// Re-implemented public functions from QAbstractItemModel
	/////////////////////
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &child) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	/// Returns whether or not this item has children (determines this from the underlying list)
	virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

	/// Returns the LYProductBacklogItem at \c index
	LYProductBacklogItem* productBacklogItem(const QModelIndex& index) const;
	/// Returns the model index for a given LYProductBacklogItem
	QModelIndex indexForProductBacklogItem(LYProductBacklogItem *productBacklogItem) const;

	// Drag and Drop functions:
	////////////////////////////////

	/// Re-implemented from QAbstractItemModel to deal with dropping when re-ordering the queue via drag-and-drop.
	virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
	/// Re-implemented from QAbstractItemModel to deal with dragging when re-ordering the queue via drag-and-drop.
	virtual QMimeData* mimeData(const QModelIndexList &indexes) const;
	/// Re-implemented from QAbstractItemModel to deal with dragging when re-ordering the queue via drag-and-drop.
	virtual QStringList mimeTypes() const;
	/// Re-implemented from QAbstractItemModel to deal with dropping when re-ordering the queue via drag-and-drop
	virtual Qt::DropActions supportedDropActions() const;

	void setInternalData(QMap<int, LYProductBacklogItem*> allIssues, QList<int> orderingInformation);

	void clear();

signals:
	/// This signal is emitted before the model is refreshed or updated. Views might want to use it to remember their scrolling position, etc.
	void modelAboutToBeRefreshed();
	/// This signal is emitted after the model is refreshed or updated. Views might want to use it to restore their scrolling position, etc.
	void modelRefreshed();

protected:
	QMap<int, LYProductBacklogItem*> allIssues_;
	QList<int> orderingInformation_;
};

#include <QMimeData>
#include <QPersistentModelIndex>

class LYModelIndexListMimeData3 : public QMimeData {
	Q_OBJECT
public:
	/// Constructor
	LYModelIndexListMimeData3(const QModelIndexList& mil) : QMimeData() {
		foreach(const QModelIndex& mi, mil)
			mil_ << mi;
	}

	/// Access the model index list
	QList<QPersistentModelIndex> modelIndexList() const { return mil_; }

	/// Returns the formats we have: only an application-specific binary format. (Internal use only; other apps should not look at this.)
	virtual QStringList formats() const { return QStringList() << "application/octet-stream"; }

protected:
	/// holds a list of QPersistentModelIndex that were the source of the drag event (with the assumption that they pertain to the same model as the drop destination)
	QList<QPersistentModelIndex> mil_;
};

#endif // LYGITHUBPRODUCTBACKLOG_H
