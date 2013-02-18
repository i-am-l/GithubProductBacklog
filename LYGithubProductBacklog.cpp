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
		connectionQueue_.pop_front();

		qDebug() << "Successfully authenticated";
		//connect(this, SIGNAL(productBacklogOrderingReturned(QVariantMap)), this, SLOT(populateProductBacklog()));
		//retrieveProductBacklogOrdering();

		LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(this, SIGNAL(productBacklogOrderingReturned(QVariantMap)), this, SLOT(populateProductBacklog()), this, SLOT(retrieveProductBacklogOrdering()), this);
		connectionQueue_.push_back(connectionQueueObject);
		connectionQueue_.at(0)->initiate();
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
	Q_UNUSED(comment)
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

bool LYGithubProductBacklog::authenticateHelper(){
	if(username_.isEmpty() || password_.isEmpty() || repository_.isEmpty())
		return false;

	githubManager_->setUserName(username_);
	githubManager_->setPassword(password_);
	githubManager_->setRepository(repository_);

	//connect(githubManager_, SIGNAL(authenticated(bool)), this, SLOT(onGitAuthenticated(bool)));
	//githubManager_->authenticate();

	LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(githubManager_, SIGNAL(authenticated(bool)), this, SLOT(onGitAuthenticated(bool)), githubManager_, SLOT(authenticate()), this);
	connectionQueue_.push_back(connectionQueueObject);
	connectionQueue_.at(0)->initiate();

	return true;
}

void LYGithubProductBacklog::populateProductBacklog(){
	disconnect(this, 0, this, SLOT(populateProductBacklog()));
	githubManager_->getIssues(LYGithubManager::IssuesFilterAll);
	connect(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)), this, SLOT(onPopulateProductBacklogReturned(QList<QVariantMap>)));
}

#include <QMetaMethod>
void LYGithubProductBacklog::retrieveProductBacklogOrdering(){
	connectionQueue_.pop_front();

	if(ordingInformationCommentId_ < 0){
		qDebug() << "Have to do the work from scratch";
		//githubManager_->getIssues(LYGithubManager::IssuesFilterAll, LYGithubManager::IssuesStateClosed);

		const QMetaObject *mo = githubManager_->metaObject();
		//const char *slotInQuestion = SLOT(getIssues(LYGithubManager::IssuesFilter,LYGithubManager::IssuesState,LYGithubManager::IssuesSort,LYGithubManager::IssuesDirection));
		const char *slotInQuestion = SLOT(getIssues(LYGithubManager::IssuesFilter,LYGithubManager::IssuesState));
		QString slotInQuestionAsString = QString("%1").arg(slotInQuestion).remove(0, 1);
		qDebug() << "Slot is " << slotInQuestion << slotInQuestionAsString;
		int indexOfMethod = mo->indexOfMethod(slotInQuestionAsString.toAscii());
		qDebug() << "We have method index of " << indexOfMethod;
		QMetaMethod mm = mo->method(indexOfMethod);
		qDebug() << "Info about that call " << mm.parameterTypes();
		int indexOfOpenParenthesis = slotInQuestionAsString.indexOf('(');
		QString finalSlotString = slotInQuestionAsString.remove(indexOfOpenParenthesis, slotInQuestionAsString.count()-indexOfOpenParenthesis);

		QList<QGenericArgument> arguments;
		arguments.append(Q_ARG(LYGithubManager::IssuesFilter, LYGithubManager::IssuesFilterAll));
		arguments.append(Q_ARG(LYGithubManager::IssuesState, LYGithubManager::IssuesStateClosed));
		mo->invokeMethod(githubManager_, finalSlotString.toAscii(), arguments.at(0), arguments.at(1));

		connect(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)), this, SLOT(onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap>)));

		//LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(githubManager_, SIGNAL(authenticated(bool)), this, SLOT(onGitAuthenticated(bool)), githubManager_, SLOT(authenticate()), this);
		//connectionQueue_.push_back(connectionQueueObject);
		//connectionQueue_.at(0)->initiate();
	}
	else{
		qDebug() << "Already know the comment id, so we can cut corners";
		githubManager_->getSingleComment(ordingInformationCommentId_);
		connect(githubManager_, SIGNAL(singleCommentReturned(QVariantMap)), this, SLOT(onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap)));

		//LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(githubManager_, SIGNAL(authenticated(bool)), this, SLOT(onGitAuthenticated(bool)), githubManager_, SLOT(authenticate()), this);
		//connectionQueue_.push_back(connectionQueueObject);
		//connectionQueue_.at(0)->initiate();
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

LYConnectionQueueObject::LYConnectionQueueObject(QObject *sender, const char *signal, QObject *receiver, const char *slot, QObject *initiatorObject, const char *initiatorSlot, QObject *parent) :
	QObject(parent)
{
	sender_ = sender;
	signal_ = signal;
	receiver_ = receiver;
	slot_ = slot;
	initiatorObject_ = initiatorObject;
	initiatorSlot_ = initiatorSlot;
}

void LYConnectionQueueObject::initiate(){
	connect(sender_, signal_, receiver_, slot_);
	connect(sender_, signal_, this, SLOT(onSignalReceived()));
	QString normalizedInitatorSlot = QString("%1").arg(initiatorSlot_).remove(0,1).remove("()");
	initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii());
}

void LYConnectionQueueObject::onSignalReceived(){
	qDebug() << "Queue object heard the signal too";
	disconnect(sender_, signal_, receiver_, slot_);
	disconnect(sender_, signal_, this, SLOT(onSignalReceived()));
}
