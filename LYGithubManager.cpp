#include "LYGithubManager.h"

#include <QDebug>

LYGithubManager::LYGithubManager(QObject *parent) :
	QObject(parent)
{
	initialize();
}

LYGithubManager::LYGithubManager(const QString &userName, const QString &password, const QString &repository, QObject *parent) :
	QObject(parent)
{
	initialize();
	userName_ = userName;
	password_ = password;
	repository_ = repository;
	authenticate();
}

QString LYGithubManager::userName() const{
	return userName_;
}

bool LYGithubManager::isAuthenticated() const{
	return authenticated_;
}

QString LYGithubManager::repository() const{
	return repository_;
}

QString LYGithubManager::jsonSensiblePrint(const QVariantMap &jsonMap, int indentLevel) const{
	QString retVal;
	QString tabLevel;
	for(int x = 0; x < indentLevel; x++)
		tabLevel.append("\t");
	QMap<QString, QVariant>::const_iterator i = jsonMap.constBegin();
	while (i != jsonMap.constEnd()) {
		if(i.value().type() == QVariant::ULongLong)
			retVal.append(QString("%1\"%2\": \"%3\"\n").arg(tabLevel).arg(i.key()).arg(i.value().toULongLong()));
		else if(i.value().type() == QVariant::String)
			retVal.append(QString("%1\"%2\": \"%3\"\n").arg(tabLevel).arg(i.key()).arg(i.value().toString()));
		else if(i.value().canConvert(QVariant::Map))
			retVal.append(QString("%1\"%2\":\n%3").arg(tabLevel).arg(i.key()).arg(jsonSensiblePrint(i.value().toMap(), indentLevel+1)));
		++i;
	}
	return retVal;
}

void LYGithubManager::setUserName(const QString &userName){
	userName_ = userName;
}

void LYGithubManager::setPassword(const QString &password){
	password_ = password;
}

void LYGithubManager::setRepository(const QString &repository){
	repository_ = repository;
}

void LYGithubManager::authenticate(){
	if(authenticateReply_)
		return;
	QNetworkRequest request;
	QString authenticateURL = "https://api.github.com/user";

	request.setUrl(QUrl(authenticateURL));

	QString userInfo = userName_+":"+password_;
	QByteArray userData = userInfo.toLocal8Bit().toBase64();
	QString headerData = "Basic " + userData;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());

	authenticateReply_ = manager_->get(request);
	authenticateReply_->ignoreSslErrors();
	connect(authenticateReply_, SIGNAL(readyRead()), this, SLOT(onAuthenicatedRequestReturned()));
	connect(authenticateReply_, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onSomeErrorOccured(QNetworkReply::NetworkError)));
}

void LYGithubManager::getIssues(LYGithubManager::IssuesFilter filter, LYGithubManager::IssuesState state, LYGithubManager::IssuesSort sort, LYGithubManager::IssuesDirection direction, int page){
	if(!isAuthenticated() || repository_.isEmpty() || getIssuesReply_)
		return;

	fullIssuesReply_.clear();

	QNetworkRequest request;

	QString issuesURL = "https://api.github.com/repos/"+repository_+"/issues";
	QString issuesOptions = "?";
	issuesOptions.append("filter=");
	switch(filter){
	case LYGithubManager::IssuesFilterAssigned:
		issuesOptions.append("assigned&");
		break;
	case LYGithubManager::IssuesFilterCreated:
		issuesOptions.append("created&");
		break;
	case LYGithubManager::IssuesFilterMentioned:
		issuesOptions.append("mentioned&");
		break;
	case LYGithubManager::IssuesFilterSubscribed:
		issuesOptions.append("subscribed&");
		break;
	case LYGithubManager::IssuesFilterAll:
		issuesOptions.append("all&");
		break;
	default:
		issuesOptions.append("all&");
	}
	issuesOptions.append("state=");
	switch(state){
	case LYGithubManager::IssuesStateOpen:
		issuesOptions.append("open&");
		break;
	case LYGithubManager::IssuesStateClosed:
		issuesOptions.append("closed&");
		break;
	default:
		issuesOptions.append("open&");
	}
	issuesOptions.append("sort=");
	switch(sort){
	case LYGithubManager::IssuesSortCreated:
		issuesOptions.append("created&");
		break;
	case LYGithubManager::IssuesSortUpdated:
		issuesOptions.append("updated&");
		break;
	case LYGithubManager::IssuesSortComments:
		issuesOptions.append("comments&");
		break;
	}
	issuesOptions.append("direction=");
	switch(direction){
	case LYGithubManager::IssuesDirectionAscending:
		issuesOptions.append("asc&");
		break;
	case LYGithubManager::IssuesDirectionDescending:
		issuesOptions.append("desc&");
		break;
	}
	issuesOptions.append(QString("page=%1&").arg(page));
	// Above per_page=45 Qt seems to freak out for some reason.
	// The qjson parser freaks out if the string coming back is too big it seems. Tried some testing and it was inconclusive, it managed a QByteArray of 149904 but died after one of 118460.
	// Keeping this count lower and making sure there aren't ridiculously big comments is the solution right now.
	issuesOptions.append("per_page=30");
	qDebug() << "Requesting " << issuesURL+issuesOptions;
	lastGetIssuesRequest_ = issuesURL+issuesOptions;
	request.setUrl(QUrl(issuesURL+issuesOptions));

	QString userInfo = userName_+":"+password_;
	QByteArray userData = userInfo.toLocal8Bit().toBase64();
	QString headerData = "Basic " + userData;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());

	getIssuesReply_ = manager_->get(request);
	connect(getIssuesReply_, SIGNAL(readyRead()), this, SLOT(onIssuesReturned()));
}

