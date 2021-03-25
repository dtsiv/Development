#include "qvoi.h"
#include "qcoretracefilter.h"
#include "qinisettings.h"
#include "qexceptiondialog.h"

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QVoi::QVoi(QObject *parent /* = 0 */)
        : QObject(parent)
        , m_coreInstance(QCoreTraceFilter::getInstance()) {
    QIniSettings &iniSettings = QIniSettings::getInstance();
    QIniSettings::STATUS_CODES scRes;
    iniSettings.setDefault(QVOI_MEAS_NOISE_VAR,0.0e0);
    m_coreInstance.m_dMeasNoise = iniSettings.value(QVOI_MEAS_NOISE_VAR,scRes).toDouble();
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
int QVoi::processPrimaryPoint(
        double dTsExact, // exact time of strobe exection (seconds)
        double dR,       // distance (meters)
        double dElRad,   // elevation (radians)
        double dAzRad,   // azimuth (radians)
        double dV_D,     // Doppler velocity (m/s)
        double dVDWin) { // Doppler velocity window (m/s)
    QCoreTraceFilter::sPrimaryPt newPrimaryPoint;
    newPrimaryPoint.dTs = dTsExact;
    newPrimaryPoint.dR = dR;
    newPrimaryPoint.dElRad = dElRad;
    newPrimaryPoint.dAzRad = dAzRad;
    newPrimaryPoint.dV_D = dV_D;
    newPrimaryPoint.dVDWin = dVDWin;
    return m_coreInstance.addPrimaryPoint(newPrimaryPoint);
}
