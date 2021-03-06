#include "LYGithubProductBacklogCentralWidget.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QTimer>
#include <QMessageBox>

LYGithubProductBacklogCentralWidget::LYGithubProductBacklogCentralWidget(const QString &username, const QString &repository, QWidget *parent) :
	QWidget(parent)
{
	productBacklog_ = new LYGithubProductBacklog();

	networkBusyView_ = 0;

	statusLogView_ = new LYGithubProductBacklogStatusLogView();
	statusLogView_->hide();

	treeView_ = new QTreeView();
	treeView_->setModel(productBacklog_->model());
	treeView_->setSelectionBehavior(QAbstractItemView::SelectItems);
	treeView_->setSelectionMode(QAbstractItemView::SingleSelection);
	treeView_->setAttribute(Qt::WA_MacShowFocusRect, false);
	treeView_->setDragEnabled(true);
	treeView_->viewport()->setAcceptDrops(true);
	treeView_->setDropIndicatorShown(true);
	treeView_->setDragDropMode(QTreeView::InternalMove);
	treeView_->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(treeView_, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onCustomContextMenuRequested(QPoint)));

	uploadChangesButton_ = new QPushButton("Upload Changes");
	uploadChangesButton_->setEnabled(false);

	closeIssueButton_ = new QPushButton("Close Issue");
	closeIssueButton_->setEnabled(false);

	addIssueButton_ = new QPushButton("Add Issue");

	QHBoxLayout *hl = new QHBoxLayout();
	hl->addWidget(addIssueButton_);
	hl->addWidget(closeIssueButton_);
	hl->addStretch(10);
	hl->addWidget(uploadChangesButton_);

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addLayout(hl);
	vl->addWidget(treeView_);

	setLayout(vl);

	connect(productBacklog_, SIGNAL(activeChanges(bool)), this, SLOT(onActiveChangesChanged(bool)));
	connect(uploadChangesButton_, SIGNAL(clicked()), this, SLOT(onUploadChangesButtonClicked()));
	connect(productBacklog_, SIGNAL(authenticated(bool)), this, SLOT(onAuthenticated(bool)));
	connect(productBacklog_, SIGNAL(uploaded(bool)), this, SLOT(onUploaded(bool)));

	connect(productBacklog_->productBacklogModel(), SIGNAL(modelAboutToBeRefreshed()), this, SLOT(onModelAboutToBeRefreshed()));
	connect(productBacklog_->productBacklogModel(), SIGNAL(modelRefreshed()), this, SLOT(onModelRefreshed()));

	connect(productBacklog_, SIGNAL(sanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks)), this, SLOT(onSanityCheckReturned(LYProductBacklogModel::ProductBacklogSanityChecks)));
	connect(productBacklog_, SIGNAL(networkRequestBusy(bool, QString)), this, SLOT(onNetworkRequestBusy(bool, QString)));

	connect(treeView_, SIGNAL(clicked(QModelIndex)), this, SLOT(onTreeViewIndexClicked(QModelIndex)));
	connect(addIssueButton_, SIGNAL(clicked()), this, SLOT(onAddIssueButtonClicked()));
	connect(closeIssueButton_, SIGNAL(clicked()), this, SLOT(onCloseIssueButtonClicked()));

	authenticationView_ = new LYGithubProductBacklogAuthenticationView(username, repository);
	connect(authenticationView_, SIGNAL(submitAuthenticationInformation(QString,QString,QString)), this, SLOT(onSubmitAuthenticationInformationAvailable(QString,QString,QString)));
	authenticationView_->show();
	authenticationView_->raise();
}

LYGithubProductBacklogCentralWidget::~LYGithubProductBacklogCentralWidget(){
	if(statusLogView_)
		delete statusLogView_;

	if(authenticationView_)
		delete authenticationView_;

	if(networkBusyView_)
		delete networkBusyView_;
}

void LYGithubProductBacklogCentralWidget::showStatusLog(){
	statusLogView_->show();
}

void LYGithubProductBacklogCentralWidget::ensureReadyForClose(){
	statusLogView_->hide();
}

