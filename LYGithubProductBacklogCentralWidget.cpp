#include "LYGithubProductBacklogCentralWidget.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QTimer>
#include <QMessageBox>

LYGithubProductBacklogAuthenticationView::LYGithubProductBacklogAuthenticationView(QWidget *parent) :
	QDialog(parent)
{
	QFormLayout *fl = new QFormLayout();

	usernameLineEdit_ = new QLineEdit("i-am-l");
	passwordLineEdit_ = new QLineEdit();
	passwordLineEdit_->setEchoMode(QLineEdit::Password);
	repositoryLineEdit_ = new QLineEdit("i-am-l/GithubProductBacklog");

	fl->addRow("Username:", usernameLineEdit_);
	fl->addRow("Password:", passwordLineEdit_);
	fl->addRow("Repository:", repositoryLineEdit_);

	submitButton_ = new QPushButton("Submit");
	submitButton_->setDefault(true);

	serverInteractionProgressBar_ = new QProgressBar();
	serverInteractionProgressBar_->setMinimumWidth(200);

	serverInteractionLabel_ = new QLabel();

	QVBoxLayout *vl_ = new QVBoxLayout();
	vl_->addLayout(fl);
	vl_->addWidget(submitButton_);
	vl_->addWidget(serverInteractionProgressBar_);
	vl_->addWidget(serverInteractionLabel_);

	setLayout(vl_);
	setWindowModality(Qt::WindowModal);

	connect(usernameLineEdit_, SIGNAL(textChanged(QString)), this, SLOT(onLineEditsEditted()));
	connect(passwordLineEdit_, SIGNAL(textChanged(QString)), this, SLOT(onLineEditsEditted()));
	connect(repositoryLineEdit_, SIGNAL(textChanged(QString)), this, SLOT(onLineEditsEditted()));

	connect(submitButton_, SIGNAL(clicked()), this, SLOT(onSubmitButtonClicked()));

	prepareServerInteractionWidgets();
	onLineEditsEditted();
	usernameLineEdit_->setFocus();
}

void LYGithubProductBacklogAuthenticationView::setAuthenticated(bool authenticated){
	serverInteractionProgressBar_->setMaximum(1);
	serverInteractionProgressBar_->setValue(1);
	if(authenticated){
		serverInteractionLabel_->setText("Authentication Successful");
		QTimer::singleShot(1000, this, SLOT(close()));
	}
	else{
		serverInteractionLabel_->setText("Authentication Failed");
		passwordLineEdit_->clear();
		submitButton_->show();
		QTimer::singleShot(1000, this, SLOT(prepareServerInteractionWidgets()));
	}
}

void LYGithubProductBacklogAuthenticationView::onLineEditsEditted(){
	if( usernameLineEdit_->text().isEmpty() || passwordLineEdit_->text().isEmpty() || repositoryLineEdit_->text().isEmpty() )
		submitButton_->setEnabled(false);
	else
		submitButton_->setEnabled(true);
}

void LYGithubProductBacklogAuthenticationView::onSubmitButtonClicked(){
	submitButton_->setEnabled(false);
	serverInteractionLabel_->setText("Authenticating ...");
	serverInteractionProgressBar_->show();
	serverInteractionLabel_->show();
	submitButton_->hide();
	emit submitAuthenticationInformation(usernameLineEdit_->text(), passwordLineEdit_->text(), repositoryLineEdit_->text());
}

void LYGithubProductBacklogAuthenticationView::prepareServerInteractionWidgets(){
	serverInteractionProgressBar_->setMinimum(0);
	serverInteractionProgressBar_->setMaximum(0);

	serverInteractionProgressBar_->hide();
	serverInteractionLabel_->hide();
}

