#ifndef LYGITHUBMANAGER_H
#define LYGITHUBMANAGER_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QSslError>
#include <QBuffer>

#include "qjson/serializer.h"
#include "qjson/parser.h"

class LYGithubManager : public QObject
{
	Q_OBJECT

public:
	/// Enum for the issues filter type.
	enum IssuesFilter { IssuesFilterAssigned, IssuesFilterCreated, IssuesFilterMentioned, IssuesFilterSubscribed, IssuesFilterAll};
	/// Enum for the issues state filter type.
	enum IssuesState { IssuesStateOpen, IssuesStateClosed};
	/// Enum for the issues sort type filter.
	enum IssuesSort{ IssuesSortCreated, IssuesSortUpdated, IssuesSortComments};
	/// Enum for the issues sorting direction filter.
	enum IssuesDirection{ IssuesDirectionAscending, IssuesDirectionDescending};

	/// Constructor.  Builds a default Github manager.
	LYGithubManager(QObject *parent = 0);
	/// Constructor.  Builds a default Github manager and attempts to connect to Github based on the given username, password, and respository.
	LYGithubManager(const QString &userName, const QString &password, const QString &repository, QObject *parent = 0);

	/// Returns the current user name.
	QString userName() const;
	/// Returns whether we are currently authenticated with the Github server.
	bool isAuthenticated() const;
	/// Returns which repository we are connecting to.
	QString repository() const;
	/// Returns a string in a readable format based on the returned values when requesting the issues from the respository.
	QString jsonSensiblePrint(const QVariantMap &jsonMap, int indentLevel = 0) const;

public slots:
	/// Sets the user name used to connect to the repository.
	void setUserName(const QString &userName);
	/// Sets the password used to authenticate to the repository.
	void setPassword(const QString &password);
	/// Attempts to authenticate with Github with the provided user name and password.
	void authenticate();
	/// Sets which repository this should be connected to.
	void setRepository(const QString &repository);
	/// Does the work to get the issues from the connected Github repository based on the \param filter, \param state, \param sort, and \param direction flags.
	void getIssues(LYGithubManager::IssuesFilter filter = LYGithubManager::IssuesFilterAssigned,
			   LYGithubManager::IssuesState state = LYGithubManager::IssuesStateOpen,
			   LYGithubManager::IssuesSort sort = LYGithubManager::IssuesSortCreated,
			   LYGithubManager::IssuesDirection direction = LYGithubManager::IssuesDirectionAscending);
	/// Does the work to get all of the comments on a single issue (by id) from the connected Github repository.
	void getSingleIssueComments(int issueNumber);
	/// Does the work to get a single comment (by id) in the connected Github repository.
	void getSingleComment(int commentId);
	/// Does the work to edit a single comment (by id) in the connected Github repository.
	void editSingleComment(int commentId, const QString &newComment);
	/// Slot that creates a new issue with the given \param title, \param body, and, optionally, an \param assignee.
	void createNewIssue(const QString &title, const QString &body, const QString &assignee = QString());
	/// Slot that closes an issue with the given \param issue number
	void closeIssue(int issueNumber);

signals:
	/// Notifier that indicates whether we successfully authenticated with Github or not.
	void authenticated(bool isAuthenticated);
	/// Notifier that contains all issues when they have been requested.
	void issuesReturned(QList<QVariantMap> issues);
	/// Notifier that contains the comments requested
	void singleIssueCommentReturned(QVariantMap comments);
	/// Notifier that contains the comment requested
	void singleCommentReturned(QVariantMap comment);
	/// Notifier that contains the new values for the comment
	void singleCommentEdited(QVariantMap comment);
	/// Notifier whether or not an issue was successfully created.
	void issueCreated(bool issueCreated, QVariantMap newIssue);
	/// Notifier whether or not an issue was successfully closed.
	void issueClosed(bool issueClosed, QVariantMap closedIssue);

protected slots:
	/// Slot handling the authentication response.
	void onAuthenicatedRequestReturned();
	/// Slot handling the issues request response.
	void onIssuesReturned();
	/// Slot handling the single issue's comments request response.
	void onSingleIssueCommentsReturned();
	/// Slot handling the get single comment request response.
	void onGetSingleCommentReturned();
	/// Slot handling the edit single comment request response.
	void onEditSingleCommentReturned();
	/// Slot handling the response when creating a new issue.
	void onCreateNewIssueReturned();
	/// Slot handling the reponse when closing an issue
	void onCloseIssueReturned();

	void onSomeErrorOccured(QNetworkReply::NetworkError nError);

protected:
	/// Helper method that intializes all of the classes member variables.
	void initialize();

protected:
	/// Pointer that handles the network access.
	QNetworkAccessManager *manager_;
	/// Pointer specifically focusing on the authenticatation network reply.
	QNetworkReply *authenticateReply_;

	/// Pointer specifically focusing on the get issues network reply.
	QNetworkReply *getIssuesReply_;
	/// Pointer specifically focusing on the get single issue's comments network reply.
	QNetworkReply *getSingleIssueCommentsReply_;

	/// Pointer specifically focusing on the get a single comment reply.
	QNetworkReply *getSingleCommentReply_;
	/// Pointer specifically focusing on the edit a single comment reply.
	QNetworkReply *editSingleCommentReply_;

	/// Pointer specifically focusing on the create new issue network reply.
	QNetworkReply *createNewIssueReply_;
	/// Pointer specifically focusing on the closing of an existing issue  network reply.
	QNetworkReply *closeIssueReply_;

	/// Holds the user name used for connecting to Github.
	QString userName_;
	/// Holds the password that goes with the user name.
	QString password_;
	/// Flag holding whether or not this manager has successfully been authenticated.
	bool authenticated_;
	/// Holds the name of the repository of interest.
	QString repository_;
};

/// Declares the ENUMS as availabe to the QMetaObject system
Q_DECLARE_METATYPE(LYGithubManager::IssuesFilter)
Q_DECLARE_METATYPE(LYGithubManager::IssuesState)
Q_DECLARE_METATYPE(LYGithubManager::IssuesSort)
Q_DECLARE_METATYPE(LYGithubManager::IssuesDirection)

#endif // LYGITHUBMANAGER_H