void LYGithubProductBacklogCentralWidget::onSubmitAuthenticationInformationAvailable(const QString &username, const QString &password, const QString &repository){
	productBacklog_->setUserName(username);
	productBacklog_->setPassword(password);
	productBacklog_->setRepository(repository);
}

void LYGithubProductBacklogCentralWidget::onAuthenticated(bool authenticated){
	authenticationView_->setAuthenticated(authenticated);

	if(authenticated)
		emit requestStatusBarMessage("Successfully Authenticated", 3000);
	else
		emit requestStatusBarMessage("Failed to Authenticate", 3000);
}

void LYGithubProductBacklogCentralWidget::onUploadChangesButtonClicked(){
	productBacklog_->uploadChanges();
}

void LYGithubProductBacklogCentralWidget::onUploaded(bool successfullyUploaded){
	if(successfullyUploaded)
		emit requestStatusBarMessage("Successfully Uploaded Changes to Github", 5000);
	else
		emit requestStatusBarMessage("Failed to Upload Changes to Github", 5000);
}

void LYGithubProductBacklogCentralWidget::onActiveChangesChanged(bool hasActiveChanges){
	uploadChangesButton_->setEnabled(hasActiveChanges);
}

void LYGithubProductBacklogCentralWidget::onTreeViewIndexClicked(const QModelIndex &index){
	QString closeIssueString = QString("Close Issue #%1").arg(productBacklog_->productBacklogModel()->productBacklogItem(index)->issueNumber());
	closeIssueButton_->setText(closeIssueString);
	closeIssueButton_->setEnabled(true);
}

void LYGithubProductBacklogCentralWidget::onAddIssueButtonClicked(){
	LYGithubProductBacklogAddIssueView *addIssueView = new LYGithubProductBacklogAddIssueView();
	connect(addIssueView, SIGNAL(requestCreateNewIssue(QString,QString)), productBacklog_, SLOT(createNewIssue(QString,QString)));
	connect(productBacklog_, SIGNAL(newIssueCreated(bool)), addIssueView, SLOT(onGitIssueCreated(bool)));
	addIssueView->exec();
	//memory leak here?
}

