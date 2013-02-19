#include "LYProductBacklogModel.h"

LYProductBacklogModel::LYProductBacklogModel(QObject *parent) :
	QAbstractItemModel(parent)
{

}

QModelIndex LYProductBacklogModel::index(int row, int column, const QModelIndex &parent) const{
	// Return bad index if it's outside the known range
	if(column < 0 || column > 0 || row < 0)
		return QModelIndex();

	// if no parent is top level
	if(!parent.isValid()){
		int foundTopLevels = 0;
		for(int x = 0; x < orderingInformation_.count(); x++){
			if(allIssues_.value(orderingInformation_.at(x))->parentIssueNumber() == -1)
				foundTopLevels++;
			if(foundTopLevels == row+1)
				return createIndex(row, column, allIssues_.value(orderingInformation_.at(x)));
		}
		return QModelIndex();
	}
	// if parent then it's sub-level
	else{
		LYProductBacklogItem *parentProductBacklogItem = productBacklogItem(parent);
		if(!parentProductBacklogItem)
			return QModelIndex();

		int parentIndexInList = orderingInformation_.indexOf(allIssues_.key(parentProductBacklogItem));
		int siblingsFound = 0;
		for(int x = parentIndexInList; x < orderingInformation_.count(); x++){
			if(allIssues_.value(orderingInformation_.at(x))->parentIssueNumber() == parentProductBacklogItem->issueNumber())
				siblingsFound++;
			if(siblingsFound == row+1)
				return createIndex(row, column, allIssues_.value(orderingInformation_.at(x)));
		}
		return QModelIndex();
	}
}

QModelIndex LYProductBacklogModel::parent(const QModelIndex &child) const
{
	if(!child.isValid())
		return QModelIndex();

	LYProductBacklogItem *childProductBacklogItem = productBacklogItem(child);
	if(!childProductBacklogItem)
		return QModelIndex();

	for(int x = 0; x < orderingInformation_.count(); x++)
		if(allIssues_.value(orderingInformation_.at(x))->issueNumber() == childProductBacklogItem->parentIssueNumber())
			return indexForProductBacklogItem(allIssues_.value(orderingInformation_.at(x)));

	return QModelIndex();
}

int LYProductBacklogModel::rowCount(const QModelIndex &parent) const
{
	// top level
	if(!parent.isValid()){
		int topLevelsFound = 0;
		for(int x = 0; x < orderingInformation_.count(); x++)
			if(allIssues_.value(orderingInformation_.at(x))->parentIssueNumber() == -1)
				topLevelsFound++;

		return topLevelsFound;
	}
	// some sub-level
	else{
		LYProductBacklogItem *parentProductBacklogItem = productBacklogItem(parent);
		if(!parentProductBacklogItem){
			return 0;
		}
		int childrenFound = 0;
		int parentIndexInList = orderingInformation_.indexOf(allIssues_.key(parentProductBacklogItem));
		for(int x = parentIndexInList; x < orderingInformation_.count(); x++)//might optimize to figure a good ending point
			if(allIssues_.value(orderingInformation_.at(x))->parentIssueNumber() == parentProductBacklogItem->issueNumber())
				childrenFound++;

		return childrenFound;
	}
}

int LYProductBacklogModel::columnCount(const QModelIndex &parent) const
{
	if(!parent.isValid())
		return 1;
	else
		return 0;
}

QVariant LYProductBacklogModel::data(const QModelIndex &index, int role) const
{
	LYProductBacklogItem *item = productBacklogItem(index);
	if(!item){
		return QVariant();
	}

	if(role == Qt::DisplayRole) {
		switch(index.column()) {
		case 0: return item->issueTitle();
		}
	}

	return QVariant();
}

Qt::ItemFlags LYProductBacklogModel::flags(const QModelIndex &index) const
{
	if(!index.isValid())
		return Qt::ItemIsDropEnabled;
	if( index.column() != 0)
		return Qt::ItemIsEnabled;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

QVariant LYProductBacklogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch(section) {
		case 0: return QString("Title");
		default: return QVariant();
		}
	}
	return QVariant();
}

