#include "LYGithubProductBacklog.h"

#include <QDebug>

LYGithubProductBacklog::LYGithubProductBacklog(const QString &username, const QString &password, const QString &repository, QObject *parent) :
	QObject(parent)
{
	activeChanges_ = false;
	ordingInformationCommentId_ = -1;
	username_ = username;
	password_ = password;
	repository_ = repository;

	productBacklogModel_ = new QStandardItemModel(this);
	QStandardItem *item = new QStandardItem("Not Connected");
	productBacklogModel_->appendRow(item);

	newProductBacklogModel_ = new LYProductBacklogModel(this);

	connect(productBacklogModel_, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(onItemChanged(QStandardItem*)));
	productBacklogModel_->setSupportedDragActions(Qt::MoveAction);

	//newProductBacklogModel_->setSupportedDragActions(Qt::MoveAction);

	githubManager_ = new LYGithubManager(this);
	authenticateHelper();
}

QAbstractItemModel* LYGithubProductBacklog::newModel() const{
	return newProductBacklogModel_;
}

void LYGithubProductBacklog::uploadChanges(){
	connect(this, SIGNAL(productBacklogOrderingReturned(QVariantMap)), this, SLOT(onUploadChangedCheckedOrderingReturn(QVariantMap)));
	retrieveProductBacklogOrdering();
}

void LYGithubProductBacklog::setUserName(const QString &username){
	username_ = username;
	authenticateHelper();
}

void LYGithubProductBacklog::setPassword(const QString &password){
	password_ = password;
	authenticateHelper();
}

void LYGithubProductBacklog::setRepository(const QString &repository){
	repository_ = repository;
	authenticateHelper();
}

void LYGithubProductBacklog::onGitAuthenticated(bool wasAuthenticated){
	if(wasAuthenticated){
		qDebug() << "Successfully authenticated";
		connect(this, SIGNAL(productBacklogOrderingReturned(QVariantMap)), this, SLOT(populateProductBacklog()));
		retrieveProductBacklogOrdering();
	}
	else
		qDebug() << "Could not authenticate";

	emit authenticated(wasAuthenticated);
}

