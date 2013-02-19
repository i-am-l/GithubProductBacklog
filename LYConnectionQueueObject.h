#ifndef LYCONNECTIONQUEUEOBJECT_H
#define LYCONNECTIONQUEUEOBJECT_H

#include <QObject>
#include <QVariant>

class LYConnectionQueueObject : public QObject
{
Q_OBJECT
public:
	/// Constructs an empty object. To be useful the slots have to be called to set senders, receivers, and initiators
	LYConnectionQueueObject(QObject *parent = 0);

public slots:
	/// Sets the sender object
	void setSender(QObject *sender);
	/// Sets the sender signal we're waiting for
	void setSignal(const char *signal);
	/// Sets both the sender object and the signal
	void setSender(QObject *sender, const char *signal);

	/// Sets the receiver object
	void setReceiver(QObject *receiver);
	/// Sets the receiver slot we'll connect to
	void setSlot(const char *slot);
	/// Sets both the receiver object and the slot
	void setReceiver(QObject *receiver, const char *slot);

	/// Sets the initiator object
	void setInitiatorObject(QObject *initiatorObject);
	/// Sets the initiator slot we'll call to start the whole sequence
	void setInitiatorSlot(const char *initiatorSlot);
	/// Sets the arguments for the initiator slot (can take as many as five arguments)
	void setInitiatorArguments(QVariantList initiatorArguments);
	/// Sets the initiator object, initiator slot, and the arguments all at the same time
	void setInitiatorObject(QObject *initiatorObject, const char *initiatorSlot, QVariantList initiatorArguments = QVariantList());

	/// Calls the desired slot with the specified arguments on the initiator object
	void initiate();

protected slots:
	/// Handles disconnects when the sender's signal is received as well as emitting finished
	void onSignalReceived();

signals:
	/// Emitted after we have successfully called the initiator slot on the initiator object
	void initiated(LYConnectionQueueObject *queueObject);
	/// Emitted when the sender's signal is received and after we have disconnected all signals and slots
	void finished(LYConnectionQueueObject *queueObject);

protected:
	/// Holds the sender
	QObject *sender_;
	/// Holds the signature of the signal
	const char *signal_;
	/// Holds the receiver
	QObject *receiver_;
	/// Holds the signature of the slot
	const char *slot_;
	/// Holds the initiator object
	QObject *initiatorObject_;
	/// Holds the signature of the initiator slot
	const char *initiatorSlot_;
	/// Holds the initiator arguments
	QVariantList initiatorArguments_;
};

#endif // LYCONNECTIONQUEUEOBJECT_H