bool LYProductBacklogModel::hasChildren(const QModelIndex &parent) const{
	if(!parent.isValid())
		return true; // top level must have children
	else{
		if(rowCount(parent) > 0)
			return true;
		return false;
	}
}

LYProductBacklogItem* LYProductBacklogModel::productBacklogItem(const QModelIndex &index) const
{
	if(!index.isValid())
		return 0;
	return static_cast<LYProductBacklogItem*>(index.internalPointer());
}

QModelIndex LYProductBacklogModel::indexForProductBacklogItem(LYProductBacklogItem *productBacklogItem) const{
	if(!productBacklogItem)
		return QModelIndex();

	if(productBacklogItem->parentIssueNumber() == -1){
		// top level productBacklogItem
		int topLevelBefore = 0;
		for(int x = 0; x < orderingInformation_.count(); x++){
			if(allIssues_.value(orderingInformation_.at(x)) == productBacklogItem){
				return createIndex(topLevelBefore, 0, productBacklogItem);
			}
			if(allIssues_.value(orderingInformation_.at(x))->parentIssueNumber() == -1)
				topLevelBefore++;
		}
		return QModelIndex();
	}
	else{
		// we have a parent productBacklogItem
		bool foundParent = false;
		int backwardsIndex = orderingInformation_.indexOf(allIssues_.key(productBacklogItem)); //find myself
		int numberOfSiblings = 0;
		while(!foundParent){
			if(backwardsIndex == 0){
				return QModelIndex();
			}
			backwardsIndex--;
			if(allIssues_.value(orderingInformation_.at(backwardsIndex))->issueNumber() == productBacklogItem->parentIssueNumber()) //mark parent found if id matches
				foundParent = true;
			else if(allIssues_.value(orderingInformation_.at(backwardsIndex))->parentIssueNumber() == productBacklogItem->parentIssueNumber()) //or increment the number of your siblings
				numberOfSiblings++;
		}
		return createIndex(numberOfSiblings, 0, productBacklogItem);
	}
}

