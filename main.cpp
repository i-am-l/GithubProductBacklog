#include <QtGui/QApplication>
#include "LYGithubProductBacklogMainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LYGithubProductBacklogMainWindow w;
    w.show();

    return a.exec();
}
