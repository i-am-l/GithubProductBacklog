#include "LYConnectionQueueObject.h"

LYConnectionQueueObject::LYConnectionQueueObject(QObject *parent) :
	QObject(parent)
{

}

QString LYConnectionQueueObject::signal() const{
	QString retVal = QString("%1").arg(signal_);
	return retVal;
}

void LYConnectionQueueObject::setSender(QObject *sender){
	sender_ = sender;
}

void LYConnectionQueueObject::setSignal(const char *signal){
	signal_ = signal;
}

void LYConnectionQueueObject::setSender(QObject *sender, const char *signal){
	setSender(sender);
	setSignal(signal);
}

void LYConnectionQueueObject::setReceiver(QObject *receiver){
	receiver_ = receiver;
}

void LYConnectionQueueObject::setSlot(const char *slot){
	slot_ = slot;
}

void LYConnectionQueueObject::setReceiver(QObject *receiver, const char *slot){
	setReceiver(receiver);
	setSlot(slot);
}

void LYConnectionQueueObject::setInitiatorObject(QObject *initiatorObject){
	initiatorObject_ = initiatorObject;
}

void LYConnectionQueueObject::setInitiatorSlot(const char *initiatorSlot){
	initiatorSlot_ = initiatorSlot;
}

void LYConnectionQueueObject::setInitiatorArguments(QVariantList initiatorArguments){
	initiatorArguments_ = initiatorArguments;
}

void LYConnectionQueueObject::setInitiatorObject(QObject *initiatorObject, const char *initiatorSlot, QVariantList initiatorArguments){
	setInitiatorObject(initiatorObject);
	setInitiatorSlot(initiatorSlot);
	setInitiatorArguments(initiatorArguments);
}

void LYConnectionQueueObject::initiate(){
	connect(sender_, signal_, receiver_, slot_);
	connect(sender_, signal_, this, SLOT(onSignalReceived()));
	QString normalizedInitatorSlot = QString("%1").arg(initiatorSlot_).remove(0,1);
	int indexOfOpenParenthesis = normalizedInitatorSlot.indexOf('(');
	normalizedInitatorSlot = normalizedInitatorSlot.remove(indexOfOpenParenthesis, normalizedInitatorSlot.count()-indexOfOpenParenthesis);

	int initiatorArgumentsCount = initiatorArguments_.count();
	switch(initiatorArgumentsCount){
	case 0:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii());
		break;
	case 1:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()));
		break;
	case 2:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()));
		break;
	case 3:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()), QGenericArgument(initiatorArguments_.at(2).typeName(), initiatorArguments_.at(2).data()));
		break;
	case 4:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()), QGenericArgument(initiatorArguments_.at(2).typeName(), initiatorArguments_.at(2).data()), QGenericArgument(initiatorArguments_.at(3).typeName(), initiatorArguments_.at(3).data()));
		break;
	case 5:
		initiatorObject_->metaObject()->invokeMethod(initiatorObject_, normalizedInitatorSlot.toAscii(), QGenericArgument(initiatorArguments_.at(0).typeName(), initiatorArguments_.at(0).data()), QGenericArgument(initiatorArguments_.at(1).typeName(), initiatorArguments_.at(1).data()), QGenericArgument(initiatorArguments_.at(2).typeName(), initiatorArguments_.at(2).data()), QGenericArgument(initiatorArguments_.at(3).typeName(), initiatorArguments_.at(3).data()), QGenericArgument(initiatorArguments_.at(4).typeName(), initiatorArguments_.at(4).data()));
		break;
	}
	emit initiated(this);

}

void LYConnectionQueueObject::onSignalReceived(){
	disconnect(sender_, signal_, receiver_, slot_);
	disconnect(sender_, signal_, this, SLOT(onSignalReceived()));
	emit finished(this);
}
