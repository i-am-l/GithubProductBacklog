#ifndef LYCONNECTIONQUEUEOBJECT_H
#define LYCONNECTIONQUEUEOBJECT_H

#include <QObject>
#include <QVariant>

class LYConnectionQueueObject : public QObject
{
Q_OBJECT
public:
	LYConnectionQueueObject(QObject *parent = 0);

	QString signal() const;

	void printInitiatorArguments() const;

public slots:
	void setSender(QObject *sender);
	void setSignal(const char *signal);
	void setSender(QObject *sender, const char *signal);

	void setReceiver(QObject *receiver);
	void setSlot(const char *slot);
	void setReceiver(QObject *receiver, const char *slot);


	void setInitiatorObject(QObject *initiatorObject);
	void setInitiatorSlot(const char *initiatorSlot);
	void setInitiatorArguments(QVariantList initiatorArguments);
	void setInitiatorObject(QObject *initiatorObject, const char *initiatorSlot, QVariantList initiatorArguments = QVariantList());

	void initiate();

protected slots:
	void onSignalReceived();

signals:
	void initiated(LYConnectionQueueObject *queueObject);
	void finished(LYConnectionQueueObject *queueObject);

protected:
	QObject *sender_;
	const char *signal_;
	QObject *receiver_;
	const char *slot_;
	QObject *initiatorObject_;
	const char *initiatorSlot_;
	QVariantList initiatorArguments_;
};

#endif // LYCONNECTIONQUEUEOBJECT_H
