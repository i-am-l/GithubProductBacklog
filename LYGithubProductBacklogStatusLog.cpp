#include "LYGithubProductBacklogStatusLog.h"

LYGithubProductBacklogStatusLog *LYGithubProductBacklogStatusLog::instance_ = 0;

LYGithubProductBacklogStatusLog::LYGithubProductBacklogStatusLog(QObject *parent) :
	QObject(parent)
{
	model_ = new QStringListModel(statusLog_);
}

void LYGithubProductBacklogStatusLog::releaseStatusLog(){
	if(instance_){
		delete instance_;
		instance_ = 0;
	}
}

QStringListModel* LYGithubProductBacklogStatusLog::model(){
	return model_;
}

LYGithubProductBacklogStatusLog* LYGithubProductBacklogStatusLog::statusLog(){
	if(!instance_)
		instance_ = new LYGithubProductBacklogStatusLog();

	return instance_;
}

void LYGithubProductBacklogStatusLog::appendStatusMessage(const QString &statusMessage){
	statusLog_.prepend(QString("%1 - %2").arg(statusLog_.count()+1, 3, 10, QChar('0')).arg(statusMessage));
	model_->setStringList(statusLog_);
}
