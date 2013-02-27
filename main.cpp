#include <QtGui/QApplication>
#include "LYGithubProductBacklogMainWindow.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	if(a.arguments().count() == 3){
		LYGithubProductBacklogMainWindow w(a.arguments().at(1), a.arguments().at(2));
		w.show();
		return a.exec();
	}
	else{
		LYGithubProductBacklogMainWindow w;
		w.show();
		return a.exec();
	}
}
