#include <QtCore>
#include <QApplication>

#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICNSPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
Q_IMPORT_PLUGIN(QTgaPlugin)
Q_IMPORT_PLUGIN(QTiffPlugin)
Q_IMPORT_PLUGIN(QWbmpPlugin)
Q_IMPORT_PLUGIN(QWebpPlugin)
// Q_IMPORT_PLUGIN(QPSQLDriverPlugin)

#include "qindicatorwindow.h"
#include "qexceptiondialog.h"

class MyApplication : public QApplication {
public:
  MyApplication(int& argc, char ** argv) :
    QApplication(argc, argv) { }
  virtual ~MyApplication() { }

    // reimplemented from QApplication so we can throw exceptions in slots
    virtual bool notify(QObject * receiver, QEvent * event) {
        try {
            return QApplication::notify(receiver, event);
        } catch(std::exception& e) {
            // ExceptionDialog is currently modal
            static bool bInProgress = false;
            if (!bInProgress) {
                 bInProgress = true;
                 showExceptionDialog(QString("Exception thrown: ") + e.what());
                 bInProgress = false;
            }
            // no need to propagate event on exception
            return true;
        }
        // propagate the event
        return false;
    }

};

int main(int argc, char *argv[]) {
	MyApplication a(argc, argv);
    QIndicatorWindow w;
    w.showStopper();
    w.initComponents();
    return a.exec();
}
