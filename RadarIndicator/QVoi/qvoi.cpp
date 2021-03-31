#include "qvoi.h"
#include "qcoretracefilter.h"
#include "qtracefilter.h"
#include "qinisettings.h"
#include "qexceptiondialog.h"

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QVoi::QVoi(QObject *parent /* = 0 */)
        : QObject(parent)
        , m_coreInstance(QCoreTraceFilter::getInstance())
        , m_dTcurrent(0.0e0) {
    QIniSettings &iniSettings = QIniSettings::getInstance();
    QIniSettings::STATUS_CODES scRes;
    iniSettings.setDefault(QVOI_MEAS_NOISE_VAR,0.0e0);
    m_coreInstance.m_dMeasNoise = iniSettings.value(QVOI_MEAS_NOISE_VAR,scRes).toDouble();
    m_coreInstance.m_dCorrSignif = 0.8e0;
    m_coreInstance.m_iMaxClusterSz = 6;
    m_coreInstance.initCorrThresh();
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QVoi::~QVoi() {
    // qDebug() << "inside ~QVoi()";
    QIniSettings &iniSettings = QIniSettings::getInstance();
    iniSettings.setNum(QVOI_MEAS_NOISE_VAR, m_coreInstance.m_dMeasNoise);
    m_coreInstance.cleanup();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QVoi::addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    QTabWidget *pTabWidget = qobject_cast<QTabWidget *> (pPropTabs);

    QWidget *pWidget=new QWidget;
    QVBoxLayout *pVLayout=new QVBoxLayout;

    QGridLayout *pGLayout=new QGridLayout;
    pGLayout->addWidget(new QLabel("Measurement noise"),0,0);
    pPropPages->m_pleMeasNoise = new QLineEdit(QString::number(m_coreInstance.m_dMeasNoise,'f',1));
    pPropPages->m_pleMeasNoise->setMaximumWidth(PROP_LINE_WIDTH);
    pGLayout->addWidget(pPropPages->m_pleMeasNoise,0,1);

    pGLayout->setColumnStretch(2,100);
    pGLayout->setColumnStretch(5,100);
    pVLayout->addLayout(pGLayout);

    pVLayout->addStretch();
    pWidget->setLayout(pVLayout);
    pTabWidget->insertTab(iIdx,pWidget,QVOI_PROP_TAB_CAPTION);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QVoi::propChanged(QObject *pPropDlg) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    bool bOk;
    QString qs;

    qs = pPropPages->m_pleMeasNoise->text();
    double dMeasNoise = qs.toDouble(&bOk);
    if (bOk) m_coreInstance.m_dMeasNoise = dMeasNoise;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QVoi::processPrimaryPoint(
        double dTsExact, // exact time of strobe exection (seconds)
        double dR,       // distance (meters)
        double dElRad,   // elevation (radians)
        double dAzRad,   // azimuth (radians)
        double dV_D,     // Doppler velocity (m/s)
        double dVDWin,   // Doppler velocity window (m/s)
        struct sVoiPrimaryPointInfo &sPriPtInfoOut) {  // output structure
    // update current time (seconds)
    m_dTcurrent = (dTsExact > m_dTcurrent) ? dTsExact : m_dTcurrent;
    // call addPrimaryPoint() method of the singleton
    QCoreTraceFilter::sPrimaryPt newPrimaryPoint;
    newPrimaryPoint.dTs = dTsExact;
    newPrimaryPoint.dR = dR;
    newPrimaryPoint.dElRad = dElRad;
    newPrimaryPoint.dAzRad = dAzRad;
    newPrimaryPoint.dV_D = dV_D;
    newPrimaryPoint.dVDWin = dVDWin;
    quint32 uPriPtIndex=m_coreInstance.addPrimaryPoint(newPrimaryPoint);
    // fill (partially) the output structure
    sPriPtInfoOut.uPriPtIndex=uPriPtIndex;
    sPriPtInfoOut.uFilterIndex = newPrimaryPoint.uFilterIndex;
    sPriPtInfoOut.qpfDistVD=QPointF(dR,dV_D);
    return;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QVoi::listFilters(QList<struct sVoiFilterInfo> &qlFiltersInfo) {
    // kill stale filters
    QListIterator<quint32> i1(m_coreInstance.m_qmFilters.keys());
    while (i1.hasNext()) {
        quint32 uIndex=i1.next();
        QTraceFilter *pFilter = m_coreInstance.m_qmFilters.value(uIndex,NULL);
        if (pFilter && pFilter->isStale(m_dTcurrent)) {
            delete m_coreInstance.m_qmFilters.take(uIndex);
        }
    }
    // list the remaining filters and return the list as qlFiltersInfo
    QMapIterator<quint32,QTraceFilter *> i(m_coreInstance.m_qmFilters);
    while (i.hasNext()) {
        i.next();
        QTraceFilter *pFilter = i.value();
        quint32 uIndex=i.key();
        QCoreTraceFilter::sFilterState & sState = pFilter->getState(m_dTcurrent);
        struct sVoiFilterInfo sInfo;
        sInfo.qpfDistVD=sState.qpfDistVD;
        sInfo.qsFormular=sState.qsName;
        sInfo.bTrackingOn=pFilter->isTrackingOn();
        sInfo.uFilterIndex=uIndex;
        qlFiltersInfo.append(sInfo);
    }
}
