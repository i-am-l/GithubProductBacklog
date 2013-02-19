#ifndef LYCONNECTIONQUEUE_H
#define LYCONNECTIONQUEUE_H

#include "LYConnectionQueueObject.h"
#include <QStringList>

class LYConnectionQueue : public QObject
{
Q_OBJECT
public:
	LYConnectionQueue(QObject *parent = 0);

	int queuedObjectsCount() const;
	int waitingObjectsCount() const;

	QStringList queuedObjects() const;
	QStringList waitingObjects() const;

	LYConnectionQueueObject* first();
	LYConnectionQueueObject* objectAt(int index);

public slots:
	void startQueue();
	void stopQueue();
	void clearQueue();

	void pushFrontConnectionQueueObject(LYConnectionQueueObject *queueObject);
	void pushBackConnectionQueueObject(LYConnectionQueueObject *queueObject);

protected slots:
	void onInitiated(LYConnectionQueueObject *queueObject);
	void onFinished(LYConnectionQueueObject *queueObject);

protected:
	QList<LYConnectionQueueObject*> connetionQueue_;
	QList<LYConnectionQueueObject*> initiatedButUnfinished_;

	bool queueStopped_;
};

#endif // LYCONNECTIONQUEUE_H