void LYGithubProductBacklog::onPopulateProductBacklogReturned(QList<QVariantMap> issues){
	disconnect(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)), this, SLOT(onPopulateProductBacklogReturned(QList<QVariantMap>)));

	productBacklogModel_->clear();
	QString issueItemString;
	QStandardItem *issueItem;
	QMap<int, QStandardItem*> allIssues;
	for(int x = 0; x < issues.count(); x++){
		issueItemString = QString("Issue %1: %2").arg(issues.at(x).value("number").toString()).arg(issues.at(x).value("title").toString());
		issueItem = new QStandardItem(issueItemString);
		issueItem->setData(QVariant(issues.at(x).value("number").toInt()), Qt::UserRole);
		issueItem->setEditable(false);
		issueItem->setDragEnabled(true);
		issueItem->setDropEnabled(false);
		allIssues.insert(issues.at(x).value("number").toInt(), issueItem);
	}

	QString partialOrderingParse = orderingInformation_;
	partialOrderingParse.replace('{', ';');
	partialOrderingParse.replace("};", "");
	QStringList orderingList = partialOrderingParse.split(";", QString::SkipEmptyParts);
	int issueNumber;
	for(int x = 0; x < orderingList.count(); x++){
		issueNumber = orderingList.at(x).toInt();
		productBacklogModel_->appendRow(allIssues.value(issueNumber));
	}

	QMap<int, int> issueNumberToParentIssueNumber;
	QList<int> parentStack;
	parentStack.push_front(-1);
	int currentIssueNumber;
	for(int x = 0; x < orderingInformation_.count(); x++){
		if( (orderingInformation_.at(x) == '{') || (orderingInformation_.at(x) == '}') ){
			//do nothing
		}
		else if(orderingInformation_.at(x) == ';'){
			parentStack.pop_front();
		}
		else{
			QString numberString;
			numberString.append(orderingInformation_.at(x));
			while(orderingInformation_.at(x+1).isDigit())
				numberString.append(orderingInformation_.at(++x));
			currentIssueNumber = numberString.toInt();
			issueNumberToParentIssueNumber.insert(currentIssueNumber, parentStack.front());
			parentStack.push_front(currentIssueNumber);
		}
	}

	newProductBacklogModel_->clear();
	LYProductBacklogItem *newIssueItem;
	QMap<int, LYProductBacklogItem*> newAllIssues;
	for(int x = 0; x < issues.count(); x++){
		newIssueItem = new LYProductBacklogItem(issues.at(x).value("number").toString() + " - " + issues.at(x).value("title").toString(), issues.at(x).value("number").toInt(), issueNumberToParentIssueNumber.value(issues.at(x).value("number").toInt()));
		newAllIssues.insert(newIssueItem->issueNumber(), newIssueItem);
	}

	QList<int> newOrderingInformation;
	for(int x = 0; x < orderingList.count(); x++)
		newOrderingInformation.append(orderingList.at(x).toInt());
	newProductBacklogModel_->setInternalData(newAllIssues, newOrderingInformation);
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap> issues){
	disconnect(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)), this, SLOT(onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap>)));

	int issueNumber = -1;
	for(int x = 0; x < issues.count(); x++)
		if(issues.at(x).value("title").toString() == "ProductBacklogInfo")
			issueNumber = issues.at(x).value("number").toInt();

	if(issueNumber > 0){
		githubManager_->getSingleIssueComments(issueNumber);
		connect(githubManager_, SIGNAL(singleIssueCommentReturned(QVariantMap)), this, SLOT(onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap)));
	}
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap comments){
	disconnect(githubManager_, SIGNAL(singleIssueCommentReturned(QVariantMap)), this, SLOT(onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap)));

	orderingInformation_ = comments.value("body").toString();
	ordingInformationCommentId_ = comments.value("id").toInt();

	emit productBacklogOrderingReturned(comments);
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap comment){
	disconnect(githubManager_, SIGNAL(singleCommentReturned(QVariantMap)), this, SLOT(onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap)));

	emit productBacklogOrderingReturned(comment);
}

void LYGithubProductBacklog::onUploadChangedCheckedOrderingReturn(QVariantMap comment){
	disconnect(this, 0, this, SLOT(onUploadChangedCheckedOrderingReturn(QVariantMap)));

	QString repositoryCurrentOrdering = comment.value("body").toString();
	qDebug() << "Are they the same? " << (repositoryCurrentOrdering == orderingInformation_);

	if(repositoryCurrentOrdering == orderingInformation_){
		qDebug() << "No repository side changes, proceeding with updates";

		QString newOrderingInformation;
		for(int x = 0; x < productBacklogModel_->rowCount(); x++)
			newOrderingInformation.append(QString("%1;").arg(productBacklogModel_->item(x)->data(Qt::UserRole).toInt()));

		githubManager_->editSingleComment(ordingInformationCommentId_, newOrderingInformation);
		connect(githubManager_, SIGNAL(singleCommentEdited(QVariantMap)), this, SLOT(onUploadChangesReturned(QVariantMap)));
	}
	else
		qDebug() << "Repository side changes detected, cannot proceed with updates";
}

void LYGithubProductBacklog::onUploadChangesReturned(QVariantMap comment){
	disconnect(githubManager_, SIGNAL(singleCommentEdited(QVariantMap)), this, SLOT(onUploadChangesReturned(QVariantMap)));
	activeChanges_ = false;
	emit activeChanges(false);
}

void LYGithubProductBacklog::onItemChanged(QStandardItem *item){
	Q_UNUSED(item)
	if(!activeChanges_){
		activeChanges_ = true;
		emit activeChanges(activeChanges_);
	}
}

void LYGithubProductBacklog::onNewItemChanged(QStandardItem *item){
	qDebug() << "Heard that an item in the new model changed";
}