void LYGithubProductBacklogCentralWidget::onCloseIssueButtonClicked(){
	if(!productBacklog_->productBacklogModel()->hasChildren(treeView_->currentIndex()))
		productBacklog_->closeIssue(productBacklog_->productBacklogModel()->productBacklogItem(treeView_->currentIndex())->issueNumber());
	else
		QMessageBox::warning(this, "Cannot Close This Issue", "This issue has open children, it cannot be closed", QMessageBox::Ok);
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

void LYGithubProductBacklogCentralWidget::onNetworkRequestBusy(bool isBusy, const QString &busyText){
	setEnabled(!isBusy);
	if(isBusy && !networkBusyView_){
		networkBusyView_ = new LYGithubProductBacklogNetworkBusyView(busyText);
		networkBusyView_->show();
	}
	else if(!isBusy && networkBusyView_){
		networkBusyView_->close();
		networkBusyView_->deleteLater();
		networkBusyView_ = 0;
	}
}

#include <QMenu>
void LYGithubProductBacklogCentralWidget::onCustomContextMenuRequested(const QPoint &point){
	Q_UNUSED(point)
	QMenu *menu = new QMenu;
	QModelIndex index = treeView_->currentIndex();

	if(productBacklog_->productBacklogModel()->productBacklogItem(index)->isSelected())
		menu->addAction("Set Not Selected", this, SLOT(onCustomContextMenuRequestToggle()));
	else
		menu->addAction("Set Selected", this, SLOT(onCustomContextMenuRequestToggle()));

	menu->exec(QCursor::pos());
}

void LYGithubProductBacklogCentralWidget::onCustomContextMenuRequestToggle(){
	QModelIndex index = treeView_->currentIndex();

	productBacklog_->productBacklogModel()->toggleIsSelectedOnIndex(index);
}

void LYGithubProductBacklogCentralWidget::onModelAboutToBeRefreshed(){
	expandedIndexList_ = expandedIndices();
}

void LYGithubProductBacklogCentralWidget::onModelRefreshed(){
	for(int x = 0; x < expandedIndexList_.count(); x++)
		treeView_->setExpanded(expandedIndexList_.at(x), true);
}

QModelIndexList LYGithubProductBacklogCentralWidget::expandedIndices(const QModelIndex parent){
	QModelIndexList retVal;

	for(int x = 0; x < productBacklog_->model()->rowCount(parent); x++){
		if(treeView_->isExpanded(productBacklog_->model()->index(x, 0, parent))){
			retVal.append(productBacklog_->model()->index(x, 0, parent));
			retVal.append(expandedIndices(productBacklog_->model()->index(x, 0, parent)));
		}
	}

	return retVal;
}

LYGithubProductBacklogAuthenticationView::LYGithubProductBacklogAuthenticationView(const QString &username, const QString &repository, QWidget *parent) :
	QDialog(parent)
{
	QFormLayout *fl = new QFormLayout();

	usernameLineEdit_ = new QLineEdit(username);
	passwordLineEdit_ = new QLineEdit();
	passwordLineEdit_->setEchoMode(QLineEdit::Password);
	repositoryLineEdit_ = new QLineEdit(repository);

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
	closedIssuesWithChildrenLayout->addWidget(new QLabel("This is bad, we don't have a automated fix for this right now"));
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
	if(sanityCheck.testFlag(LYProductBacklogModel::SanityCheckFailedFalseOrderedIssueWithChildren)){
		masterVL->addWidget(closedIssuesWithChildrenGroupBox);
		fixClosedIssuesWithChildrenCheckBox_->setChecked(false);
		fixClosedIssuesWithChildrenCheckBox_->setEnabled(false);
		fixButton_->setEnabled(false);
	}
	masterVL->addLayout(buttonsHL);
	setLayout(masterVL);
}

void LYGithubProductBacklogSanityCheckView::onCheckBoxToggled(){
	if(fixMissingIssuesCheckBox_->isChecked() && fixClosedIssuesWithoutChildrenCheckBox_->isChecked() && fixClosedIssuesWithChildrenCheckBox_->isChecked())
		fixButton_->setEnabled(true);
	else
		fixButton_->setEnabled(false);
}

LYGithubProductBacklogAddIssueView::LYGithubProductBacklogAddIssueView(QWidget *parent)
	: QDialog(parent)
{
	issueCreatedSuccessfully_ = false;
	exitCountDownTimer_ = 0;

	issueTitleEdit_ = new QLineEdit();

	issueBodyEdit_ = new QTextEdit();

	submitIssuesButton_ = new QPushButton(QIcon(":/22x22/greenCheck.png"), "Submit");
	submitIssuesButton_->setEnabled(false);

	cancelButton_ = new QPushButton(QIcon(":/22x22/list-remove-2.png"), "Cancel");

	waitingBar_ = new QProgressBar();
	waitingBar_->setMinimum(0);
	waitingBar_->setMaximum(0);
	waitingBar_->setMinimumWidth(200);
	waitingBar_->hide();

	messageLabel_ = new QLabel();

	QHBoxLayout *messageVL = new QHBoxLayout();
	messageVL->addWidget(messageLabel_, 0, Qt::AlignCenter);
	messageVL->addWidget(waitingBar_, 0, Qt::AlignCenter);

	QVBoxLayout *fl = new QVBoxLayout();
	fl->addWidget(new QLabel("Title"), 0, Qt::AlignLeft);
	fl->addWidget(issueTitleEdit_);
	fl->addWidget(new QLabel("Description"), 0, Qt::AlignLeft);
	fl->addWidget(issueBodyEdit_);

	QHBoxLayout *hl = new QHBoxLayout();
	hl->addStretch();
	hl->addWidget(submitIssuesButton_);
	hl->addWidget(cancelButton_);

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addLayout(fl);
	vl->addLayout(messageVL);
	vl->addLayout(hl);

	setLayout(vl);

	connect(cancelButton_, SIGNAL(clicked()), this, SLOT(onCancelButtonClicked()));
	connect(submitIssuesButton_, SIGNAL(clicked()), this, SLOT(onSubmitIssueButtonClicked()));

	connect(issueTitleEdit_, SIGNAL(textEdited(QString)), this, SLOT(onEditsChanged()));
	connect(issueBodyEdit_, SIGNAL(textChanged()), this, SLOT(onEditsChanged()));
}

void LYGithubProductBacklogAddIssueView::onCancelButtonClicked()
{
	hideAndFinish();
}

void LYGithubProductBacklogAddIssueView::onSubmitIssueButtonClicked()
{
	waitingBar_->show();
	messageLabel_->show();
	submitIssuesButton_->setEnabled(false);

	messageLabel_->setText("Submitting Issue...");
	emit requestCreateNewIssue(issueTitleEdit_->text(), issueBodyEdit_->document()->toPlainText());
}

void LYGithubProductBacklogAddIssueView::onGitIssueCreated(bool issueCreated)
{
	if(issueCreated){

		issueCreatedSuccessfully_ = true;
		waitingBar_->setMaximum(1);
		waitingBar_->setValue(1);
		messageLabel_->setText("Issue Submitted");

		issueTitleEdit_->setEnabled(false);
		issueBodyEdit_->setEnabled(false);
		submitIssuesButton_->setEnabled(false);
		cancelButton_->setEnabled(false);

		exitCountDownCounter_ = 0;
		exitCountDownTimer_ = new QTimer(this);
		connect(exitCountDownTimer_, SIGNAL(timeout()), this, SLOT(onExitCountDownTimeout()));
		exitCountDownTimer_->start(1000);
		onExitCountDownTimeout();
	}
	else{

		waitingBar_->show();
		messageLabel_->show();
		messageLabel_->setText("Could not create issue");
	}
}

void LYGithubProductBacklogAddIssueView::onEditsChanged()
{
	if(!issueTitleEdit_->text().isEmpty() && !issueBodyEdit_->document()->toPlainText().isEmpty() && !issueCreatedSuccessfully_)
		submitIssuesButton_->setEnabled(true);
	else
		submitIssuesButton_->setEnabled(false);
}

void LYGithubProductBacklogAddIssueView::onExitCountDownTimeout()
{
	if(exitCountDownCounter_ == 3){

		hideAndFinish();
		return;
	}

	QString goodbyeMessage = QString("New Issue Created\n(Closing this window in %1 seconds)").arg(3-exitCountDownCounter_);
	issueBodyEdit_->setText(goodbyeMessage);
	exitCountDownCounter_++;
}

void LYGithubProductBacklogAddIssueView::hideAndFinish()
{
	hide();
	emit finished();
}

#include <QCloseEvent>
void LYGithubProductBacklogAddIssueView::closeEvent(QCloseEvent *e)
{
	e->accept();
	hideAndFinish();
}

LYGithubProductBacklogNetworkBusyView::LYGithubProductBacklogNetworkBusyView(const QString &interactionText, QWidget *parent) :
	QDialog(parent)
{
	serverInteractionProgressBar_ = new QProgressBar();
	serverInteractionProgressBar_->setMinimumWidth(200);

	serverInteractionLabel_ = new QLabel(interactionText);

	QVBoxLayout *vl_ = new QVBoxLayout();
	vl_->addWidget(serverInteractionProgressBar_);
	vl_->addWidget(serverInteractionLabel_);

	setLayout(vl_);
	setWindowModality(Qt::WindowModal);

	serverInteractionProgressBar_->setMinimum(0);
	serverInteractionProgressBar_->setMaximum(0);
}

LYGithubProductBacklogStatusLogView::LYGithubProductBacklogStatusLogView(QWidget *parent) :
	QWidget(parent)
{
	statusLogListView_ = new QListView();
	statusLogListView_->setModel(LYGithubProductBacklogStatusLog::statusLog()->model());
	statusLogListView_->setAlternatingRowColors(true);
	statusLogListView_->setEditTriggers(QAbstractItemView::NoEditTriggers);

	QVBoxLayout *vl = new QVBoxLayout();
	vl->addWidget(statusLogListView_);
	setLayout(vl);
}
