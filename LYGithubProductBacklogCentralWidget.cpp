#include "LYGithubProductBacklogCentralWidget.h"

#include <QVBoxLayout>
#include <QFormLayout>

LYGithubProductBacklogAuthenticationView::LYGithubProductBacklogAuthenticationView(QWidget *parent) :
	QWidget(parent)
{
	QFormLayout *fl = new QFormLayout();

	usernameLineEdit_ = new QLineEdit("");
	passwordLineEdit_ = new QLineEdit();
	passwordLineEdit_->setEchoMode(QLineEdit::Password);
	repositoryLineEdit_ = new QLineEdit("");

	fl->addRow("Username:", usernameLineEdit_);
	fl->addRow("Password:", passwordLineEdit_);
	fl->addRow("Repository:", repositoryLineEdit_);

	submitButton_ = new QPushButton("Submit");

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addLayout(fl);
	vl->addWidget(submitButton_);

	setLayout(vl);
	setWindowModality(Qt::WindowModal);

	connect(usernameLineEdit_, SIGNAL(textChanged(QString)), this, SLOT(onLineEditsEditted()));
	connect(passwordLineEdit_, SIGNAL(textChanged(QString)), this, SLOT(onLineEditsEditted()));
	connect(repositoryLineEdit_, SIGNAL(textChanged(QString)), this, SLOT(onLineEditsEditted()));

	connect(submitButton_, SIGNAL(clicked()), this, SLOT(onSubmitButtonClicked()));

	onLineEditsEditted();
}

void LYGithubProductBacklogAuthenticationView::onLineEditsEditted(){
	if( usernameLineEdit_->text().isEmpty() || passwordLineEdit_->text().isEmpty() || repositoryLineEdit_->text().isEmpty() )
		submitButton_->setEnabled(false);
	else
		submitButton_->setEnabled(true);
}

void LYGithubProductBacklogAuthenticationView::onSubmitButtonClicked(){
	emit submitAuthenticationInformation(usernameLineEdit_->text(), passwordLineEdit_->text(), repositoryLineEdit_->text());
}

LYGithubProductBacklogCentralWidget::LYGithubProductBacklogCentralWidget(QWidget *parent) :
	QWidget(parent)
{
	productBacklog_ = new LYGithubProductBacklog();

	listView_ = new QListView(this);
	listView_->setModel(productBacklog_->model());
	listView_->setSelectionMode(QAbstractItemView::SingleSelection);
	listView_->setDragEnabled(true);
	listView_->viewport()->setAcceptDrops(true);
	listView_->setDropIndicatorShown(true);
	listView_->setDragDropMode(QListView::InternalMove);

	uploadChangesButton_ = new QPushButton("Upload Changes");
	uploadChangesButton_->setEnabled(false);

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addWidget(uploadChangesButton_);
	vl->addWidget(listView_);

	setLayout(vl);

	connect(productBacklog_, SIGNAL(activeChanges(bool)), this, SLOT(onActiveChangesChanged(bool)));
	connect(uploadChangesButton_, SIGNAL(clicked()), this, SLOT(onUploadChangesButtonClicked()));

	authenticationView_ = new LYGithubProductBacklogAuthenticationView();
	connect(authenticationView_, SIGNAL(submitAuthenticationInformation(QString,QString,QString)), this, SLOT(onSubmitAuthenticationInformationAvailable(QString,QString,QString)));
	authenticationView_->show();
	authenticationView_->raise();
}

void LYGithubProductBacklogCentralWidget::onSubmitAuthenticationInformationAvailable(const QString &username, const QString &password, const QString &repository){
	authenticationView_->hide();
	productBacklog_->setUserName(username);
	productBacklog_->setPassword(password);
	productBacklog_->setRepository(repository);
}

void LYGithubProductBacklogCentralWidget::onUploadChangesButtonClicked(){
	productBacklog_->uploadChanges();
}

void LYGithubProductBacklogCentralWidget::onActiveChangesChanged(bool hasActiveChanges){
	uploadChangesButton_->setEnabled(hasActiveChanges);
}

