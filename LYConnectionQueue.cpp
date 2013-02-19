#include "LYConnectionQueue.h"

#include <QDebug>

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
