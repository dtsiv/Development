#include "qindicatorwindow.h"
#include "qexceptiondialog.h"
#include "qtargetsmap.h"

#include "nr.h"
using namespace std;

QIndicatorWindow::QIndicatorWindow(QWidget *parent, Qt::WindowFlags flags)
        : QMainWindow(parent, flags)
          , m_pSqlModel(NULL)
          , m_pPoi(NULL)
          , m_pRegFileParser(NULL)
          , m_pTargetsMap(NULL)
          , m_pVoi(NULL)
          , m_pStopper(NULL)
          , settingsAct(NULL)
          , m_pParseMgr(NULL)
          , m_pSimuMgr(NULL)
          , m_bParsingInProgress(false)
          , m_bSigSelInProgress(false)
          , m_plbFalseAlarmInfo(NULL)
          , m_plbStrobNo(NULL)
          , m_qsFalseAlarmInfo("<FA>: %1")
          , m_qsStrobNo("Strob # %1")
          , m_qsDetectionsInfo("Alarms: %1")
          , m_paFalseAlarmGaussNoiseTest(NULL)
          , m_bUseForAll(true)
          , m_bShutDown(false) {
    // set radar icon
    setWindowIcon(QIcon(QPixmap(":/Resources/spear.ico")));

    // setup status bar
    QFont font=QApplication::font();
    QFontMetrics fmStatusMsg(font);
    m_plbStatusArea=new QLabel(QString::fromLocal8Bit(CONN_STATUS_DISCONN));
    m_plbStatusArea->setMinimumWidth(fmStatusMsg.width(m_plbStatusArea->text())+STATUS_MESSAGE_PADDING);
    statusBar()->addPermanentWidget(m_plbStatusArea);
    m_plbStrobNo = new QLabel;
    m_plbStrobNo->setMinimumWidth(fmStatusMsg.width(m_qsStrobNo.arg(1234))+STATUS_MESSAGE_PADDING);
    statusBar()->addPermanentWidget(m_plbStrobNo);
    m_plbFalseAlarmInfo = new QLabel;
    m_plbFalseAlarmInfo->setMinimumWidth(fmStatusMsg.width(m_qsFalseAlarmInfo.arg(0.1e0,0,'e',1))+STATUS_MESSAGE_PADDING);
    statusBar()->addPermanentWidget(m_plbFalseAlarmInfo);
    m_plbDetectionsInfo=new QLabel;
    m_plbDetectionsInfo->setMinimumWidth(fmStatusMsg.width(m_qsDetectionsInfo.arg(0.1e0,0,'e',1))+STATUS_MESSAGE_PADDING);
    statusBar()->addPermanentWidget(m_plbDetectionsInfo);

    m_qtLastInfoUpdate=QDateTime::currentDateTime();
    // add spacer item to the status bar
    statusBar()->addPermanentWidget(new QLabel(" "),99);
    m_plbStatusMsg=new QLabel(QString::fromLocal8Bit("Press Control-P for settings"));
    m_plbStatusMsg->setMinimumWidth(fmStatusMsg.width(m_plbStatusMsg->text())+STATUS_MESSAGE_PADDING);
    statusBar()->addPermanentWidget(m_plbStatusMsg);

    // setup actions
    m_paFalseAlarmGaussNoiseTest = new QAction(this);
    m_paFalseAlarmGaussNoiseTest->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_G));
    QObject::connect(m_paFalseAlarmGaussNoiseTest, SIGNAL(triggered()), this, SLOT(onFalseAlarmGaussNoiseTest()));

    // settings object and main window adjustments
    QIniSettings &iniSettings = QIniSettings::getInstance();
    QIniSettings::STATUS_CODES scRes;
    iniSettings.setDefault(SETTINGS_KEY_GEOMETRY,QSerial(QRect(200, 200, 200, 200)).toBase64());
    QString qsEncodedGeometry = iniSettings.value(SETTINGS_KEY_GEOMETRY,scRes).toString();
    // qDebug() << scRes << " err=" << QIniSettings::INIT_ERROR << " notf=" << QIniSettings::KEY_NOT_FOUND << " def=" << QIniSettings::READ_DEFAULT << " valid=" << QIniSettings::READ_VALID;
    bool bOk;
    QRect qrGeometry = QSerial(qsEncodedGeometry).toRect(&bOk);
    // qDebug() << "qrGeometry=" << qrGeometry << " bOk=" << bOk;
    if (bOk) setGeometry(qrGeometry);

    // catch user input enter event
    QCoreApplication *pCoreApp = QCoreApplication::instance();
    pCoreApp->installEventFilter(new UserControlInputFilter(this,pCoreApp));

    // create actions
    settingsAct = new QAction(QIcon(":/Resources/settings.ico"), tr("Settings"), this);
    settingsAct->setShortcut(QKeySequence("Ctrl+P"));
    settingsAct->setStatusTip(QString::fromLocal8Bit("Settings"));
    settingsAct->setText(QString::fromLocal8Bit("Settings"));
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(onSetup()));
    addAction(m_paFalseAlarmGaussNoiseTest);

    // create instances of components
    m_pSqlModel = new QSqlModel();
    if (m_pSqlModel) m_qlObjects << qobject_cast<QObject *> (m_pSqlModel);

    m_pPoi = new QPoi(this);
    if (m_pPoi) m_qlObjects << qobject_cast<QObject *> (m_pPoi);

    m_pRegFileParser = new QRegFileParser(this);
    if (m_pRegFileParser) m_qlObjects << qobject_cast<QObject *> (m_pRegFileParser);

    m_pTargetsMap = new QTargetsMap(this);
    if (m_pTargetsMap) m_qlObjects << qobject_cast<QObject *> (m_pTargetsMap);

    m_pVoi = new QVoi(this);
    if (m_pVoi) m_qlObjects << qobject_cast<QObject *> (m_pVoi);

    // connect simulation timer
    m_simulationTimer.setInterval(m_pTargetsMap->m_uTimerMSecs);
    QObject::connect(&m_simulationTimer,SIGNAL(timeout()),SLOT(onSimulationTimeout()));

    m_pParseMgr = new QParseMgr(this);
    m_pSimuMgr = new QSimuMgr(this);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QIndicatorWindow::~QIndicatorWindow() {
    // settings object and main window adjustments
    QIniSettings &iniSettings = QIniSettings::getInstance();
    QRect qrCurGeometry=geometry();
    QString qsEncodedGeometry = QSerial(qrCurGeometry).toBase64();
    // qDebug() << geometry() << " " << qsEncodedGeometry;
    iniSettings.setValue(SETTINGS_KEY_GEOMETRY, qsEncodedGeometry);

    //-------------------this does not work!  qDebug() << "deleting from m_qlObjects";
    // for (int i=0; i<m_qlObjects.size(); i++) {
    //     QObject *pObj=m_qlObjects.at(i);
    //     if (pObj) delete pObj; // need to check destructor, check order of objects in QList!
    // }
    //-----------------------------------------------------------------------------------------
    if (m_pSqlModel)  { delete m_pSqlModel; m_pSqlModel=NULL; }
    if (m_pPoi)       { delete m_pPoi; m_pPoi=NULL; }
    if (m_pRegFileParser)  { delete m_pRegFileParser; m_pRegFileParser=NULL; }
    if (m_pTargetsMap)     { delete m_pTargetsMap; m_pTargetsMap=NULL; }
    if (m_pVoi)       { delete m_pVoi; m_pVoi=NULL; }
    if (m_pStopper)   { delete m_pStopper; m_pStopper=NULL; }
    if (m_pParseMgr)  { delete m_pParseMgr; m_pParseMgr=NULL; }
    if (m_pSimuMgr)   { delete m_pSimuMgr; m_pSimuMgr=NULL; }
    if (m_paFalseAlarmGaussNoiseTest) { delete m_paFalseAlarmGaussNoiseTest; m_paFalseAlarmGaussNoiseTest=NULL;}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::initComponents() {
    QTime qtCurr = QTime::currentTime();
    QDockWidget *dock = new QDockWidget(QTARGETSMAP_DOC_CAPTION);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    MapWidget *pMapWidget = m_pTargetsMap->getMapInstance();
    if (pMapWidget) {
        pMapWidget->setFocusPolicy(Qt::StrongFocus);
        dock->setWidget(pMapWidget);
    }
    addDockWidget(Qt::RightDockWidgetArea, dock);

    // connect to sqlite DB
    if (m_pSqlModel->openDatabase()) {
        m_plbStatusArea->setText(CONN_STATUS_SQLITE);
        // exec main SELECT query
        bool bMainSelectQueryExecOk = m_pSqlModel->execMainSelectQuery();
        if (!bMainSelectQueryExecOk) {
            qDebug() << "m_pSqlModel->execMainSelectQuery() failed";
        }
    }

    // time eater to display the stopper window
    while(qtCurr.msecsTo(QTime::currentTime())<STOPPER_MIN_DELAY_MSECS) {
        qApp->processEvents();
    }
    QTimer::singleShot(0,this,SLOT(hideStopper()));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::hideStopper() {
    if (m_pStopper) {
        delete m_pStopper;
        m_pStopper = NULL;
    }
    setVisible(true);
    if (m_pSqlModel->isDBOpen()) m_simulationTimer.start();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::onSetup() {
    static bool bInSetup=false;
    if (!bInSetup) {
        bInSetup=true;
        settingsAct->setEnabled(false);
        toggleTimer(/* bool bStop= */ true);
        QCoreApplication::processEvents();
        QPropPages dlgPropPages(this);
        if (!m_qrPropDlgGeo.isEmpty()) dlgPropPages.setGeometry(m_qrPropDlgGeo);
        dlgPropPages.exec();
        // remember current geometry of setup dlg for ini-file
        m_qrPropDlgGeo=dlgPropPages.geometry();
        // adapt new settings if dialog was accepted
        if (dlgPropPages.result() == QDialog::Accepted) {
            // clear debugging flag
            m_pSimuMgr->m_bFalseAlarmGaussNoiseTest = false;
            // restart simulation from the most early strob
            m_pSimuMgr->restart();
        }
        else { // dialog was rejected
            // update the DB connection status
            if (!m_pSqlModel->isDBOpen()) {
                m_plbStatusArea->setText(CONN_STATUS_DISCONN);
            }
            // if currently the main select query is still active
            if (m_pSqlModel->m_mainSelectQuery.isActive()) {
                // ... then continue simulation
                m_simulationTimer.start();
            }
            // otherwise we do not care. Just wave and smile)
        }
        // setup was finished and results applied
        settingsAct->setEnabled(true);
        bInSetup=false;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::fillTabs(QObject *pPropDlg, QObject *pPropTabs) {
    for (int i=0; i<m_qlObjects.size(); i++) {
        QMetaObject::invokeMethod(m_qlObjects.at(i),"addTab",Q_ARG(QObject *,pPropDlg), Q_ARG(QObject *,pPropTabs), Q_ARG(int, i));
        qobject_cast<QTabWidget*>(pPropTabs)->widget(i)->setAutoFillBackground(true);
   }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::propChanged(QObject *pPropDlg) {
    for (int i=0; i<m_qlObjects.size(); i++) {
        QMetaObject::invokeMethod(m_qlObjects.at(i),"propChanged",Q_ARG(QObject *,pPropDlg));
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::onFalseAlarmGaussNoiseTest() {
    // stop simulation timer and wait for slots finished
    toggleTimer(/*bool bStop=*/ true);
    QCoreApplication::processEvents();

    //-------------------------- expt detection
    //    quint64 iTry=0, Ntries=1.0e10;
    //    quint64 iDet=0;
    //    qDebug() << "Starting expt detection ...";
    //    while(iTry<Ntries) {
    //        iDet += m_pPoi->exptDetection();
    //        if ((++iTry%(int)1.0e8)==0) {
    //            QCoreApplication::processEvents();
    //            qDebug() << "Tries: " << iTry
    //                     << " P_FA_exp="
    //                     << QString::number(1.0e0*iDet/iTry,'e',6);
    //        }
    //    }
    //    qDebug() << "Expt detection: P_FA_theory="
    //             << QString::number(m_pPoi->m_dFalseAlarmProb,'e',6)
    //             << " P_FA_exp="
    //             << QString::number(1.0e0*iDet/Ntries,'e',6);
    //----------------------------------------

    // restart with FalseAlarmGaussNoiseTest debug flag
    m_pSimuMgr->m_bFalseAlarmGaussNoiseTest = true;
    m_pSimuMgr->restart();
}
