#ifndef QSIMUMGR_H
#define QSIMUMGR_H

#include <QtGlobal>
#include <QFile>
#include <QTextStream>
#include <QPoint>
#include <QPointF>

class QIndicatorWindow;
class QPoi;
class QVoi;
class QSqlModel;
class QTargetsMap;

class QSimuMgr {
public:
    QSimuMgr(QIndicatorWindow *pOwner);
    ~QSimuMgr();
    void processStrob();
    void restart();
    void getStrobRecord();
    void checkStrobeParams();
    void getBeamData();
    void falseAlarmGaussNoiseTest();
    void getectTargets();
    void getAngles();
    void phaseCoherenceTest();
    void dataFileOutput();
    void addTargetMarker();
    void updateStatusInfo();
    void traceFilter();

public:
    quint32 m_uStrobeNo;
    bool m_bFalseAlarmGaussNoiseTest;

private:
    QIndicatorWindow *m_pOwner;
    QPoi *m_pPoi;
    QVoi *m_pVoi;
    QSqlModel *m_pSqlModel;
    QTargetsMap *m_pTargetsMap;
    QFile m_qfPeleng;
    QTextStream m_tsPeleng;
    bool m_bStarted;
    double m_dStartTime;
	double m_dTsRounded;
    double m_dTsExact;
    qint64 m_iRecId;
    qint64 m_iTimeStamp;
    QByteArray m_baBeamsSumDP;
    QByteArray *m_pbaBeamDataDP;
	double m_dElevationRad;
	double m_dAzimuthRad;
	double m_dElevationDeg;
	double m_dAzimuthDeg;
	double m_dElevationScan;
	double m_dAzimuthScan;
	double m_dElevationResult;
	double m_dAzimuthResult;
	QPoint m_qpRepresentative;
	QPointF m_qpfWeighted;
	quint32 m_uCandNum;
    double m_dSNRc_rep;
	double *m_pBeamAmplRe;
	double *m_pBeamAmplIm;
	bool m_bAnglesOk;
    int m_iVoiIdx;
    qint64 m_iNoisemapGUID;
    QTextStream m_tsPhaseCoherence;
    QFile m_qfPhaseCoherence;
};

#endif // QSIMUMGR_H