LYGithubProductBacklogCentralWidget::LYGithubProductBacklogCentralWidget(QWidget *parent) :
	QWidget(parent)
{
	productBacklog_ = new LYGithubProductBacklog();

	treeView_ = new QTreeView();
	treeView_->setModel(productBacklog_->model());
	treeView_->setSelectionBehavior(QAbstractItemView::SelectItems);
	treeView_->setSelectionMode(QAbstractItemView::SingleSelection);
	treeView_->setDragEnabled(true);
	treeView_->viewport()->setAcceptDrops(true);
	treeView_->setDropIndicatorShown(true);
	treeView_->setDragDropMode(QTreeView::InternalMove);

	uploadChangesButton_ = new QPushButton("Upload Changes");
	uploadChangesButton_->setEnabled(false);

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addWidget(uploadChangesButton_);
	vl->addWidget(treeView_);

	setLayout(vl);

	connect(productBacklog_, SIGNAL(activeChanges(bool)), this, SLOT(onActiveChangesChanged(bool)));
	connect(uploadChangesButton_, SIGNAL(clicked()), this, SLOT(onUploadChangesButtonClicked()));
	connect(productBacklog_, SIGNAL(authenticated(bool)), this, SLOT(onAuthenticated(bool)));

	connect(productBacklog_, SIGNAL(detectedMissingIssues(QList<int>)), this, SLOT(onDetectedMissingIssues(QList<int>)));
	connect(productBacklog_, SIGNAL(detectedClosedIssuesWithoutChildren(QList<int>)), this, SLOT(onDetectedClosedIssuesWithoutChildren(QList<int>)));
	connect(productBacklog_, SIGNAL(detectedClosedIssuesWithChildren(QList<int>)), this, SLOT(onDetectedClosedIssuesWithChildren(QList<int>)));

	authenticationView_ = new LYGithubProductBacklogAuthenticationView();
	connect(authenticationView_, SIGNAL(submitAuthenticationInformation(QString,QString,QString)), this, SLOT(onSubmitAuthenticationInformationAvailable(QString,QString,QString)));
	authenticationView_->show();
	authenticationView_->raise();
}

void LYGithubProductBacklogCentralWidget::onSubmitAuthenticationInformationAvailable(const QString &username, const QString &password, const QString &repository){
	productBacklog_->setUserName(username);
	productBacklog_->setPassword(password);
	productBacklog_->setRepository(repository);
}

void LYGithubProductBacklogCentralWidget::onAuthenticated(bool authenticated){
	authenticationView_->setAuthenticated(authenticated);
}

void LYGithubProductBacklogCentralWidget::onUploadChangesButtonClicked(){
	productBacklog_->uploadChanges();
}

void LYGithubProductBacklogCentralWidget::onActiveChangesChanged(bool hasActiveChanges){
	uploadChangesButton_->setEnabled(hasActiveChanges);
}

void LYGithubProductBacklogCentralWidget::onDetectedMissingIssues(QList<int> missingIssuesNumbers){
	QMessageBox messageBox;

	QString issuesString;
	for(int x = 0; x < missingIssuesNumbers.count(); x++)
		issuesString.append(QString("%1 ").arg(missingIssuesNumbers.at(x)));
	issuesString.prepend("( ");
	issuesString.append(")");
	messageBox.setText("Unordered Issues Detected");
	messageBox.setInformativeText(QString("Some of the issues in the repository have not been ordered:\n%1\nShould these issues simply be appended to the ordering list?\nAlternatively, you can abort and the program will close without making any changes.").arg(issuesString));
	messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Abort);
	messageBox.setDefaultButton(QMessageBox::Ok);
	int retVal = messageBox.exec();
	switch(retVal){
	case QMessageBox::Ok:
		productBacklog_->acceptAppendMissingIssues();
		break;
	case QMessageBox::Abort:
		emit requestQuit();
		break;
	}
}

void LYGithubProductBacklogCentralWidget::onDetectedClosedIssuesWithoutChildren(QList<int> closedIssuesWithoutChildren){

}

void LYGithubProductBacklogCentralWidget::onDetectedClosedIssuesWithChildren(QList<int> closedIssuesWithChildren){

}
