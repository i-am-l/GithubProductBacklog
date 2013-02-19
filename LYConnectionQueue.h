#ifndef LYCONNECTIONQUEUE_H
#define LYCONNECTIONQUEUE_H

#include "LYConnectionQueueObject.h"
#include <QStringList>

class LYConnectionQueue : public QObject
{
Q_OBJECT
public:
	/// Constructor creates an empty queue
	LYConnectionQueue(QObject *parent = 0);

	/// Returns the number of objects that have yet to be initiated
	int queuedObjectsCount() const;
	/// Returns the number of objects that have been initiated but have not yet finished
	int waitingObjectsCount() const;

	/// Returns the first item in the queue that is yet to be initiated (head of the queue)
	LYConnectionQueueObject* first();
	/// Returns any object in the queue by index
	LYConnectionQueueObject* objectAt(int index);

public slots:
	/// Starts the first item in the queue. Unless stopQueue() is called when on object is finished the next will be initiated
	void startQueue();
	/// Stops the queue if it is running. Nothing can stop the current object, but the next object will not be initiated
	void stopQueue();
	/// Removes all objects from the queue and the unfinished list
	void clearQueue();

	/// Puts a new object at the front of the queue
	void pushFrontConnectionQueueObject(LYConnectionQueueObject *queueObject);
	/// Puts a new object at the back of the queue
	void pushBackConnectionQueueObject(LYConnectionQueueObject *queueObject);

protected slots:
	/// Handles disconnects and connects as well as moving objects between lists when an object has been successfully initiated
	void onInitiated(LYConnectionQueueObject *queueObject);
	/// Handles disconnects, removal from lists, and initiating the next object when one object finishes
	void onFinished(LYConnectionQueueObject *queueObject);

protected:
	/// List of objects to be run (acts as a queue)
	QList<LYConnectionQueueObject*> connetionQueue_;
	/// List of objects that have been initiated but have not finished yet
	QList<LYConnectionQueueObject*> initiatedButUnfinished_;

	/// Internal for determing if the queue should proceed to initiate the next object
	bool queueStopped_;
};

#endif // LYCONNECTIONQUEUE_H
