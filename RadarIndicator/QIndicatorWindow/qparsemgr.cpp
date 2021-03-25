#include "qparsemgr.h"
#include "qindicatorwindow.h"
#include "qexceptiondialog.h"

double dParseProgressBarStep=(double)PARSE_PROGRESS_BAR_STEP/PARSE_PROGRESS_BAR_MAX;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QParseMgr::QParseMgr(QIndicatorWindow *pOwner) :
         m_pOwner(pOwner)
       , m_pRegFileParser(NULL)
       , m_pPoi(NULL) {
    // map members for convenience
    m_pRegFileParser           = m_pOwner->m_pRegFileParser;
    m_pSqlModel                = m_pOwner->m_pSqlModel;
    m_pPoi                     = m_pOwner->m_pPoi;
    m_pRegFileParser->m_pParseMgr = this;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QParseMgr::~QParseMgr() {
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QParseMgr::startParsing(QObject *pSender) {

    // reset totals
    // m_uTot = 0;
    // m_uDB = 0;
    // m_uHDD = 0;
    // m_uSam = 0;
    // get access to QPropPages dynamic object (!take care - only exists while dlg)
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pSender);
    qDebug() << "startParsing(): stopping simulation timer";
    m_pOwner->toggleTimer(/* bool bStop= */ true);
    m_pOwner->m_bParsingInProgress=true;
    pPropPages->m_ppbAccept->setEnabled(false);
    pPropPages->m_ppbParse->setEnabled(false);
    pPropPages->m_ppbarParseProgress->setMaximum(PARSE_PROGRESS_BAR_MAX);
    // ensure progress bar updates
    QObject::connect(m_pOwner,SIGNAL(updateParseProgressBar(double)),pPropPages,SIGNAL(updateParseProgressBar(double)));
    // before parsing can start - change DB connection and reopen it
    m_pRegFileParser->changeAndReopenDB(pSender);
    // list registration files within selected directory
    m_pRegFileParser->listRegFilesDir(pSender);
    // reset user interface and reset m_bParsingInProgress flag
    pPropPages->m_ppbAccept->setEnabled(true);
    pPropPages->m_ppbParse->setEnabled(true);
    m_pOwner->m_bParsingInProgress=false;
    // qDebug() << "Total seconds: " << m_uTot*1.0e-3;
    // qDebug() << "   DB seconds: " << m_uDB*1.0e-3;
    // qDebug() << "  HDD seconds: " << m_uHDD*1.0e-3;
    // qDebug() << "  Sam seconds: " << m_uSam*1.0e-3;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QParseMgr::doUpdateParseProgressBar(bool bReset /* =false */) {
    static int iCurr=0,iPrev=0;
    if (bReset) {
        iCurr=iPrev=0;
        emit m_pOwner->updateParseProgressBar(iCurr);
        return;
    }
    iCurr = qRound(m_pRegFileParser->getProgress()/dParseProgressBarStep);
    if (iCurr==iPrev) return;
    iPrev=iCurr;
    emit m_pOwner->updateParseProgressBar(iCurr*dParseProgressBarStep);
    int iMaxMSecs=500;
    QCoreApplication::processEvents(QEventLoop::AllEvents,iMaxMSecs);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::onParseDataFile() {
    if (m_pParseMgr) m_pParseMgr->startParsing(QObject::sender());
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::onUpdateParseProgressBar(double dCurr) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (QObject::sender());
    int iMax=pPropPages->m_ppbarParseProgress->maximum();
    int iVal = qRound(iMax*dCurr/PARSE_PROGRESS_BAR_STEP)*PARSE_PROGRESS_BAR_STEP;
    // qDebug() << "onUpdateParseProgressBar: dCurr=" << dCurr << " iVal=" << iVal ;
    pPropPages->m_ppbarParseProgress->setValue(iVal);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::onParsingMsgSet(QString qsMsg) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (QObject::sender());
    pPropPages->m_plbParsingMsg->setText(qsMsg);
}
