#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
    
  QCoreApplication::setOrganizationName("MillRunTech");
  QCoreApplication::setOrganizationDomain("MillRunTech.com");
  QCoreApplication::setApplicationName("Koha Offline Circulation");
  MainWindow mainWindow;
  mainWindow.show();
                        
  return app.exec();
}