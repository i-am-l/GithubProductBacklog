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

	githubManager_ = new LYGithubManager(this);

	createStartupConnectionQueue();
	authenticateHelper();
}

QAbstractItemModel* LYGithubProductBacklog::newModel() const{
	return newProductBacklogModel_;
}

void LYGithubProductBacklog::uploadChanges(){
	//connect(this, SIGNAL(productBacklogOrderingReturned(QVariantMap)), this, SLOT(onUploadChangedCheckedOrderingReturn(QVariantMap)));
	//retrieveProductBacklogOrdering();

	createUploadChangesConnectionQueue();
	QVariantList arguments;
	arguments.append(QVariant::fromValue(ordingInformationCommentId_));
	uploadChangesConnectionQueue_.first()->setInitiatorArguments(arguments);

	uploadChangesConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::setUserName(const QString &username){
	username_ = username;
	if(authenticateHelper())
		startupConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::setPassword(const QString &password){
	password_ = password;
	if(authenticateHelper())
		startupConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::setRepository(const QString &repository){
	repository_ = repository;
	if(authenticateHelper())
		startupConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::onGitAuthenticated(bool wasAuthenticated){
	if(wasAuthenticated)
		qDebug() << "Successfully authenticated";
	else
		qDebug() << "Could not authenticate";

	emit authenticated(wasAuthenticated);
}

void LYGithubProductBacklog::onPopulateProductBacklogReturned(QList<QVariantMap> issues){
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

	qDebug() << "Count on ordering list: " << orderingList.count();
	qDebug() << "Count on issues list: " << issues.count();

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
	int issueNumber = -1;
	for(int x = 0; x < issues.count(); x++)
		if(issues.at(x).value("title").toString() == "ProductBacklogInfo")
			issueNumber = issues.at(x).value("number").toInt();

	if(issueNumber > 0){
		QVariantList arguments;
		arguments.append(QVariant(issueNumber));
		startupConnectionQueue_.first()->setInitiatorArguments(arguments);
	}
	else{
		qDebug() << "Couldn't find magic issue";
		startupConnectionQueue_.startQueue();
		startupConnectionQueue_.clearQueue();
	}
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap comments){
	orderingInformation_ = comments.value("body").toString();
	ordingInformationCommentId_ = comments.value("id").toInt();

	emit productBacklogOrderingReturned(comments);
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap comment){
	emit productBacklogOrderingReturned(comment);
}

void LYGithubProductBacklog::onUploadChangedCheckedOrderingReturn(QVariantMap comment){
	//disconnect(this, 0, this, SLOT(onUploadChangedCheckedOrderingReturn(QVariantMap)));

	QString repositoryCurrentOrdering = comment.value("body").toString();
	qDebug() << "Are they the same? " << (repositoryCurrentOrdering == orderingInformation_);
	qDebug() << repositoryCurrentOrdering;
	qDebug() << orderingInformation_;

	if(repositoryCurrentOrdering == orderingInformation_){
		qDebug() << "No repository side changes, proceeding with updates";


		QString newOrderingInformation;
		for(int x = 0; x < productBacklogModel_->rowCount(); x++)
			newOrderingInformation.append(QString("%1;").arg(productBacklogModel_->item(x)->data(Qt::UserRole).toInt()));

		QVariantList arguments;
		arguments.append(QVariant::fromValue(ordingInformationCommentId_));
		arguments.append(QVariant::fromValue(newOrderingInformation));
		uploadChangesConnectionQueue_.first()->setInitiatorArguments(arguments);

		uploadChangesConnectionQueue_.stopQueue();
		uploadChangesConnectionQueue_.clearQueue();

		//githubManager_->editSingleComment(ordingInformationCommentId_, newOrderingInformation);
		//connect(githubManager_, SIGNAL(singleCommentEdited(QVariantMap)), this, SLOT(onUploadChangesReturned(QVariantMap)));
	}
	else{
		uploadChangesConnectionQueue_.stopQueue();
		uploadChangesConnectionQueue_.clearQueue();

		qDebug() << "Repository side changes detected, cannot proceed with updates";
	}
}

void LYGithubProductBacklog::onUploadChangesReturned(QVariantMap comment){
	Q_UNUSED(comment)
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

bool LYGithubProductBacklog::authenticateHelper(){
	if(username_.isEmpty() || password_.isEmpty() || repository_.isEmpty())
		return false;

	githubManager_->setUserName(username_);
	githubManager_->setPassword(password_);
	githubManager_->setRepository(repository_);

	return true;
}

void LYGithubProductBacklog::populateProductBacklog(){
	//disconnect(this, 0, this, SLOT(populateProductBacklog()));
	//githubManager_->getIssues(LYGithubManager::IssuesFilterAll);
	//connect(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)), this, SLOT(onPopulateProductBacklogReturned(QList<QVariantMap>)));
}

void LYGithubProductBacklog::retrieveProductBacklogOrdering(){
	if(ordingInformationCommentId_ < 0){
		qDebug() << "Have to do the work from scratch";
	}
	else{
		qDebug() << "Already know the comment id, so we can cut corners";
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

void LYGithubProductBacklog::createStartupConnectionQueue(){
	startupConnectionQueue_.clearQueue();

	LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(authenticated(bool)));
	connectionQueueObject->setReceiver(this, SLOT(onGitAuthenticated(bool)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(authenticate()));
	startupConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)));
	connectionQueueObject->setReceiver(this, SLOT(onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap>)));
	QVariantList arguments1;
	arguments1.append(QVariant::fromValue(LYGithubManager::IssuesFilterAll));
	arguments1.append(QVariant::fromValue(LYGithubManager::IssuesStateClosed));

	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(getIssues()), arguments1);
	startupConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(singleIssueCommentReturned(QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(getSingleIssueComments(int)));
	startupConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)));
	connectionQueueObject->setReceiver(this, SLOT(onPopulateProductBacklogReturned(QList<QVariantMap>)));
	QVariantList arguments2;
	arguments2.append(QVariant::fromValue(LYGithubManager::IssuesFilterAll));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(getIssues()), arguments2);
	startupConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);
}