bool LYProductBacklogModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent){
	Q_UNUSED(column)

	// Handle the move action
	if(action == Qt::MoveAction){
		const LYProductBacklogModelIndexListMimeData3 *modelIndexListData = qobject_cast<const LYProductBacklogModelIndexListMimeData3*>(data);
		if(modelIndexListData){
			QList<QPersistentModelIndex> modelIndexList = modelIndexListData->modelIndexList();
			// We only handle moving one thing
			if(modelIndexList.count() != 1)
				return false;
			// Check to make sure the index is valid
			if(!modelIndexList.at(0).isValid())
				return false;

			// Grad the single model index we're interested in and the underlying item
			QPersistentModelIndex modelIndex = modelIndexList.at(0);
			LYProductBacklogItem *pbItem = productBacklogItem(index(modelIndex.row(), modelIndex.column(), modelIndex.parent()));

			// Figure out where we're moving to by figuring out the issue number of the thing that will be before us
			int indexOfThingIWillBeInsertedAfter;
			LYProductBacklogItem *itemBeforeMeAtDestinationLevel = 0;
			int effectiveRow = row;
			// These are appends, so figure out the row count
			if(row == -1)
				effectiveRow = rowCount(parent);

			// Can't get the item before us if we're going to be the first one
			if(effectiveRow == 0){
				// do nothing here
			}
			// Get the item before us by looking up the parent
			else if(parent.isValid())
				itemBeforeMeAtDestinationLevel = productBacklogItem(parent.child(effectiveRow-1, 0));
			// No parent, must be top level so look there
			else
				itemBeforeMeAtDestinationLevel = productBacklogItem(index(effectiveRow-1, 0, QModelIndex()));

			// Get the actual issue number that will be right before us in the flat list ... that is the last child of the sibling right before us at the same level
			if(itemBeforeMeAtDestinationLevel)
				indexOfThingIWillBeInsertedAfter = childrenOf(itemBeforeMeAtDestinationLevel).last();
			// Maybe we can't because there's nothing right before us
			else{
				// First item at the top level is special
				if(!parent.isValid())
					indexOfThingIWillBeInsertedAfter = -1;
				// Differentiate between going to position zero on an exisiting row
				else if(row == 0)
					indexOfThingIWillBeInsertedAfter = productBacklogItem(parent)->issueNumber();
				// And going to the end of a row
				else
					indexOfThingIWillBeInsertedAfter = childrenOf(productBacklogItem(parent)).last();
			}

			//Figure out the flat list of things we have to move and the offset to our current position
			QList<int> listToMove = childrenOf(pbItem);
			int offsetIndex = orderingInformation_.indexOf(pbItem->issueNumber());

			// Make sure that something didn't get messed up (running off the end of the list or the list changing order somehow)
			for(int x = 0; x < listToMove.count(); x++){
				if(offsetIndex+x == orderingInformation_.count()){
					//qDebug() << "Ran off the end of the ordering list";
					return false;
				}
				if(orderingInformation_.at(x+offsetIndex) != listToMove.at(x)){
					//qDebug() << "List mismatch at " << x+offsetIndex << x;
					return false;
				}
			}

			// Begin actually changing things
			emit beginResetModel();
			emit modelAboutToBeRefreshed();

			// Remove us and our children from the flat list
			for(int x = 0; x < listToMove.count(); x++)
				orderingInformation_.removeAt(offsetIndex);

			//Get the insertion index in the altered flat list
			int insertAtIndex = orderingInformation_.indexOf(indexOfThingIWillBeInsertedAfter)+1;
			// Add us and are children there
			for(int x = listToMove.count()-1; x >= 0; x--)
				orderingInformation_.insert(insertAtIndex, listToMove.at(x));

			// Change the parent issue number on the item if the move changed the level
			if(modelIndex.parent() != parent){
				int newParentIssueNumber = -1;
				if(parent.isValid())
					newParentIssueNumber = productBacklogItem(parent)->issueNumber();
				pbItem->setParentIssueNumber(newParentIssueNumber);
			}
			// Finished actually changing things
			emit endResetModel();
			emit modelRefreshed();

			return true;
		}
	}
	return false;
}

QMimeData* LYProductBacklogModel::mimeData(const QModelIndexList &indexes) const{
	if(indexes.count() != 1)
		return 0;
	return new LYProductBacklogModelIndexListMimeData3(indexes);
}

QStringList LYProductBacklogModel::mimeTypes() const{
	return QStringList() << "application/octet-stream";
}

Qt::DropActions LYProductBacklogModel::supportedDropActions() const{
	return (Qt::MoveAction | Qt::IgnoreAction);
}

#include <QDebug>
void LYProductBacklogModel::parseList(const QString &orderingInformation, QList<QVariantMap> issues){
	QMap<int, int> issueNumberToParentIssueNumber = parseListNotation(orderingInformation);

	QString partialOrderingParse = orderingInformation;
	partialOrderingParse.replace('{', ';');
	partialOrderingParse.replace("};", "");
	QStringList orderingList = partialOrderingParse.split(";", QString::SkipEmptyParts);
	QList<int> localOrderingList;
	QList<int> localMissingList;
	for(int x = 0; x < orderingList.count(); x++)
		localOrderingList.append(orderingList.at(x).toInt());

	clear();

	LYProductBacklogItem *newIssueItem;
	QMap<int, LYProductBacklogItem*> newAllIssues;
	for(int x = 0; x < issues.count(); x++){
		newIssueItem = new LYProductBacklogItem(issues.at(x).value("number").toString() + " - " + issues.at(x).value("title").toString(), issues.at(x).value("number").toInt(), issueNumberToParentIssueNumber.value(issues.at(x).value("number").toInt()));
		newAllIssues.insert(newIssueItem->issueNumber(), newIssueItem);
		if(localOrderingList.contains(newIssueItem->issueNumber()))
			localOrderingList.removeAll(newIssueItem->issueNumber());
		else
			localMissingList.append(newIssueItem->issueNumber());
	}

	orderedIssuesNotFound_.clear();
	unorderedIssuesFound_.clear();
	orderedIssuesNotFound_ = localOrderingList;
	unorderedIssuesFound_ = localMissingList;

	QList<int> newOrderingInformation;
	for(int x = 0; x < orderingList.count(); x++)
		newOrderingInformation.append(orderingList.at(x).toInt());
	setInternalData(newAllIssues, newOrderingInformation);
}

