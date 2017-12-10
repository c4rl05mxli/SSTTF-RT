#include "dialog.h"
#include <QApplication>

QString compilationTime = QString("%1 %2").arg(__DATE__).arg(__TIME__);


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    QString title="SSTTF-TR";
    title.append(" Fecha de Compilación:  ");
    title.append(compilationTime);
    w.setFixedSize(800,480);
    w.setWindowTitle(title);
   //w.setStyleSheet("background-color:blue;");
   //w.setModal(true);
   //w.setWindowFlags(Qt::FramelessWindowHint);
    w.show();
    return a.exec();
}
