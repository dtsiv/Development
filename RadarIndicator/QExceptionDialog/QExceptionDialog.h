#ifndef QEXCEPTIONDIALOG_H
#define QEXCEPTIONDIALOG_H

#include <QtCore>
#include <QDialog>
#include <exception>

#ifndef __GNUC__
#define _GLIBCXX_USE_NOEXCEPT
#endif

class QByteArray;
class QString;

//-----------------------------------------------------------------------
// To use exceptions:
// 1. Throw object rvalue, not pointer: throw QRmoConnectionException("server has accepted new connection!");
// 2. Question: Do we therefore need copy-contructor?
// 3. Need to reimplement QApplication::notify() in end-user application. See main.cpp in SpeedDialog project
//-----------------------------------------------------------------------
class RmoException : public std::exception {
public:
	RmoException(QString str) _GLIBCXX_USE_NOEXCEPT; 
	// copy constructor
	// RmoException(const RmoException& _Right) _GLIBCXX_USE_NOEXCEPT;
    // assignment operator
	// RmoException& operator=(const RmoException& _Right) _GLIBCXX_USE_NOEXCEPT;

	// virtual keyword in base class destructor is important to invoke 
	// child class destructor for pointer or reference variable
	virtual ~RmoException() _GLIBCXX_USE_NOEXCEPT;
	// it is important to specify const modifier to enable polymorphic what()
	virtual const char * what() const _GLIBCXX_USE_NOEXCEPT;
private:
    QString m_qsMsg;
    mutable QByteArray m_baLocalEnc;
};

class QGraphicsScene;
//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
class QExceptionDialog : public QDialog {
    Q_OBJECT

public:
    explicit QExceptionDialog(QString exceptionWhat, QWidget *parent = 0);
    ~QExceptionDialog();

private:
    QGraphicsScene *m_pgsPlane;
};
//-----------------------------------------------------------------------
//
//-----------------------------------------------------------------------
void showExceptionDialog(QString exceptionWhat, QWidget *parent = 0);

#endif // QEXCEPTIONDIALOG_H

