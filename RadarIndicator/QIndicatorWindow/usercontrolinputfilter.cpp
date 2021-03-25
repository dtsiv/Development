#include "usercontrolinputfilter.h"
#include "qindicatorwindow.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
UserControlInputFilter::UserControlInputFilter(QIndicatorWindow *pOwner, QObject *pobj /*=0*/)
    : QObject(pobj), m_pOwner(pOwner) {
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*virtual*/ bool UserControlInputFilter::eventFilter([[maybe_unused]]QObject *pobj, QEvent *pe) {
    QEvent::Type eventType = pe->type();
    QKeyEvent *pKeyEvent = (QKeyEvent*)pe;
    // allow quit from application
    if ((eventType == QEvent::KeyPress)
               && (pKeyEvent->key()==Qt::Key_Q || pKeyEvent->key()==Qt::Key_X)
               && (pKeyEvent->modifiers() & Qt::ControlModifier || pKeyEvent->modifiers() & Qt::AltModifier)) {
            m_pOwner->m_bShutDown=true;
            m_pOwner->close();
            qApp->quit();
            return true;
    }

    // allow user events, block hotkeys for modal dialog of signal sampling selection
    if (m_pOwner->m_bSigSelInProgress==true) {
        if (eventType == QEvent::KeyPress) {
            // block all hotkeys
            if ((pKeyEvent->key()==Qt::Key_P || pKeyEvent->key()==Qt::Key_S)
                           && (pKeyEvent->modifiers() & Qt::ControlModifier)
              || pKeyEvent->key()==Qt::Key_Escape) {
                return true;
            }
        }
        // allow all other events
        return false;
    }

    // block all user input events during parsing
    if (m_pOwner->m_bParsingInProgress==true) {

        // block all other user input events
        if (    eventType == QEvent::KeyPress
             || eventType == QEvent::KeyRelease
             || eventType == QEvent::MouseButtonPress
             || eventType == QEvent::MouseButtonRelease
             || eventType == QEvent::MouseButtonDblClick) {
            // block all possible events. Which else?...
            return true;
        }
    }

    // common user input events
    if (eventType == QEvent::KeyPress) {
        QKeyEvent *pKeyEvent = (QKeyEvent*)pe;
        // qDebug() << "key= " << pKeyEvent->key();
        // qDebug() << "pKeyEvent->modifiers()= " << QString::number((int)pKeyEvent->modifiers(),16);
        // if (pKeyEvent->key()==Qt::Key_Return) {
        //    // block all other events
        //    return true;
        //}
        if ((pKeyEvent->key()==Qt::Key_P)
               && (pKeyEvent->modifiers() & Qt::ControlModifier)) {
            m_pOwner->onSetup();
            return true;
        }
        else if (pKeyEvent->key()==Qt::Key_Escape) {
            bool bStop = true;
            m_pOwner->toggleTimer(bStop);
            return true;
        }
        else if ((pKeyEvent->key()==Qt::Key_Q || pKeyEvent->key()==Qt::Key_X)
               && (pKeyEvent->modifiers() & Qt::ControlModifier || pKeyEvent->modifiers() & Qt::AltModifier)) {
            qApp->quit();
            return true;
        }
        else if ((pKeyEvent->key()==Qt::Key_S) && (pKeyEvent->modifiers() & Qt::ControlModifier)) {
            m_pOwner->toggleTimer();
            return true;
        }
    }
    // allow all other events
    return false;
}