void LYGithubProductBacklog::createUploadChangesConnectionQueue(){
	uploadChangesConnectionQueue_.clearQueue();

	LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(singleCommentReturned(QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onUploadChangedCheckedOrderingReturn(QVariantMap)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(getSingleComment(int)));
	uploadChangesConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(singleCommentEdited(QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onUploadChangesReturned(QVariantMap)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(editSingleComment(int,QString)));
	uploadChangesConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);
}


LYConnectionQueueObject::LYConnectionQueueObject(QObject *parent) :
	QObject(parent)
{

}

QString LYConnectionQueueObject::signal() const{
	QString retVal = QString("%1").arg(signal_);
	return retVal;
}

void LYConnectionQueueObject::setSender(QObject *sender){
	sender_ = sender;
}

void LYConnectionQueueObject::setSignal(const char *signal){
	signal_ = signal;
}

void LYConnectionQueueObject::setSender(QObject *sender, const char *signal){
	setSender(sender);
	setSignal(signal);
}

void LYConnectionQueueObject::setReceiver(QObject *receiver){
	receiver_ = receiver;
}

void LYConnectionQueueObject::setSlot(const char *slot){
	slot_ = slot;
}

void LYConnectionQueueObject::setReceiver(QObject *receiver, const char *slot){
	setReceiver(receiver);
	setSlot(slot);
}

void LYConnectionQueueObject::setInitiatorObject(QObject *initiatorObject){
	initiatorObject_ = initiatorObject;
}

void LYConnectionQueueObject::setInitiatorSlot(const char *initiatorSlot){
	initiatorSlot_ = initiatorSlot;
}

void LYConnectionQueueObject::setInitiatorArguments(QVariantList initiatorArguments){
	initiatorArguments_ = initiatorArguments;
}

void LYConnectionQueueObject::setInitiatorObject(QObject *initiatorObject, const char *initiatorSlot, QVariantList initiatorArguments){
	setInitiatorObject(initiatorObject);
	setInitiatorSlot(initiatorSlot);
	setInitiatorArguments(initiatorArguments);
}

void LYConnectionQueueObject::initiate(){
	connect(sender_, signal_, receiver_, slot_);
	connect(sender_, signal_, this, SLOT(onSignalReceived()));
	QString normalizedInitatorSlot = QString("%1").arg(initiatorSlot_).remove(0,1);
	int indexOfOpenParenthesis = normalizedInitatorSlot.indexOf('(');
	normalizedInitatorSlot = normalizedInitatorSlot.remove(indexOfOpenParenthesis, normalizedInitatorSlot.count()-indexOfOpenParenthesis);

	int initiatorArgumentsCount = initiatorArguments_.count();
	switch(initiatorArgumentsCount){
	case 0:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii());
		break;
	case 1:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()));
		break;
	case 2:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()));
		break;
	case 3:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()), QGenericArgument(initiatorArguments_.at(2).typeName(), initiatorArguments_.at(2).data()));
		break;
	case 4:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()), QGenericArgument(initiatorArguments_.at(2).typeName(), initiatorArguments_.at(2).data()), QGenericArgument(initiatorArguments_.at(3).typeName(), initiatorArguments_.at(3).data()));
		break;
	case 5:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()), QGenericArgument(initiatorArguments_.at(2).typeName(), initiatorArguments_.at(2).data()), QGenericArgument(initiatorArguments_.at(3).typeName(), initiatorArguments_.at(3).data()), QGenericArgument(initiatorArguments_.at(4).typeName(), initiatorArguments_.at(4).data()));
		break;
	}
	emit initiated(this);

}

