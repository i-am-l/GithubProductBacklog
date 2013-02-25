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

	productBacklogModel_ = new LYProductBacklogModel(this);
	connect(productBacklogModel_, SIGNAL(modelRefreshed()), this, SLOT(onProductBacklogModelRefreshed()));

	githubManager_ = new LYGithubManager(this);

	createStartupConnectionQueue();
	authenticateHelper();
}

QAbstractItemModel* LYGithubProductBacklog::model() const{
	return productBacklogModel_;
}

LYProductBacklogModel* LYGithubProductBacklog::productBacklogModel() const{
	return productBacklogModel_;
}

void LYGithubProductBacklog::fixStartupIssues(){
	productBacklogModel_->fixParseIssues(orderingInformation_, issues_);
	uploadChanges();
}

void LYGithubProductBacklog::uploadChanges(){
	createUploadChangesConnectionQueue();
	QVariantList arguments;
	arguments.append(QVariant::fromValue(ordingInformationCommentId_));
	uploadChangesConnectionQueue_.first()->setInitiatorArguments(arguments);

	uploadChangesConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::createNewIssue(const QString &title, const QString &body){
	createCreateNewIssueConnectionQueue();
	QVariantList arguments;
	arguments.append(QVariant::fromValue(title));
	arguments.append(QVariant::fromValue(body));
	createNewIssueConnectionQueue_.first()->setInitiatorArguments(arguments);

	createNewIssueConnectionQueue_.startQueue();
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

QStringList LYGithubProductBacklog::missingIssues() const{
	QStringList retVal;
	QList<int> unorderedIssuesFound = productBacklogModel_->unorderedIssuesFound();
	for(int x = 0; x < unorderedIssuesFound.count(); x++)
		retVal.append(QString("%1 - %2").arg(unorderedIssuesFound.at(x)).arg(productBacklogModel_->titleFromIssueNumber(unorderedIssuesFound.at(x))));
	return retVal;
}

QStringList LYGithubProductBacklog::orderedIssuesWithoutChildren() const{
	QStringList retVal;
	QList<int> orderedIssuesWithoutChildrenNotFound = productBacklogModel_->orderedIssuesWithoutChildrenNotFound();
	for(int x = 0; x < orderedIssuesWithoutChildrenNotFound.count(); x++)
		retVal.append(QString("%1 - %2").arg(orderedIssuesWithoutChildrenNotFound.at(x)).arg(productBacklogModel_->titleFromIssueNumber(orderedIssuesWithoutChildrenNotFound.at(x))));
	return retVal;
}

QStringList LYGithubProductBacklog::orderedIssuesWithChildren() const{
	QStringList retVal;
	QList<int> orderedIssuesWithChildrenNotFound = productBacklogModel_->orderedIssuesWithChildrenNotFound();
	for(int x = 0; x < orderedIssuesWithChildrenNotFound.count(); x++)
		retVal.append(QString("%1 - %2").arg(orderedIssuesWithChildrenNotFound.at(x)).arg(productBacklogModel_->titleFromIssueNumber(orderedIssuesWithChildrenNotFound.at(x))));
	return retVal;
}

void LYGithubProductBacklog::onGitAuthenticated(bool wasAuthenticated){
	if(wasAuthenticated)
		qDebug() << "Successfully authenticated";
	else
		qDebug() << "Could not authenticate";

	emit authenticated(wasAuthenticated);
}

void LYGithubProductBacklog::onPopulateProductBacklogReturned(QList<QVariantMap> issues){
	issues_ = issues;
	LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck = productBacklogModel_->parseList(orderingInformation_, issues_);
	emit sanityCheckReturned(sanityCheck);
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap> issues){
	int issueNumber = -1;
	for(int x = 0; x < issues.count(); x++)
		if(issues.at(x).value("title").toString() == "ProductBacklogInfo")
			issueNumber = issues.at(x).value("number").toInt();

	closedIssues_ = issues;
	productBacklogModel_->setClosedIssues(closedIssues_);

	if(issueNumber > 0){
		QVariantList arguments;
		arguments.append(QVariant(issueNumber));
		startupConnectionQueue_.first()->setInitiatorArguments(arguments);
	}
	else{
		startupConnectionQueue_.stopQueue();
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
	QString repositoryCurrentOrdering = comment.value("body").toString();

	if(repositoryCurrentOrdering == orderingInformation_){
		qDebug() << "No repository side changes, proceeding with updates";

		QString newOrderingInformation = productBacklogModel_->generateListNotation();
		QVariantList arguments;
		arguments.append(QVariant::fromValue(ordingInformationCommentId_));
		arguments.append(QVariant::fromValue(newOrderingInformation));
		uploadChangesConnectionQueue_.first()->setInitiatorArguments(arguments);
	}
	else{
		uploadChangesConnectionQueue_.stopQueue();
		uploadChangesConnectionQueue_.clearQueue();

		qDebug() << "Repository side changes detected, cannot proceed with updates";
	}
}

void LYGithubProductBacklog::onUploadChangesReturned(QVariantMap comment){
	orderingInformation_ = comment.value("body").toString();
	activeChanges_ = false;
	emit activeChanges(false);
}

void LYGithubProductBacklog::onCreateNewIssueReturned(bool issueCreatedSuccessfully, QVariantMap newIssue){
	emit newIssueCreated(issueCreatedSuccessfully);

	if(issueCreatedSuccessfully){
		QString updatedOrderingInformation = orderingInformation_;
		updatedOrderingInformation.append(QString("%1;").arg(newIssue.value("number").toInt()));
		QList<QVariantMap> updatedIssues = issues_;
		updatedIssues.append(newIssue);

		productBacklogModel_->parseList(updatedOrderingInformation, updatedIssues);
		uploadChanges();
	}
}

void LYGithubProductBacklog::onProductBacklogModelRefreshed(){
	bool hasChanges = false;
	if(orderingInformation_ != productBacklogModel_->generateListNotation())
		hasChanges = true;

	if(activeChanges_ != hasChanges){
		activeChanges_ = hasChanges;
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

void LYGithubProductBacklog::createCreateNewIssueConnectionQueue(){
	createNewIssueConnectionQueue_.clearQueue();

	LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(issueCreated(bool, QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onCreateNewIssueReturned(bool, QVariantMap)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(createNewIssue(QString,QString)));
	createNewIssueConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);
}
