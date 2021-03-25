#ifndef QINDICATORWINDOW_H
#define QINDICATORWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QtWidgets>
#include <QMetaObject>
#include <QtGlobal>

#include "qinisettings.h"
#include "qproppages.h"
#include "qsqlmodel.h"
#include "qregfileparser.h"
#include "qtargetsmap.h"
#include "qpoi.h"
#include "qvoi.h"
#include "qstopper.h"
#include "usercontrolinputfilter.h"
#include "qparsemgr.h"
#include "qsimumgr.h"

#define SETTINGS_KEY_GEOMETRY                   "geometry"

#define STATUS_MESSAGE_PADDING                  20

class QPulseSamplingDlg;

class QIndicatorWindow : public QMainWindow
{
	Q_OBJECT

public:
    QIndicatorWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QIndicatorWindow();

    Q_INVOKABLE void fillTabs(QObject *pPropDlg, QObject *pPropTabs);
    Q_INVOKABLE void propChanged(QObject *pPropDlg);

public slots:
    void hideStopper();
    void onSetup();
    void onSimulationTimeout();
    void onParseDataFile();
    void onUpdateParseProgressBar(double dCurr);
    void onParsingMsgSet(QString qsMsg);
    void onFalseAlarmGaussNoiseTest();

signals:
    void updateParseProgressBar(double dCurr);

public:
    void showStopper();
    void initComponents();
    void toggleTimer(bool bStop = false);

private:
    QList<QObject*> m_qlObjects;
    QSqlModel *m_pSqlModel;
    QRegFileParser *m_pRegFileParser;
    QTargetsMap *m_pTargetsMap;
    QPoi *m_pPoi;
    QVoi *m_pVoi;

private:
    QStopper *m_pStopper;
    QAction *settingsAct;

private:
    QRect m_qrPropDlgGeo;
    QLabel *m_plbStatusArea;
    QLabel *m_plbStatusMsg;
    QLabel *m_plbFalseAlarmInfo;
    QString m_qsFalseAlarmInfo;
    QLabel *m_plbStrobNo;
    QString m_qsStrobNo;
    QLabel *m_plbDetectionsInfo;
    QString m_qsDetectionsInfo;
    QTimer m_simulationTimer;
    QParseMgr *m_pParseMgr;
    QSimuMgr *m_pSimuMgr;
    QDateTime m_qtLastInfoUpdate;
    QAction *m_paFalseAlarmGaussNoiseTest;

public:
    bool m_bParsingInProgress;
    bool m_bUseForAll;
    bool m_bSigSelInProgress;
    bool m_bShutDown;

friend class QParseMgr;
friend class QSimuMgr;
friend class QRegFileParser;
friend class QPulseSamplingDlg;
friend class QTargetsMap;
};

#endif // QINDICATORWINDOW_H
