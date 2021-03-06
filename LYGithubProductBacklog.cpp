#include "LYGithubProductBacklog.h"

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
	emit networkRequestBusy(true, "Uploading Changes");
	createUploadChangesConnectionQueue();

	uploadChangesConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::createNewIssue(const QString &title, const QString &body){
	emit networkRequestBusy(true, "Creating Issue");
	createCreateNewIssueConnectionQueue();
	QVariantList arguments;
	arguments.append(QVariant::fromValue(title));
	arguments.append(QVariant::fromValue(body));
	createNewIssueConnectionQueue_.first()->setInitiatorArguments(arguments);

	createNewIssueConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::closeIssue(int issueNumber){
	emit networkRequestBusy(true, "Closing Issue");
	createCloseIssueConnectionQueue();
	QVariantList arguments;
	arguments.append(QVariant::fromValue(issueNumber));
	closeIssueConnectionQueue_.first()->setInitiatorArguments(arguments);

	closeIssueConnectionQueue_.startQueue();
}

void LYGithubProductBacklog::setUserName(const QString &username){
	username_ = username;
	if(authenticateHelper()){
		emit networkRequestBusy(true, "Populating Product Backlog");
		startupConnectionQueue_.startQueue();
	}
}

void LYGithubProductBacklog::setPassword(const QString &password){
	password_ = password;
	if(authenticateHelper()){
		emit networkRequestBusy(true, "Populating Product Backlog");
		startupConnectionQueue_.startQueue();
	}
}

void LYGithubProductBacklog::setRepository(const QString &repository){
	repository_ = repository;
	if(authenticateHelper()){
		emit networkRequestBusy(true, "Populating Product Backlog");
		startupConnectionQueue_.startQueue();
	}
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
	emit authenticated(wasAuthenticated);
}

void LYGithubProductBacklog::onPopulateProductBacklogReturned(QList<QVariantMap> issues){
	issues_ = issues;
	LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck = productBacklogModel_->parseList(orderingInformation_, issues_);
	emit sanityCheckReturned(sanityCheck);
	emit networkRequestBusy(false, "");
}

void LYGithubProductBacklog::onPopulateProductBacklogGetFileContentsReturned(QVariantMap fileContents){
	QString rawFileContents = fileContents.value("content").toString();
	QString decodedFileContents = QByteArray::fromBase64(rawFileContents.toLocal8Bit());

	QString fileSHA = fileContents.value("sha").toString();

	orderingInformation_ = decodedFileContents.remove("\n");
	orderingInformationSHA_ = fileSHA;
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap> issues){
	int issueNumber = -1;

	for(int x = 0; x < issues.count(); x++)
		if(issues.at(x).value("title").toString() == "ProductBacklogInfo")
			issueNumber = issues.at(x).value("number").toInt();

	closedIssues_ = issues;
	productBacklogModel_->setClosedIssues(closedIssues_);
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingInfoIssueCommentsReturned(QVariantMap comments){
	ordingInformationCommentId_ = comments.value("id").toInt();

	emit productBacklogOrderingReturned(comments);
}

void LYGithubProductBacklog::onPopulateProductBacklogOrderingDirectOrderingCommentReturned(QVariantMap comment){
	emit productBacklogOrderingReturned(comment);
}

void LYGithubProductBacklog::onUploadChangedCheckedOrderingReturn(QVariantMap fileContents){
	QString currentRepositorySHA = fileContents.value("sha").toString();

	bool canAttemptUpload = false;
	if(currentRepositorySHA == orderingInformationSHA_){
		canAttemptUpload = true;

		QString newOrderingInformation = productBacklogModel_->generateListNotation();
		QVariantList arguments;
		arguments.append(QVariant::fromValue(QString("ProductBacklogData.txt")));
		arguments.append(QVariant::fromValue(QString("Updating product backlog")));
		arguments.append(QVariant::fromValue(newOrderingInformation));
		arguments.append(QVariant::fromValue(orderingInformationSHA_));
		uploadChangesConnectionQueue_.first()->setInitiatorArguments(arguments);
	}
	else{
		uploadChangesConnectionQueue_.stopQueue();
		uploadChangesConnectionQueue_.clearQueue();
		emit uploaded(false);
	}
	emit networkRequestBusy(false, "");
}

void LYGithubProductBacklog::onUploadChangesReturned(bool updated, QVariantMap fileContents){
	if(updated){
		orderingInformation_ = productBacklogModel_->generateListNotation();
		orderingInformationSHA_ = fileContents.value("content").toMap().value("sha").toString();
	}
	activeChanges_ = false;
	emit activeChanges(false);
	emit uploaded(updated);
}

void LYGithubProductBacklog::onCreateNewIssueReturned(bool issueCreatedSuccessfully, QVariantMap newIssue){
	emit newIssueCreated(issueCreatedSuccessfully);

	if(issueCreatedSuccessfully){
		QString updatedOrderingInformation = orderingInformation_;
		updatedOrderingInformation.append(QString("%1;").arg(newIssue.value("number").toInt()));
		issues_.append(newIssue);

		productBacklogModel_->parseList(updatedOrderingInformation, issues_);
		emit networkRequestBusy(false, "");
		uploadChanges();
	}
}

void LYGithubProductBacklog::onCloseIssueReturned(bool issueClosedSuccessfully, QVariantMap closedIssue){
	emit issueClosed(issueClosedSuccessfully);

	if(issueClosedSuccessfully){
		int closedIssueNumber = closedIssue.value("number").toInt();
		QString updatedOrderingInformation = orderingInformation_;
		updatedOrderingInformation.remove(QString("%1;").arg(closedIssueNumber));

		for(int x = 0; x < issues_.count(); x++)
			if(issues_.at(x).value("number").toInt() == closedIssueNumber)
				issues_.removeAt(x);

		productBacklogModel_->parseList(updatedOrderingInformation, issues_);
		emit networkRequestBusy(false, "");
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

#include <QDebug>
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

	// Do authentication
	LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(authenticated(bool)));
	connectionQueueObject->setReceiver(this, SLOT(onGitAuthenticated(bool)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(authenticate()));
	startupConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	// Get closed issues
	connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(issuesReturned(QList<QVariantMap>)));
	connectionQueueObject->setReceiver(this, SLOT(onPopulateProductBacklogOrderingFindIssueReturned(QList<QVariantMap>)));
	QVariantList arguments1;
	arguments1.append(QVariant::fromValue(LYGithubManager::IssuesFilterAll));
	arguments1.append(QVariant::fromValue(LYGithubManager::IssuesStateClosed));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(getIssues()), arguments1);
	startupConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	// Get ordering information and its file SHA
	connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(fileContentsReturned(QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onPopulateProductBacklogGetFileContentsReturned(QVariantMap)));
	QVariantList arguments3;
	arguments3.append(QVariant::fromValue(QString("ProductBacklogData.txt")));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(getFileContents(QString)), arguments3);
	startupConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	// Get open issues
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

	// Check if the product backlog information has changed since we last got it
	LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(fileContentsReturned(QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onUploadChangedCheckedOrderingReturn(QVariantMap)));
	QVariantList arguments;
	arguments.append(QVariant::fromValue(QString("ProductBacklogData.txt")));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(getFileContents(QString)), arguments);
	uploadChangesConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);

	// Update the product backlog file
	connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(fileContentsUpdated(bool,QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onUploadChangesReturned(bool,QVariantMap)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(updateFileContents(QString,QString,QString,QString)));
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

void LYGithubProductBacklog::createCloseIssueConnectionQueue(){
	closeIssueConnectionQueue_.clearQueue();

	LYConnectionQueueObject *connectionQueueObject = new LYConnectionQueueObject(this);
	connectionQueueObject->setSender(githubManager_, SIGNAL(issueClosed(bool,QVariantMap)));
	connectionQueueObject->setReceiver(this, SLOT(onCloseIssueReturned(bool,QVariantMap)));
	connectionQueueObject->setInitiatorObject(githubManager_, SLOT(closeIssue(int)));
	closeIssueConnectionQueue_.pushBackConnectionQueueObject(connectionQueueObject);
}