void LYConnectionQueueObject::onSignalReceived(){
	disconnect(sender_, signal_, receiver_, slot_);
	disconnect(sender_, signal_, this, SLOT(onSignalReceived()));
	emit finished(this);
}

LYConnectionQueue::LYConnectionQueue(QObject *parent) :
	QObject(parent)
{
	queueStopped_ = true;
}

int LYConnectionQueue::queuedObjectsCount() const{
	return connetionQueue_.count();
}

int LYConnectionQueue::waitingObjectsCount() const{
	return initiatedButUnfinished_.count();
}

QStringList LYConnectionQueue::queuedObjects() const{
	QStringList retVal;
	for(int x = 0; x < connetionQueue_.count(); x++)
		retVal.append(connetionQueue_.at(x)->signal());
	return retVal;
}

QStringList LYConnectionQueue::waitingObjects() const{
	QStringList retVal;
	for(int x = 0; x < initiatedButUnfinished_.count(); x++)
		retVal.append(initiatedButUnfinished_.at(x)->signal());
	return retVal;
}

LYConnectionQueueObject* LYConnectionQueue::first(){
	return connetionQueue_.first();
}

LYConnectionQueueObject* LYConnectionQueue::objectAt(int index){
	return connetionQueue_[index];
}

void LYConnectionQueue::startQueue(){
	if(connetionQueue_.count() > 0){
		queueStopped_ = false;
		connetionQueue_.first()->initiate();
	}
}

void LYConnectionQueue::stopQueue(){
	queueStopped_ = true;
	qDebug() << "Stopping queue";
}

void LYConnectionQueue::clearQueue(){
	connetionQueue_.clear();
	initiatedButUnfinished_.clear();
	// need to delete the queueObjects?
}

void LYConnectionQueue::pushFrontConnectionQueueObject(LYConnectionQueueObject *queueObject){
	connetionQueue_.push_front(queueObject);
	connect(queueObject, SIGNAL(initiated(LYConnectionQueueObject*)), this, SLOT(onInitiated(LYConnectionQueueObject*)));
}

void LYConnectionQueue::pushBackConnectionQueueObject(LYConnectionQueueObject *queueObject){
	connetionQueue_.push_back(queueObject);
	connect(queueObject, SIGNAL(initiated(LYConnectionQueueObject*)), this, SLOT(onInitiated(LYConnectionQueueObject*)));
}

void LYConnectionQueue::onInitiated(LYConnectionQueueObject *queueObject){
	disconnect(queueObject, SIGNAL(initiated(LYConnectionQueueObject*)), this, SLOT(onInitiated(LYConnectionQueueObject*)));
	connect(queueObject, SIGNAL(finished(LYConnectionQueueObject*)), this, SLOT(onFinished(LYConnectionQueueObject*)));
	connetionQueue_.removeAll(queueObject);
	initiatedButUnfinished_.append(queueObject);
}

void LYConnectionQueue::onFinished(LYConnectionQueueObject *queueObject){
	disconnect(queueObject, SIGNAL(finished(LYConnectionQueueObject*)), this, SLOT(onFinished(LYConnectionQueueObject*)));
	initiatedButUnfinished_.removeAll(queueObject);

	if(connetionQueue_.count() > 0 && !queueStopped_)
		connetionQueue_.first()->initiate();
}