void LYGithubManager::getSingleIssueComments(int issueNumber){
	if(!isAuthenticated() || repository_.isEmpty() || getSingleIssueCommentsReply_)
		return;
	QNetworkRequest request;

	QString issuesURL = QString("https://api.github.com/repos/%1/issues/%2/comments").arg(repository_).arg(issueNumber);
	request.setUrl(QUrl(issuesURL));

	QString userInfo = userName_+":"+password_;
	QByteArray userData = userInfo.toLocal8Bit().toBase64();
	QString headerData = "Basic " + userData;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());

	getSingleIssueCommentsReply_ = manager_->get(request);
	connect(getSingleIssueCommentsReply_, SIGNAL(readyRead()), this, SLOT(onSingleIssueCommentsReturned()));
}

void LYGithubManager::getSingleComment(int commentId){
	if(!isAuthenticated() || repository_.isEmpty() || getSingleCommentReply_)
		return;
	QNetworkRequest request;

	QString commentURL = QString("https://api.github.com/repos/%1/issues/comments/%2").arg(repository_).arg(commentId);
	request.setUrl(QUrl(commentURL));

	QString userInfo = userName_+":"+password_;
	QByteArray userData = userInfo.toLocal8Bit().toBase64();
	QString headerData = "Basic " + userData;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());

	getSingleCommentReply_ = manager_->get(request);
	connect(getSingleCommentReply_, SIGNAL(readyRead()), this, SLOT(onGetSingleCommentReturned()));
}

void LYGithubManager::editSingleComment(int commentId, const QString &newComment){
	if(!isAuthenticated() || repository_.isEmpty() || editSingleCommentReply_)
		return;
	QNetworkRequest request;

	QString commentURL = QString("https://api.github.com/repos/%1/issues/comments/%2").arg(repository_).arg(commentId);
	request.setUrl(QUrl(commentURL));

	QString userInfo = userName_+":"+password_;
	QByteArray userData = userInfo.toLocal8Bit().toBase64();
	QString headerData = "Basic " + userData;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());

	QVariantMap jdata;
	jdata["body"] = newComment;
	QJson::Serializer jserializer;
	QByteArray jsonData = jserializer.serialize(jdata);

	QBuffer *buffer = new QBuffer;
	buffer->setData(jsonData);
	buffer->open(QIODevice::ReadOnly);
	editSingleCommentReply_ = manager_->sendCustomRequest(request, "PATCH", buffer);
	buffer->setParent(editSingleCommentReply_);

	connect(editSingleCommentReply_, SIGNAL(readyRead()), this, SLOT(onEditSingleCommentReturned()));
}

void LYGithubManager::createNewIssue(const QString &title, const QString &body, const QString &assignee){
	if(!isAuthenticated() || repository_.isEmpty() || createNewIssueReply_)
		return;
	QNetworkRequest request;

	QString issuesURL = "https://api.github.com/repos/"+repository_+"/issues";
	request.setUrl(QUrl(issuesURL));

	QString userInfo = userName_+":"+password_;
	QByteArray userData = userInfo.toLocal8Bit().toBase64();
	QString headerData = "Basic " + userData;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());

	QVariantMap jdata;
	jdata["title"] = title;
	jdata["body"] = body;
	if(!assignee.isEmpty())
		jdata["assignee"] = assignee;
	QJson::Serializer jserializer;
	QByteArray jsonData = jserializer.serialize(jdata);
	//qdebug() << jsonData;

	createNewIssueReply_ = manager_->post(request, jsonData);
	createNewIssueReply_->ignoreSslErrors();
	connect(createNewIssueReply_, SIGNAL(readyRead()), this, SLOT(onCreateNewIssueReturned()));
}

