#include "LYGithubProductBacklogCentralWidget.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QTimer>
#include <QMessageBox>

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

	connect(productBacklog_, SIGNAL(sanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks)), this, SLOT(onSanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks)));

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

void LYGithubProductBacklogCentralWidget::onSanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck){
	QStringList missingIssues;
	QStringList orderedIssuesWithoutChildren;
	QStringList orderedIssuesWithChildren;
	if(sanityCheck.testFlag(LYProductBacklogModel::SanityCheckFailedMissingIssue))
		missingIssues = productBacklog_->missingIssues();
	if(sanityCheck.testFlag(LYProductBacklogModel::SanityCheckFailedFalseOrderedIssueNoChildren))
		orderedIssuesWithoutChildren = productBacklog_->orderedIssuesWithoutChildren();
	if(sanityCheck.testFlag(LYProductBacklogModel::SanityCheckFailedFalseOrderedIssueWithChildren))
		orderedIssuesWithChildren = productBacklog_->orderedIssuesWithChildren();

	if(!sanityCheck.testFlag(LYProductBacklogModel::SanityCheckPassed)){
		LYGithubProductBacklogSanityCheckView *sanityCheckView = new LYGithubProductBacklogSanityCheckView(sanityCheck, productBacklog_->missingIssues(), productBacklog_->orderedIssuesWithoutChildren(), productBacklog_->orderedIssuesWithChildren());
		int retVal = sanityCheckView->exec();

		switch(retVal){
		case QDialog::Accepted:
			productBacklog_->fixStartupIssues();
			break;
		case QDialog::Rejected:
			emit requestQuit();
			break;
		}
	}
}

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

LYGithubProductBacklogSanityCheckView::LYGithubProductBacklogSanityCheckView(LYProductBacklogModel::ProductBacklogSanityChecks sanityCheck, QStringList missingIssues, QStringList closedIssuesWithoutChildren, QStringList closedIssuesWithChildren, QWidget *parent) :
	QDialog(parent)
{
	missingIssuesListView_ = new QListView();
	closedIssuesWithoutChildrenListView_ = new QListView();
	closedIssuesWithChildrenListView_ = new QListView();

	QStringListModel *missingIssuesModel = new QStringListModel(missingIssues);
	QStringListModel *closedIssuesWithoutChildrenModel = new QStringListModel(closedIssuesWithoutChildren);
	QStringListModel *closedIssuesWithChildrenModel = new QStringListModel(closedIssuesWithChildren);

	missingIssuesListView_->setModel(missingIssuesModel);
	closedIssuesWithoutChildrenListView_->setModel(closedIssuesWithoutChildrenModel);
	closedIssuesWithChildrenListView_->setModel(closedIssuesWithChildrenModel);

	QGroupBox *missingIssuesGroupBox = new QGroupBox("Some Open Issues are Missing");
	QVBoxLayout *missingIssuesLayout = new QVBoxLayout();
	fixMissingIssuesCheckBox_ = new QCheckBox("Append missing issues to the back of the Product Backlog");
	fixMissingIssuesCheckBox_->setChecked(true);
	missingIssuesLayout->addWidget(fixMissingIssuesCheckBox_);
	missingIssuesLayout->addWidget(missingIssuesListView_);
	missingIssuesGroupBox->setLayout(missingIssuesLayout);

	QGroupBox *closedIssuesWithoutChildrenGroupBox = new QGroupBox("Some Closed or Unknown Issues are Present");
	QVBoxLayout *closedIssuesWithoutChildrenLayout = new QVBoxLayout();
	fixClosedIssuesWithoutChildrenCheckBox_ = new QCheckBox("Remove these issues from the Product Backlog");
	fixClosedIssuesWithoutChildrenCheckBox_->setChecked(true);
	closedIssuesWithoutChildrenLayout->addWidget(fixClosedIssuesWithoutChildrenCheckBox_);
	closedIssuesWithoutChildrenLayout->addWidget(closedIssuesWithoutChildrenListView_);
	closedIssuesWithoutChildrenGroupBox->setLayout(closedIssuesWithoutChildrenLayout);

	QGroupBox *closedIssuesWithChildrenGroupBox = new QGroupBox("Some Closed or Unknown Issues are Present and They Supposedly Have Children");
	QVBoxLayout *closedIssuesWithChildrenLayout = new QVBoxLayout();
	fixClosedIssuesWithChildrenCheckBox_ = new QCheckBox("Reopen these Issues Because their Children are still Open");
	fixClosedIssuesWithChildrenCheckBox_->setChecked(true);
	closedIssuesWithChildrenLayout->addWidget(fixClosedIssuesWithChildrenCheckBox_);
	closedIssuesWithChildrenLayout->addWidget(closedIssuesWithChildrenListView_);
	closedIssuesWithChildrenGroupBox->setLayout(closedIssuesWithChildrenLayout);

	connect(fixMissingIssuesCheckBox_, SIGNAL(toggled(bool)), this, SLOT(onCheckBoxToggled()));
	connect(fixClosedIssuesWithoutChildrenCheckBox_, SIGNAL(toggled(bool)), this, SLOT(onCheckBoxToggled()));
	connect(fixClosedIssuesWithChildrenCheckBox_, SIGNAL(toggled(bool)), this, SLOT(onCheckBoxToggled()));

	fixButton_ = new QPushButton("Fix");
	quitButton_ = new QPushButton("Quit");
	fixButton_->setDefault(true);
	QHBoxLayout *buttonsHL = new QHBoxLayout();
	buttonsHL->addStretch(10);
	buttonsHL->addWidget(quitButton_);
	buttonsHL->addWidget(fixButton_);

	connect(fixButton_, SIGNAL(clicked()), this, SLOT(accept()));
	connect(quitButton_, SIGNAL(clicked()), this, SLOT(reject()));

	QVBoxLayout *masterVL = new QVBoxLayout();
	if(sanityCheck.testFlag(LYProductBacklogModel::SanityCheckFailedMissingIssue))
		masterVL->addWidget(missingIssuesGroupBox);
	if(sanityCheck.testFlag(LYProductBacklogModel::SanityCheckFailedFalseOrderedIssueNoChildren))
		masterVL->addWidget(closedIssuesWithoutChildrenGroupBox);
	if(sanityCheck.testFlag(LYProductBacklogModel::SanityCheckFailedFalseOrderedIssueWithChildren))
		masterVL->addWidget(closedIssuesWithChildrenGroupBox);
	masterVL->addLayout(buttonsHL);
	setLayout(masterVL);
}

void LYGithubProductBacklogSanityCheckView::onCheckBoxToggled(){
	if(fixMissingIssuesCheckBox_->isChecked() && fixClosedIssuesWithoutChildrenCheckBox_->isChecked() && fixClosedIssuesWithChildrenCheckBox_->isChecked())
		fixButton_->setEnabled(true);
	else
		fixButton_->setEnabled(false);
}