bool LYGithubProductBacklog::authenticateHelper(){
	if(username_.isEmpty() || password_.isEmpty() || repository_.isEmpty())
		return false;

	githubManager_->setUserName(username_);
	githubManager_->setPassword(password_);
	githubManager_->setRepository(repository_);

	connect(githubManager_, SIGNAL(authenticated(bool)), this, SLOT(onGitAuthenticated(bool)));
	githubManager_->authenticate();
	return true;
}

void LYGithubProductBacklog::populateProductBacklog(){
	disconnect(this, 0, this, SLOT(populateProductBacklog()));
	githubManager_->getIssues(LYGithubManager::IssuesFilterAll);
	connect(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)), this, SLOT(onPopulateProductBacklogReturned(QList<QVariantMap>)));
}

void LYGithubProductBacklog::retrieveProductBacklogOrdering(){

	if(ordingInformationCommentId_ < 0){
		qDebug() << "Have to do the work from scratch";
		githubManager_->getIssues(LYGithubManager::IssuesFilterAll, LYGithubManager::IssuesStateClosed);
		connect(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)), this, SLOT(onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap>)));
	}
	else{
		qDebug() << "Already know the comment id, so we can cut corners";
		githubManager_->getSingleComment(ordingInformationCommentId_);
		connect(githubManager_, SIGNAL(singleCommentReturned(QVariantMap)), this, SLOT(onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap)));
	}
}

