#ifndef USERCONTROLINPUTFILTER_H
#define USERCONTROLINPUTFILTER_H

#include <QMainWindow>
#include <QtGui>
#include <QtWidgets>
#include <QMetaObject>
#include <QtGlobal>
#include <QObject>

class QIndicatorWindow;

//*****************************************************************************
//
//*****************************************************************************
class UserControlInputFilter : public QObject {
protected:
    virtual bool eventFilter(QObject*, QEvent*);

public:
    UserControlInputFilter(QIndicatorWindow* pOwner, QObject *pobj=0);

private:
    QIndicatorWindow *m_pOwner;
};


#endif // USERCONTROLINPUTFILTER_H