void LYGithubManager::closeIssue(int issueNumber){
	if(!isAuthenticated() || repository_.isEmpty() || closeIssueReply_)
		return;
	QNetworkRequest request;

	QString commentURL = QString("https://api.github.com/repos/%1/issues/%2").arg(repository_).arg(issueNumber);
	request.setUrl(QUrl(commentURL));

	QString userInfo = userName_+":"+password_;
	QByteArray userData = userInfo.toLocal8Bit().toBase64();
	QString headerData = "Basic " + userData;
	request.setRawHeader("Authorization", headerData.toLocal8Bit());

	QVariantMap jdata;
	jdata["state"] = "closed";
	QJson::Serializer jserializer;
	QByteArray jsonData = jserializer.serialize(jdata);

	QBuffer *buffer = new QBuffer;
	buffer->setData(jsonData);
	buffer->open(QIODevice::ReadOnly);
	closeIssueReply_ = manager_->sendCustomRequest(request, "PATCH", buffer);
	buffer->setParent(closeIssueReply_);

	connect(closeIssueReply_, SIGNAL(readyRead()), this, SLOT(onCloseIssueReturned()));
}

void LYGithubManager::onAuthenicatedRequestReturned(){
	QJson::Parser parser;
	QVariant githubFullReply = parser.parse(authenticateReply_->readAll());
	authenticated_ = false;
	if(githubFullReply.canConvert(QVariant::Map)){
		QVariantMap jsonMap = githubFullReply.toMap();
		QMap<QString, QVariant>::const_iterator i = jsonMap.constBegin();
		if(i.key() == "message" && i.value().toString() == "Bad credentials")
			authenticated_ = false;
		else
			authenticated_ = true;
	}
	disconnect(authenticateReply_, 0);
	authenticateReply_->deleteLater();
	authenticateReply_ = 0;
	emit authenticated(authenticated_);
}

#include <QStringList>
void LYGithubManager::onIssuesReturned(){

	QList<QByteArray> headerList = getIssuesReply_->rawHeaderList();
	for(int x = 0; x < headerList.count(); x++){
		if(headerList.at(x) == "Link" && lastPageNumber_ == -1){
			QString linkHeader = getIssuesReply_->rawHeader(headerList.at(x));
			int lastPageNumber = -1;
			int nextPageNumber = -1;
			QStringList linkHeaderItems = linkHeader.split(',');
			for(int y = 0; y < linkHeaderItems.count(); y++){
				if(linkHeaderItems.at(y).contains("; rel=\"last\""))
					lastPageNumber = pageNumberFromURLString(linkHeaderItems.at(y));
				if(linkHeaderItems.at(y).contains("; rel=\"next\""))
					nextPageNumber = pageNumberFromURLString(linkHeaderItems.at(y));
			}

			lastPageNumber_ = lastPageNumber;
		}
	}

	int currentPageNumber = -1;
	if(lastPageNumber_ != -1)
		currentPageNumber = pageNumberFromURLString(lastGetIssuesRequest_);

	QJson::Parser parser;
	QVariant githubFullReply = parser.parse(getIssuesReply_->readAll());
	bool doEmit = false;
	QList<QVariantMap> localRetVal;
	QVariantMap oneIssue;
	if(githubFullReply.canConvert(QVariant::List)){
		QVariantList githubListReply = githubFullReply.toList();
		if(githubListReply.at(0).canConvert(QVariant::Map)){
			if((lastPageNumber_ == -1) || (currentPageNumber == lastPageNumber_) )
				doEmit = true;
			for(int x = 0; x < githubListReply.count(); x++){
				oneIssue = githubListReply.at(x).toMap();
				localRetVal.append(oneIssue);
			}
		}
	}

	fullIssuesReply_.append(localRetVal);

	disconnect(getIssuesReply_, 0);
	getIssuesReply_->deleteLater();
	getIssuesReply_ = 0;

	if((lastPageNumber_ != -1) && (currentPageNumber != lastPageNumber_)){

		QNetworkRequest request;

		QString currentPageNumberString = QString("&page=%1").arg(currentPageNumber);
		QString nextPageNumberString = QString("&page=%1").arg(currentPageNumber+1);
//		qDebug() << "Last request as " << lastGetIssuesRequest_;
		lastGetIssuesRequest_.replace(currentPageNumberString, nextPageNumberString);
//		qDebug() << "Request again as " << lastGetIssuesRequest_;
		request.setUrl(QUrl(lastGetIssuesRequest_));

		QString userInfo = userName_+":"+password_;
		QByteArray userData = userInfo.toLocal8Bit().toBase64();
		QString headerData = "Basic " + userData;
		request.setRawHeader("Authorization", headerData.toLocal8Bit());

		getIssuesReply_ = manager_->get(request);
		connect(getIssuesReply_, SIGNAL(readyRead()), this, SLOT(onIssuesReturned()));
	}

	if(doEmit){
		lastPageNumber_ = -1;
		//emit issuesReturned(localRetVal);
		emit issuesReturned(fullIssuesReply_);
	}
}

