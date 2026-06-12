#include "ckernel.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CKernel::Get_Instance();
    return a.exec();
}