void LYProductBacklogModel::setInternalData(QMap<int, LYProductBacklogItem *> allIssues, QList<int> orderingInformation){
	emit beginResetModel();
	allIssues_ = allIssues;
	orderingInformation_ = orderingInformation;
	emit endResetModel();
}

void LYProductBacklogModel::clear(){
	allIssues_.clear();
	orderingInformation_.clear();
}

QString LYProductBacklogModel::generateListNotation() const{
	QString retVal;
	for(int x = 0; x < rowCount(QModelIndex()); x++)
		retVal.append(recursiveGenerateNotation(productBacklogItem(index(x, 0, QModelIndex()))));
	return retVal;
}

const QList<int> LYProductBacklogModel::orderedIssuesNotFound() const{
	return orderedIssuesNotFound_;
}

const QList<int> LYProductBacklogModel::unorderedIssuesFound() const{
	return unorderedIssuesFound_;
}

QMap<int, int> LYProductBacklogModel::parseListNotation(const QString &orderingInformation) const{
	QMap<int, int> issueNumberToParentIssueNumber;
	QList<int> parentStack;
	parentStack.push_front(-1);
	int currentIssueNumber;
	for(int x = 0; x < orderingInformation.count(); x++){
		if( (orderingInformation.at(x) == '{') || (orderingInformation.at(x) == '}') ){
			//do nothing
		}
		else if(orderingInformation.at(x) == ';'){
			parentStack.pop_front();
		}
		else{
			QString numberString;
			numberString.append(orderingInformation.at(x));
			while(orderingInformation.at(x+1).isDigit())
				numberString.append(orderingInformation.at(++x));
			currentIssueNumber = numberString.toInt();
			issueNumberToParentIssueNumber.insert(currentIssueNumber, parentStack.front());
			parentStack.push_front(currentIssueNumber);
		}
	}
	return issueNumberToParentIssueNumber;
}

QList<int> LYProductBacklogModel::childrenOf(LYProductBacklogItem *pbItem) const{
	QList<int> retVal;
	retVal.append(pbItem->issueNumber());
	QModelIndex itemIndex = indexForProductBacklogItem(pbItem);
	for(int x = 0; x < rowCount(itemIndex); x++)
		retVal.append(childrenOf(productBacklogItem(itemIndex.child(x, 0))));
	return retVal;
}

QString LYProductBacklogModel::recursiveGenerateNotation(LYProductBacklogItem *pbItem) const{
	QString retVal;
	retVal.append(QString("%1").arg(pbItem->issueNumber()));
	QModelIndex myIndex = indexForProductBacklogItem(pbItem);
	if(hasChildren(myIndex))
		retVal.append("{");
	for(int x = 0; x < rowCount(myIndex); x++)
		retVal.append(recursiveGenerateNotation(productBacklogItem(index(x, 0, myIndex))));
	if(hasChildren(myIndex))
		retVal.append("}");
	retVal.append(";");
	return retVal;
}

LYProductBacklogItem::LYProductBacklogItem(const QString &issueTitle, int issueNumber, int parentIssueNumber)
{
	issueTitle_ = issueTitle;
	issueNumber_ = issueNumber;
	parentIssueNumber_ = parentIssueNumber;
}

QString LYProductBacklogItem::issueTitle() const{
	return issueTitle_;
}

int LYProductBacklogItem::issueNumber() const{
	return issueNumber_;
}

int LYProductBacklogItem::parentIssueNumber() const{
	return parentIssueNumber_;
}

void LYProductBacklogItem::setParentIssueNumber(int parentIssueNumber){
	parentIssueNumber_ = parentIssueNumber;
}