void LYGithubManager::onSingleIssueCommentsReturned(){
	QJson::Parser parser;
	QVariant githubFullReply = parser.parse(getSingleIssueCommentsReply_->readAll());
	bool doEmit = false;
	QVariantMap retVal;
	if(githubFullReply.canConvert(QVariant::List)){
		QVariantList githubListReply = githubFullReply.toList();
		if(githubListReply.at(0).canConvert(QVariant::Map)){
			doEmit = true;
			retVal = githubListReply.at(0).toMap();
		}
	}
	disconnect(getSingleIssueCommentsReply_, 0);
	getSingleIssueCommentsReply_->deleteLater();
	getSingleIssueCommentsReply_ = 0;
	if(doEmit)
		emit singleIssueCommentReturned(retVal);
}

void LYGithubManager::onGetSingleCommentReturned(){
	QJson::Parser parser;
	QVariant githubFullReply = parser.parse(getSingleCommentReply_->readAll());
	bool doEmit = false;
	QVariantMap retVal;
	if(githubFullReply.canConvert(QVariant::Map)){
		doEmit = true;
		retVal = githubFullReply.toMap();
	}
	disconnect(getSingleCommentReply_, 0);
	getSingleCommentReply_->deleteLater();
	getSingleCommentReply_ = 0;
	if(doEmit)
		emit singleCommentReturned(retVal);
}

void LYGithubManager::onEditSingleCommentReturned(){
	QJson::Parser parser;
	QVariant githubFullReply = parser.parse(editSingleCommentReply_->readAll());
	bool doEmit = false;
	QVariantMap retVal;
	if(githubFullReply.canConvert(QVariant::Map)){
		doEmit = true;
		retVal = githubFullReply.toMap();
	}
	disconnect(editSingleCommentReply_, 0);
	editSingleCommentReply_->deleteLater();
	editSingleCommentReply_ = 0;
	if(doEmit)
		emit singleCommentEdited(retVal);
}

void LYGithubManager::onCreateNewIssueReturned(){
	QJson::Parser parser;
	bool retVal = false;
	QVariantMap retMap;
	if(createNewIssueReply_->rawHeader("Status") == "201 Created")
		retVal = true;

	QVariant githubFullReply = parser.parse(createNewIssueReply_->readAll());
	if(githubFullReply.canConvert(QVariant::Map))
		retMap = githubFullReply.toMap();

	disconnect(createNewIssueReply_, 0);
	createNewIssueReply_->deleteLater();
	createNewIssueReply_ = 0;
	emit issueCreated(retVal, retMap);
}

void LYGithubManager::onCloseIssueReturned(){
	QJson::Parser parser;
	bool retVal = false;
	QVariantMap retMap;
	if(closeIssueReply_->rawHeader("Status") == "200 OK")
		retVal = true;

	QVariant githubFullReply = parser.parse(closeIssueReply_->readAll());
	if(githubFullReply.canConvert(QVariant::Map))
		retMap = githubFullReply.toMap();

	disconnect(closeIssueReply_, 0);
	closeIssueReply_->deleteLater();
	closeIssueReply_ = 0;
	emit issueClosed(retVal, retMap);
}

void LYGithubManager::onSomeErrorOccured(QNetworkReply::NetworkError nError){
	qDebug() << "Error occurred " << nError;
}

void LYGithubManager::initialize(){
	manager_ = new QNetworkAccessManager(this);
	userName_ = "";
	password_ = "";
	repository_ = "";
	authenticateReply_ = 0;
	getIssuesReply_ = 0;
	lastPageNumber_ = -1;
	getSingleIssueCommentsReply_ = 0;
	getSingleCommentReply_ = 0;
	editSingleCommentReply_ = 0;
	createNewIssueReply_ = 0;
	closeIssueReply_ = 0;
	authenticated_ = false;
}

int LYGithubManager::pageNumberFromURLString(const QString &urlString) const{
	QRegExp pageNumberRegExp = QRegExp("page=(\\d+)");
	pageNumberRegExp.indexIn(urlString);
	bool conversionOk = false;
	int retVal = pageNumberRegExp.cap(1).toInt(&conversionOk);
	if(!conversionOk)
		retVal = -1;
	return retVal;
}
