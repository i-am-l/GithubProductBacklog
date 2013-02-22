#ifndef LYPRODUCTBACKLOGMODEL_H
#define LYPRODUCTBACKLOGMODEL_H

#include <QAbstractItemModel>

class LYProductBacklogItem;

class LYProductBacklogModel : public QAbstractItemModel
{
Q_OBJECT
public:
	enum ProductBacklogSanityCheck{
		SanityCheckPassed = 0x0,
		SanityCheckFailedMissingIssue = 0x1,
		SanityCheckFailedFalseOrderedIssueNoChildren = 0x2,
		SanityCheckFailedFalseOrderedIssueWithChildren = 0x4
	};
	Q_DECLARE_FLAGS(ProductBacklogSanityChecks, ProductBacklogSanityCheck)

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

	LYProductBacklogModel::ProductBacklogSanityChecks parseList(const QString &orderingInformation, QList<QVariantMap> issues);
	bool appendMissingIssues(const QString &orderingInformation, QList<QVariantMap> issues);
	bool removeClosedIssuesWithoutChildren(const QString &orderingInformation, QList<QVariantMap> issues);

	void clear();

	QString generateListNotation() const;

	const QList<int> orderedIssuesWithoutChildrenNotFound() const;
	const QList<int> orderedIssuesWithChildrenNotFound() const;
	const QList<int> unorderedIssuesFound() const;

protected:
	QMap<int, int> parseListNotation(const QString &orderingInformation) const;
	QList<int> childrenOf(LYProductBacklogItem *pbItem) const;

	QString recursiveGenerateNotation(LYProductBacklogItem *pbItem) const;

	QStringList internalParseToFlatList(const QString &orderingInformation) const;
	LYProductBacklogModel::ProductBacklogSanityChecks internalDoSanityChecks(const QString &orderingInformation, QList<QVariantMap> issues);

	void setInternalData(QMap<int, LYProductBacklogItem*> allIssues, QList<int> orderingInformation);

signals:
	/// This signal is emitted before the model is refreshed or updated. Views might want to use it to remember their scrolling position, etc.
	void modelAboutToBeRefreshed();
	/// This signal is emitted after the model is refreshed or updated. Views might want to use it to restore their scrolling position, etc.
	void modelRefreshed();

protected:
	QMap<int, LYProductBacklogItem*> allIssues_;
	QList<int> orderingInformation_;

	QList<int> orderedIssuesWithoutChildrenNotFound_;
	QList<int> orderedIssuesWithChildrenNotFound_;
	QList<int> unorderedIssuesFound_;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(LYProductBacklogModel::ProductBacklogSanityChecks)

class LYProductBacklogItem {
public:
	LYProductBacklogItem(const QString &issueTitle, int issueNumber, int parentIssueNumber = -1);

	QString issueTitle() const;
	int issueNumber() const;
	int parentIssueNumber() const;

	void setParentIssueNumber(int parentIssueNumber);

protected:
	QString issueTitle_;
	int issueNumber_;
	int parentIssueNumber_;
};

#include <QMimeData>
#include <QPersistentModelIndex>
#include <QStringList>

class LYProductBacklogModelIndexListMimeData3 : public QMimeData {
	Q_OBJECT
public:
	/// Constructor
	LYProductBacklogModelIndexListMimeData3(const QModelIndexList& modelIndexList) : QMimeData() {
		foreach(const QModelIndex& modelIndex, modelIndexList)
			modelIndexList_ << modelIndex;
	}

	/// Access the model index list
	QList<QPersistentModelIndex> modelIndexList() const { return modelIndexList_; }

	/// Returns the formats we have: only an application-specific binary format. (Internal use only; other apps should not look at this.)
	virtual QStringList formats() const { return QStringList() << "application/octet-stream"; }

protected:
	/// holds a list of QPersistentModelIndex that were the source of the drag event (with the assumption that they pertain to the same model as the drop destination)
	QList<QPersistentModelIndex> modelIndexList_;
};

#endif // LYPRODUCTBACKLOGMODEL_H
