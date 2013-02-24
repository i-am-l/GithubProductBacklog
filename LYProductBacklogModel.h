#ifndef LYPRODUCTBACKLOGMODEL_H
#define LYPRODUCTBACKLOGMODEL_H

#include <QAbstractItemModel>

class LYProductBacklogItem;

class LYProductBacklogModel : public QAbstractItemModel
{
Q_OBJECT
public:
	/// Sanity check enum (problems that can be detected on startup)
	enum ProductBacklogSanityCheck{
		SanityCheckPassed = 0x0,				///< All sanity checks passed
		SanityCheckFailedMissingIssue = 0x1,			///< Sanity check failed because there are open issues in the repository that are not in the product backlog
		SanityCheckFailedFalseOrderedIssueNoChildren = 0x2,	///< Sanity check failed because there are closed (or non-existant) issues in the product backlog, but thankfully they don't have any children issues
		SanityCheckFailedFalseOrderedIssueWithChildren = 0x4	///< Sanity check failed because there are closed (or non-existant) issues in the product backlog, unfortunately they also have children issues
	};
	/// Declares the ProductBacklogSanityChecks type for holding ProductBacklogSanityCheck enums that are logically or'd together
	Q_DECLARE_FLAGS(ProductBacklogSanityChecks, ProductBacklogSanityCheck)

	/// Constructor
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

	/// Set the closed issues (not great API model)
	void setClosedIssues(QList<QVariantMap> closedIssues);
	/// Tries to parse the list. Returns any sanity check values. If the sanity checks didn't pass, then nothing is done to the model.
	LYProductBacklogModel::ProductBacklogSanityChecks parseList(const QString &orderingInformation, QList<QVariantMap> issues);
	/// Fixes any parse issues with the default values (needs to be extended)
	bool fixParseIssues(const QString &orderingInformation, QList<QVariantMap> issues);

	/// Clears the model
	void clear();

	/// Generates the list notation to upload to Github
	QString generateListNotation() const;

	/// Returns a list of issue numbers for missing issues
	const QList<int> orderedIssuesWithoutChildrenNotFound() const;
	/// Returns a list of issue numbers for closed (or non-existant) issues in the product backlog (without children)
	const QList<int> orderedIssuesWithChildrenNotFound() const;
	/// Returns a list of issue numbers for closed (or non-existant) issues in the product backlog (with children)
	const QList<int> unorderedIssuesFound() const;
	/// Returns a title from an issue number (for both open and closed issues)
	QString titleFromIssueNumber(int issueNumber) const;

protected:
	/// Returns a mapping from issue number to parent issue number (if no parent, then -1)
	QMap<int, int> parseListNotation(const QString &orderingInformation) const;
	/// Returns a list of children issue numbers in order (including the issue number of the issue passed in)
	QList<int> childrenOf(LYProductBacklogItem *pbItem) const;

	/// Helper function for generating the list notation
	QString recursiveGenerateNotation(LYProductBacklogItem *pbItem) const;

	/// Parses the orderingInformation from the braced, hierarchical form to a flat list of ordered integers (still in string form)
	QStringList internalParseToFlatList(const QString &orderingInformation) const;
	/// Does the work of doing the sanity checks on the given ordering information and issues list
	LYProductBacklogModel::ProductBacklogSanityChecks internalDoSanityChecks(const QString &orderingInformation, QList<QVariantMap> issues);
	/// Does the work of parsing the lists and can also fix sanity check problems
	bool internalParseListWithOptions(const QString &orderingInformation, QList<QVariantMap> issues, bool appendMissingIssues = false, bool removeClosedIssuesWithoutChildren = false);

	/// Internally sets the member variables actually holding the issues and ordering information
	void setInternalData(QMap<int, LYProductBacklogItem*> allIssues, QList<int> orderingInformation);

signals:
	/// This signal is emitted before the model is refreshed or updated. Views might want to use it to remember their scrolling position, etc.
	void modelAboutToBeRefreshed();
	/// This signal is emitted after the model is refreshed or updated. Views might want to use it to restore their scrolling position, etc.
	void modelRefreshed();

protected:
	/// Model mapping for issue number to issue item
	QMap<int, LYProductBacklogItem*> allOpenIssues_;
	/// Model list for flat ordering information as a list of integers (issue numbers)
	QList<int> orderingInformation_;
	/// The list of closed issues
	QList<QVariantMap> closedIssues_;
	/// A mapping of all issues (opened or closed) from issue number to issue title
	QMap<int, QString> allIssuesToTitle_;

	/// List of issue numbers of closed (or non-existant) issues that are in the product backlog without children
	QList<int> orderedIssuesWithoutChildrenNotFound_;
	/// List of issue numbers of closed (or non-existant) issues that are in the product backlog with children
	QList<int> orderedIssuesWithChildrenNotFound_;
	/// List of issue numbers of open issues not the product backlog
	QList<int> unorderedIssuesFound_;
};
/// Declares ProductBacklogSanityChecks type as available to QFlags
Q_DECLARE_OPERATORS_FOR_FLAGS(LYProductBacklogModel::ProductBacklogSanityChecks)

class LYProductBacklogItem {
public:
	/// Constructor take issue title, issue number, and parent's issue number
	LYProductBacklogItem(const QString &issueTitle, int issueNumber, int parentIssueNumber = -1);

	/// Returns the issue title
	QString issueTitle() const;
	/// Returns the issue number
	int issueNumber() const;
	/// Returns the parent's issue number (-1 if no parent)
	int parentIssueNumber() const;

	/// Sets the parent's issue number (-1 if no parent)
	void setParentIssueNumber(int parentIssueNumber);

protected:
	/// Holds the issue title
	QString issueTitle_;
	/// Holds the issue number
	int issueNumber_;
	/// Holds the parent's issue number (-1 if no parent)
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
	/// Holds a list of QPersistentModelIndex that were the source of the drag event (with the assumption that they pertain to the same model as the drop destination)
	QList<QPersistentModelIndex> modelIndexList_;
};

#endif // LYPRODUCTBACKLOGMODEL_H
