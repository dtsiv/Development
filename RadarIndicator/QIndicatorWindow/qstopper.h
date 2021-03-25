#ifndef QSTROPPER_H
#define QSTROPPER_H

#include <QMainWindow>
#include <QtGui>
#include <QtWidgets>
#include <QMetaObject>
#include <QtGlobal>
#include <QSplashScreen>

#define STOPPER_MIN_DELAY_MSECS                 1000


//*****************************************************************************
//
//*****************************************************************************
class QStopper : public QSplashScreen {
public:
    QStopper();
    ~QStopper();

protected:
    virtual void drawContents (QPainter *painter);

private:
    QPixmap m_pmStopper;
};

#endif // QSTROPPER_H
