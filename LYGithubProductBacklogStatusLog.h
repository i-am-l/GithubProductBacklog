#ifndef LYGITHUBPRODUCTBACKLOGSTATUSLOG_H
#define LYGITHUBPRODUCTBACKLOGSTATUSLOG_H

#include <QObject>
#include <QStringList>
#include <QStringListModel>

class LYGithubProductBacklogStatusLog : public QObject
{
Q_OBJECT
public:
	/// Singleton accessor for the status log
	static LYGithubProductBacklogStatusLog* statusLog();
	/// Releases (deletes) the status log
	static void releaseStatusLog();

	/// Returns the QStringListModel
	QStringListModel* model();

public slots:
	/// Appends a new status message to the log
	void appendStatusMessage(const QString &statusMessage);

protected:
	/// Protected constructor for singleton design paradigm
	LYGithubProductBacklogStatusLog(QObject *parent = 0);

protected:
	/// Singleton instance
	static LYGithubProductBacklogStatusLog *instance_;

	/// The stringlist that has all of the status log messages (newest to oldest)
	QStringList statusLog_;
	/// The StringListModel for this log
	QStringListModel *model_;
};

#endif // LYGITHUBPRODUCTBACKLOGSTATUSLOG_H