void LYGithubProductBacklog::printGithubMapRecursive(QVariantMap map, int indentation){
	QMap<QString, QVariant> iMap = map;
	QMap<QString, QVariant>::const_iterator i = iMap.constBegin();
	while (i != iMap.constEnd()) {
		QString iTypeName = i.value().typeName();
		if(iTypeName == "QVariantMap"){
			QString printString = QString("%1: --").arg(i.key());
			for(int x = 0; x < indentation; x++)
				printString.prepend("    ");
			qDebug() << printString;
			printGithubMapRecursive(i.value().toMap(), indentation+1);
		}
		else{
			QString printString = QString("%1 (%2): %3").arg(i.key()).arg(iTypeName).arg(i.value().toString());
			for(int x = 0; x < indentation; x++)
				printString.prepend("    ");
			qDebug() << printString;
		}
		++i;
	}
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
		//qDebug() << "Found parent item as " << parentProductBacklogItem->issueTitle();
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
	/*
	AMActionLogItem3 *item = logItem(index);
	// In case the item didn't (or isn't yet) in some sort of finished state
	if(item && item->canCopy() && !(item->finalState() == 7 || item->finalState() == 8 || item->finalState() == 9) )
		return Qt::ItemIsEnabled;
	//In case a loop or list had no logged actions in it
	if(item && item->canCopy() && item->actionInheritedLoop() && (childrenCount(index) == 0) )
		return Qt::ItemIsEnabled;
	//In case a loop or list had no logged actions in it
	if(item && item->canCopy() && item->actionInheritedLoop() && (successfulChildrenCount(index) == 0) )
		return Qt::ItemIsEnabled;
	if (item && item->canCopy())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	*/
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

	// Handle the move action
	if(action == Qt::MoveAction){
		const LYModelIndexListMimeData3 *milData = qobject_cast<const LYModelIndexListMimeData3*>(data);
		if(milData){
			QList<QPersistentModelIndex> mil = milData->modelIndexList();
			// We only handle moving one thing
			if(mil.count() != 1)
				return false;
			// Check to make sure the index is valid
			if(!mil.at(0).isValid())
				return false;

			QPersistentModelIndex mi = mil.at(0);
			LYProductBacklogItem *pbItem = productBacklogItem(index(mi.row(), mi.column(), mi.parent()));

			int indexOfThingIWillBeInsertedAfter;
			LYProductBacklogItem *itemBeforeMeAtDestinationLevel = 0;
			int effectiveRow = row;
			if(row == -1)
				effectiveRow = rowCount(parent);

			if(effectiveRow == 0)
				qDebug() << "At the destination level, there will be no issue before me";
			else if(parent.isValid()){
				itemBeforeMeAtDestinationLevel = productBacklogItem(parent.child(effectiveRow-1, 0));
				qDebug() << "At the destination level, the issue number before me will be " << productBacklogItem(parent.child(effectiveRow-1, 0))->issueNumber();
			}
			else{
				itemBeforeMeAtDestinationLevel = productBacklogItem(index(effectiveRow-1, 0, QModelIndex()));
				qDebug() << "At the destination level (which is top), the issue number before me will be " << productBacklogItem(index(effectiveRow-1, 0, QModelIndex()))->issueNumber();
			}
			if(itemBeforeMeAtDestinationLevel){
				qDebug() << "In the list I will go after " << childrenOf(itemBeforeMeAtDestinationLevel).last();
				indexOfThingIWillBeInsertedAfter = childrenOf(itemBeforeMeAtDestinationLevel).last();
			}
			else{
				if(!parent.isValid()){
					qDebug() << "In the list I will go first";
					indexOfThingIWillBeInsertedAfter = -1;
				}
				else if(row == 0){
					qDebug() << "In the list I will go after " << productBacklogItem(parent)->issueNumber();
					indexOfThingIWillBeInsertedAfter = productBacklogItem(parent)->issueNumber();
				}
				else{
					qDebug() << "In the list I will go after " << childrenOf(productBacklogItem(parent)).last();
					indexOfThingIWillBeInsertedAfter = childrenOf(productBacklogItem(parent)).last();
				}
			}

			qDebug() << "I will be inserted after " << indexOfThingIWillBeInsertedAfter;

			QList<int> listToMove = childrenOf(pbItem);
			qDebug() << "Need to move the sub list " << listToMove;
			qDebug() << "List was " << orderingInformation_;

			int offsetIndex = orderingInformation_.indexOf(pbItem->issueNumber());

			for(int x = 0; x < listToMove.count(); x++){
				if(offsetIndex+x == orderingInformation_.count()){
					qDebug() << "Ran off the end of the ordering list";
					return false;
				}
				if(orderingInformation_.at(x+offsetIndex) != listToMove.at(x)){
					qDebug() << "List mismatch at " << x+offsetIndex << x;
					return false;
				}
			}

			emit beginResetModel();

			for(int x = 0; x < listToMove.count(); x++)
				orderingInformation_.removeAt(offsetIndex);
			qDebug() << "Removed, now list is " << orderingInformation_;

			qDebug() << "Is the thing I need to be inserted after still in the list " << orderingInformation_.contains(indexOfThingIWillBeInsertedAfter);

			int insertAtIndex = orderingInformation_.indexOf(indexOfThingIWillBeInsertedAfter)+1;
			for(int x = listToMove.count()-1; x >= 0; x--)
				orderingInformation_.insert(insertAtIndex, listToMove.at(x));

			qDebug() << "Final list should be " << orderingInformation_;
			if(mi.parent() != parent){
				int newParentIssueNumber = -1;
				if(parent.isValid())
					newParentIssueNumber = productBacklogItem(parent)->issueNumber();
				pbItem->setParentIssueNumber(newParentIssueNumber);
			}
			emit endResetModel();
			qDebug() << "Item has moved to row " << indexForProductBacklogItem(pbItem).row();
			qDebug() << "Does it have children " << hasChildren(indexForProductBacklogItem(pbItem));
			for(int x = 0; x < rowCount(indexForProductBacklogItem(pbItem)); x++){
				qDebug() << "Child row " << x << " is " << productBacklogItem(index(x, 0, indexForProductBacklogItem(pbItem)))->issueTitle();
			}

			qDebug() << "Now the list notation is " << generateListNotation();

			return true;
		}
	}
	return false;
}

QMimeData* LYProductBacklogModel::mimeData(const QModelIndexList &indexes) const{
	if(indexes.count() != 1)
		return 0;
	return new LYModelIndexListMimeData3(indexes);
}

QStringList LYProductBacklogModel::mimeTypes() const{
	return QStringList() << "application/octet-stream";
}

Qt::DropActions LYProductBacklogModel::supportedDropActions() const{
	return (Qt::MoveAction | Qt::IgnoreAction);
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
