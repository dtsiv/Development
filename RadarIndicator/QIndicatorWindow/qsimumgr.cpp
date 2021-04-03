#include "qsimumgr.h"
#include "qindicatorwindow.h"
#include "qexceptiondialog.h"
#include "qregfileparser.h"
#include "qvoi.h"
#include "qtargetsmap.h"

#include <cmath>

#define INFO_UPDATE_MSEC 500

// #define PHASE_COHERENCE
// #define AMPLITUDE_METHOD

QFile qfStrobeParams;

#ifdef PHASE_COHERENCE
double getArg(double dRe, double dIm, bool &bOk) {
    bOk = false;
    double dEPS=1.0e-40;
    double dR2=dRe*dRe+dIm*dIm;
    if (dR2 < dEPS) return 0.0e0;
    bOk=true;
    return atan2(dIm,dRe);
}
#endif

struct sVoiPrimaryPointInfo QSimuMgr::m_sPriPtInfo;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSimuMgr::QSimuMgr(QIndicatorWindow *pOwner) :
          m_pOwner(pOwner)
        , m_pPoi(NULL)
        , m_pVoi(NULL)
        , m_bStarted(false)
        , m_bFalseAlarmGaussNoiseTest(false)
        , m_uStrobeNo(0)
        , m_dStartTime(0.0e0)
        , m_pBeamAmplRe(NULL)
        , m_pBeamAmplIm(NULL)
        , m_pbaBeamDataDP(NULL) {

    // local copies for convenience
    m_pPoi = m_pOwner->m_pPoi;
    m_pVoi = m_pOwner->m_pVoi;
    m_pSqlModel = m_pOwner->m_pSqlModel;
    m_pTargetsMap = m_pOwner->m_pTargetsMap;

    m_qfPeleng.setFileName("peleng.dat");
    if (!m_qfPeleng.open(QIODevice::WriteOnly)) {
        qDebug() << "QSimuMgr: m_qfPeleng.open failed";
    }
    m_tsPeleng.setDevice(&m_qfPeleng);

    m_pBeamAmplRe = new double[QPOI_NUMBER_OF_BEAMS];
    m_pBeamAmplIm = new double[QPOI_NUMBER_OF_BEAMS];
    m_pbaBeamDataDP = new QByteArray[QPOI_NUMBER_OF_BEAMS];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSimuMgr::~QSimuMgr() {
    if (m_qfPeleng.isOpen()) m_qfPeleng.close();
    if (m_pBeamAmplRe) delete m_pBeamAmplRe; m_pBeamAmplRe = NULL;
    if (m_pBeamAmplIm) delete m_pBeamAmplIm; m_pBeamAmplIm = NULL;
    if (m_pbaBeamDataDP) delete m_pbaBeamDataDP; m_pbaBeamDataDP = NULL;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::processStrob() {

    // obtain the record of strobs table of database
    getStrobRecord();

    // check strobe parameters
    checkStrobeParams();

    // obtain raw samples of the received signal for every beam
    getBeamData();

    // Debugging the match statistics block: use Gauss noise input
    if (m_bFalseAlarmGaussNoiseTest) falseAlarmGaussNoiseTest();

    // Obtain list of primary points for targets
    getectTargets();

    if (!m_bStarted) {
        m_bStarted = true;
    }

    // list filters info
    filterMarkers();

    // Update status info
    updateStatusInfo();

    return;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::getStrobRecord() {
    // map members for convenience
    QByteArray &baStructStrobeData  = m_pPoi->m_baStructStrobeData;
    quint32 &uFileVer               = m_pPoi->m_uFileVer;

    qint32 iNFFT;
    qint32 iFilteredN;

    // get next strob record guid from DB
    if (!m_pSqlModel->getStrobRecord(m_iRecId, (int &)m_uStrobeNo, m_pPoi->m_iBeamCountsNum,
                                     m_iTimeStamp, baStructStrobeData, uFileVer,
                                     m_pPoi->m_baSignalSamplData,
                                     m_pPoi->m_iSigType, m_pPoi->m_iSignalLen,
                                     m_iNoisemapGUID, iNFFT, iFilteredN)) {
        // qDebug() << "getStrobRecord failed";
        m_pOwner->toggleTimer(/* bool bStop= */ true);
        m_pOwner->m_plbStrobNo->setText("Stopped");
        return;
    }
    // Enable debugging of detectTargets()
    m_pPoi->m_uStrobeNoForDebugging = m_uStrobeNo;

    // qDebug() << "getStrobRecord: StrobNo=" << m_uStrobeNo
    //          << " m_iSigType= " << m_pPoi->m_iSigType
    //          << " m_iSignalLen=" << m_pPoi->m_iSignalLen;

    // check file version
    if (uFileVer!=REG::FORMAT_VERSION) {
        throw RmoException(QString("QSimuMgr::processStrob(): getStrobRecord failed - wrong uFileVer = %1")
                           .arg(uFileVer));
        return;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::checkStrobeParams() {
    // map members for convenience
    QByteArray &baStructStrobeData  = m_pPoi->m_baStructStrobeData;
    // update m_pPoi with current ACM::STROBE_DATA struct
    ACM::STROBE_DATA *pStructStrobeData = (ACM::STROBE_DATA*)baStructStrobeData.data();
    CHR::STROBE_HEADER *pStructStrobeHeader = (CHR::STROBE_HEADER*)(&pStructStrobeData->header);
    // assign time of simulation start
    if (!m_bStarted) {
        m_dStartTime = (pStructStrobeHeader->execTime) * (m_pPoi->m_dTs);
    }
    // exact execution time in seconds
    m_dTsExact =((pStructStrobeHeader->execTime) * (m_pPoi->m_dTs) - m_dStartTime) * 1.0e-6;
    // round execution time to millisecond precision
    m_dTsRounded = 1.0e-3 * qRound(m_dTsExact*1.0e+3);

    // check strobe params and update if necessary
    bool bStrobeUnchanged = m_pPoi->checkStrobeParams(baStructStrobeData);
    if (!m_bStarted || !bStrobeUnchanged) {
        // new strobe configuration: update strobe parameters of m_pPoi
        if (!m_pPoi->updateStrobeParams(baStructStrobeData)) {
            throw RmoException("QSimuMgr::processStrob(): updateStrobeParams() failed");
            return;
        }
        // number of points in FFT
        if (!m_pPoi->updateNFFT()) {
            throw RmoException("QSimuMgr::processStrob(): updateNFFT() failed");
            return;
        }

        if (m_pPoi->m_bUseNoiseMap) {
            m_pPoi->m_baNoiseMap = m_pSqlModel->getNoiseMap(m_iNoisemapGUID);
            if (m_pPoi->m_baNoiseMap.isEmpty()) {
                throw RmoException("QSimuMgr::processStrob(): m_pSqlModel->getNoiseMap() failed");
                return;
            }
            if (m_pPoi->m_baNoiseMap.size() != m_pPoi->m_iFilteredN*m_pPoi->m_NFFT*sizeof(double)) {
                throw RmoException("QSimuMgr::processStrob(): m_baNoiseMap.size() mismatch");
                return;
            }
        }
        // append simulation log file
        m_pPoi->appendSimuLog();
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::getBeamData() {
    // raw data array
    // QByteArray baBeamsSumDP;
    // m_iBeamCountsNum = m_NT_ * m_Np   // количество комплексных (Re, Im) отсчётов в стробе
    // ============================================================================================
    // m_Np         // количество импульсов в стробе (число периодов = 1024)
    // m_Ntau       // Длительность импульса (число выборок в одном импульсе = 8)
    // m_NT         // Полное число выборок в одном периоде (повторения импульсов = 200)
    // m_NT_        // Дистанция приема (число выборок, регистрируемых в одном периоде = 80)
    m_baBeamsSumDP.resize(m_pPoi->m_iBeamCountsNum*2*sizeof(double));
    double *pSum = (double *)m_baBeamsSumDP.data();
    for (int i=0; i<2*m_pPoi->m_iBeamCountsNum; i++) { pSum[i]=0.0e0; }

    // Get sample data for all beams
    for (int iBeam=0; iBeam<QPOI_NUMBER_OF_BEAMS; iBeam++) {
        QByteArray baBeam;
        baBeam.clear();
        if (!m_pSqlModel->getBeamData(m_iRecId, iBeam, baBeam)) {
            qDebug() << "getBeamData failed";
            m_pOwner->toggleTimer(/* bool bStop= */ true);
            return;
        }
        if (baBeam.size() != m_pPoi->m_iBeamCountsNum*2*sizeof(qint16)) {
            qDebug() << "getBeamData: baBeam.size() != m_pPoi->iBeamCountsNum*2*sizeof(qint16)";
            m_pOwner->toggleTimer(/* bool bStop= */ true);
            return;
        }
        qint16 *pBeam = (qint16 *)baBeam.data();
        m_pbaBeamDataDP[iBeam]=QByteArray(m_baBeamsSumDP.size(),(char)0);
        double *pBeamDP = (double *)m_pbaBeamDataDP[iBeam].data();
        for (int i=0; i<2*m_pPoi->m_iBeamCountsNum; i++) {
            pBeamDP[i] = 1.0e0*pBeam[i];
            pSum[i] += pBeamDP[i];
        }
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::falseAlarmGaussNoiseTest() {
    double *pSum = (double *)m_baBeamsSumDP.data();
	for (int jPeriod=0; jPeriod<m_pPoi->m_Np; jPeriod++) {
		for (int iSample=0; iSample<m_pPoi->m_NT_; iSample++) {
			int idx = 2*(jPeriod*m_pPoi->m_NT_ + iSample);
			pSum[idx]   = m_pPoi->getGaussDev();
			pSum[idx+1] = m_pPoi->getGaussDev();
		}
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::getectTargets() {
#ifdef PHASE_COHERENCE
    //---------------- prepare debug output to file "phasecoher.dat"
    m_qfPhaseCoherence.setFileName("phasecoher.dat");
    if (!m_bStarted) {
        m_qfPhaseCoherence.open(QIODevice::WriteOnly);
    }
    else {
        m_qfPhaseCoherence.open(QIODevice::Append);
    }
    m_tsPhaseCoherence.setDevice(&m_qfPhaseCoherence);
    //--------------------------------------------------
#endif

    // detect targets
    QByteArray baStrTargets;
    int nTargets;
    // actions are only needed if there are some targets
    if (m_pPoi->detectTargets(m_baBeamsSumDP, baStrTargets, nTargets)) {
        if (baStrTargets.size()!=nTargets*sizeof(struct QPoi::sTarget)) {
            throw RmoException(QString("struct size mismatch: baStrTargets.size()=%1 <> %2")
                               .arg(baStrTargets.size())
                               .arg(nTargets*sizeof(struct QPoi::sTarget)));
            return;
        }
        // update targets grand total
        m_pPoi->uTotalDetections+=nTargets;
        // loop over all targets and place their markers
        for (int iTarget = 0; iTarget < nTargets; iTarget++) {
            struct QPoi::sTarget *pTarData = (struct QPoi::sTarget *)baStrTargets.data()+iTarget;
            m_qpRepresentative = pTarData->qp_rep;
            m_qpfWeighted = pTarData->qpf_wei;
            m_uCandNum = pTarData->uCandNum;
            m_dSNRc_rep = pTarData->SNRc_rep;
            m_d_y2mc_sum = pTarData->y2mc_sum;
            m_iDelay = pTarData->qp_rep.x()-m_pPoi->m_iBlank;

            // get angles: elevation and azimuth for target
            getAngles();

            // pass the primary point into QVoi
            traceFilter();

            // output current values to data file
            dataFileOutput();

            // for the current target: add target marker to QTargetsMap
            addPrimaryPointMarker();

        } // for (int iTarget = 0; iTarget < nTargets; iTarget++) ...
    } // if (m_pPoi->detectTargets()) ...
    // else {
    //     qDebug() << "No targets for strobe " << m_uStrobeNo << " time " << m_dTsRounded;
    // }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::getAngles() {
    // Max signal amplitude for target iTarget found at (iDelay, kDoppler)
    int iDelay = m_qpRepresentative.x() - m_pPoi->m_iBlank;
    int kDoppler = m_qpRepresentative.y();
    if (kDoppler<0 || kDoppler>m_pPoi->m_NFFT-1) {
        throw RmoException(QString("QSimuMgr::processStrob(): kDoppler=%1 m_pPoi->m_NFFT=%2!").arg(kDoppler).arg(m_pPoi->m_NFFT));
        return;
    }
    if (iDelay<0 || iDelay > m_pPoi->m_NT_+m_pPoi->m_Ntau-2) {
        throw RmoException(QString("QSimuMgr::processStrob(): iDelay=%1 m_pPoi->m_NT_=%2!").arg(iDelay).arg(m_pPoi->m_NT_));
        return;
    }
    bool bOk;
    bOk = m_pPoi->getPointDopplerRep(iDelay, kDoppler, m_pbaBeamDataDP, m_pBeamAmplRe, m_pBeamAmplIm);
    if (!bOk) {
        throw RmoException("QSimuMgr::processStrob(): m_pPoi->getPointDopplerRep() failed");
        return;
    }
    // calculate angles from signal amplitudes for beams
    double dPI = 3.14159265e0;
    // double dAzimuth, dElevation; // angles in radians
    double dAzimuthMeas,dElevationMeas;
    m_bAnglesOk = m_pPoi->getAngles(m_pBeamAmplRe, m_pBeamAmplIm, dAzimuthMeas,dElevationMeas);
    // cut unreasonable values
    if (qIsNaN(dAzimuthMeas) || qIsNaN(dElevationMeas)) m_bAnglesOk = false;
    m_dElevationDeg = dElevationMeas*180.0e0/dPI;
    if (abs(m_dElevationDeg)>180.0e0) m_dElevationDeg = 360.0e0;
    m_dAzimuthDeg = dAzimuthMeas*180.0e0/dPI;
    if (abs(m_dAzimuthDeg)>180.0e0) m_dAzimuthDeg = 360.0e0;
    QByteArray &baStructStrobeData  = m_pPoi->m_baStructStrobeData;
    ACM::STROBE_DATA *pStructStrobeData = (ACM::STROBE_DATA*)baStructStrobeData.data();
    // CHR::STROBE_HEADER *pStructStrobeHeader = (CHR::STROBE_HEADER*)(&pStructStrobeData->header);
    CHR::BEAM_POS *pBeamPos = &pStructStrobeData->beamPos;
    m_dAzimuthScan = pBeamPos->beamBeta*180.0e0/32768;
    m_dElevationScan = pBeamPos->beamEpsilon*180.0e0/32768;
    m_dAzimuthResult = m_dAzimuthScan + m_dAzimuthDeg;
    m_dElevationResult = m_dElevationScan + m_dElevationDeg;
    m_dAzimuthRad = m_dAzimuthResult * dPI / 180.0e0;
    m_dElevationRad = m_dElevationResult * dPI / 180.0e0;
    // cut unreasonable values
    if (abs(m_dAzimuthResult)>180.0e0) m_dAzimuthResult = 360.0e0;
    if (abs(m_dElevationResult)>180.0e0) m_dElevationResult = 360.0e0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::dataFileOutput() {

    // data file output
#ifdef PHASE_COHERENCE
    phaseCoherenceTest();
#endif   // end of phase coherence ==================================================================

    // set real number notation QTextStream::FixedNotation
    m_tsPeleng.setRealNumberNotation(QTextStream::FixedNotation);
    m_tsPeleng << qSetFieldWidth(7) << right << qSetRealNumberPrecision(3)
        << m_dTsRounded
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(6) << right << qSetRealNumberPrecision(2)
        << m_dAzimuthScan
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(6) << right << qSetRealNumberPrecision(2)
        << m_dElevationScan
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(10) << right << qSetRealNumberPrecision(3)
        << m_dAzimuthDeg
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(10) << right << qSetRealNumberPrecision(3)
        << m_dElevationDeg
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(10) << right << qSetRealNumberPrecision(3)
        << m_dAzimuthResult
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(10) << right << qSetRealNumberPrecision(3)
        << m_dElevationResult
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(6) << right
        << m_uStrobeNo
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(8) << right << qSetRealNumberPrecision(0)
        << m_qpfWeighted.x()
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(8) << right << qSetRealNumberPrecision(1)
        << m_qpfWeighted.y()
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(4) << right
        << m_qpRepresentative.y()
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(5) << right
        << m_uCandNum
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(2) << right
        << (int)m_pPoi->m_iSigType
        << qSetFieldWidth(0) << "\t"
        << qSetFieldWidth(2) << right
        << (int)m_bAnglesOk
        << endl;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::traceFilter() {
    double dVDWin = m_pPoi->m_dVelCoef*m_pPoi->m_NFFT;         // Doppler velocity window (m/s)

    m_pVoi->processPrimaryPoint(
        m_dTsExact,
        m_qpfWeighted.x(),
        m_dElevationRad,
        m_dAzimuthRad,
        m_qpfWeighted.y(),
        dVDWin,
        m_sPriPtInfo);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::addPrimaryPointMarker() {
    // fill the formular
    QString &qsLegend=m_sPriPtInfo.qsFormular;
    qsLegend.clear();
    QString qsStrobeNo=QString("S:%1").arg(m_uStrobeNo);
    // Weinberg 2017 parameter
    // SNR-related parameter: dTau = dFrac/(0.5*n_noise);
    // false alarm probability: dProb = exp(-(0.5*n_noise)*log(1+dTau));
    // n_noise = 2 * (2*NOISE_AVERAGING_N-2) = e.g. 24   <TWO QUADRATURES!!!>
    // Signal-to-noise ratio (dB) estimated over sliding window: o o o o o o o x x x o o o o o o o
    double dTar_dB = 10.0*log(m_dSNRc_rep)/log(10.0e0);
    QString qsTar_dB=QString("dB:%1").arg(dTar_dB,0,'f',0);
    QString qsTimeSec=QString("T:%1").arg(m_dTsRounded,0,'f',1);
    int iTarSize = m_uCandNum;
    QString qsTarSize=QString("Sz:%1").arg(iTarSize);
    QString qs_kDoppler=QString("kD:%1").arg(m_qpRepresentative.y());
    QString qsAzDeg, qsElDeg;
    if (m_bAnglesOk) {
        qsAzDeg=QString("Az:%1").arg(m_dAzimuthResult,0,'f',2);
        qsElDeg=QString("El:%1").arg(m_dElevationResult,0,'f',2);
    }
    else {
        qsAzDeg=QString("N/A");
        qsElDeg=QString("N/A");
    }
    QString qsTarDistKM=QString("D:%1").arg(1.0e-3*m_qpfWeighted.x(),0,'f',1);
    QString qsTarVelMpS=QString("V:%1").arg(m_qpfWeighted.y(),0,'f',0);
    QString qsSigType=QString("Ty:%1").arg(m_pPoi->m_iSigType);
    QString qsProbFA=QString("FA:%1").arg(m_pPoi->m_dFalseAlarmProb,0,'e',0);
    QString qsTarSumDB=QString("Su:%1").arg(m_d_y2mc_sum,0,'f',0);
    QString qsFltIdx=QString("Fl:%1").arg(m_sPriPtInfo.uFilterIndex);
    QString qsTarIDelay=QString("L:%1").arg(m_iDelay);
    int nFormularItems=m_pTargetsMap->m_qlFormularItems.size();
    for (int iItem=0; iItem<nFormularItems; iItem++) {
        struct QTargetsMap::sFormularItem *pItem = (struct QTargetsMap::sFormularItem *)&m_pTargetsMap->m_qlFormularItems.at(iItem);
        if (!pItem->bEnabled) continue;
        quint32 uRefCode = pItem->uRefCode;
        QString qsItemText;
        switch (uRefCode) {
            case QTARGETSMAP_FORMULAR_ITEM_STROBENO:
                qsItemText = qsStrobeNo; break;
            case QTARGETSMAP_FORMULAR_ITEM_TARMAXDB:
                qsItemText = qsTar_dB; break;
            case QTARGETSMAP_FORMULAR_ITEM_TIMESEC:
                qsItemText = qsTimeSec; break;
            case QTARGETSMAP_FORMULAR_ITEM_AZDEGREE:
                qsItemText = qsAzDeg; break;
            case QTARGETSMAP_FORMULAR_ITEM_ELDEGREE:
                qsItemText = qsElDeg; break;
            case QTARGETSMAP_FORMULAR_ITEM_TARSIZE:
                qsItemText = qsTarSize; break;
            case QTARGETSMAP_FORMULAR_ITEM_KDOPPLER:
                qsItemText = qs_kDoppler; break;
            case QTARGETSMAP_FORMULAR_ITEM_TARDISTKM:
                qsItemText = qsTarDistKM; break;
            case QTARGETSMAP_FORMULAR_ITEM_TARVELMPS:
                qsItemText = qsTarVelMpS; break;
            case QTARGETSMAP_FORMULAR_ITEM_SIGTYPE:
                qsItemText = qsSigType; break;
            case QTARGETSMAP_FORMULAR_ITEM_TARPROBFA:
                qsItemText = qsProbFA; break;
            case QTARGETSMAP_FORMULAR_ITEM_TARSUMDB:
                qsItemText = qsTarSumDB; break;
            case QTARGETSMAP_FORMULAR_ITEM_IDELAY :
                qsItemText = qsTarIDelay; break;
            case QTARGETSMAP_FORMULAR_ITEM_FLTIDX :
                qsItemText = qsFltIdx; break;
            default:
                continue;
        }
        if (!qsLegend.isEmpty()) qsLegend+="\n";
        qsLegend+=qsItemText;
    }
    if (qsLegend.isEmpty()) qsLegend=QString("N/A");
    m_pTargetsMap->addTargetMarker(m_sPriPtInfo);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::updateStatusInfo() {
    // Total number of statistical decisions ("if target signal exists in element (kDoppler,iDelay)")
    m_pPoi->uTotalElemDecisions+=m_pPoi->m_NFFT*m_pPoi->m_iFilteredN;
    // update information panel
    QDateTime dtCur = QDateTime::currentDateTime();
    if (m_pOwner->m_qtLastInfoUpdate.msecsTo(dtCur)>INFO_UPDATE_MSEC) {
        m_pOwner->m_qtLastInfoUpdate = dtCur;
        m_pOwner->m_plbStrobNo->setText(QString("Strob # %1").arg(m_uStrobeNo));
        double dNumFA = m_pPoi->m_dFalseAlarmProb*m_pPoi->uTotalElemDecisions;
        m_pOwner->m_plbFalseAlarmInfo->setText(QString("<FA>: %1").arg(dNumFA,0,'e',1));
        m_pOwner->m_plbDetectionsInfo->setText(m_pOwner->m_qsDetectionsInfo.arg((double)m_pPoi->uTotalDetections,0,'e',1));
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::phaseCoherenceTest() {
#ifdef PHASE_COHERENCE
//    double dSumRe = m_pBeamAmplRe[0] + m_pBeamAmplRe[1] + m_pBeamAmplRe[2] + m_pBeamAmplRe[3];
//    double dSumIm = m_pBeamAmplIm[0] + m_pBeamAmplIm[1] + m_pBeamAmplIm[2] + m_pBeamAmplIm[3];
//    double dSum2 = dSumRe*dSumRe + dSumIm*dSumIm;

//    double dSumAzRe = m_pBeamAmplRe[0] + m_pBeamAmplRe[2] - m_pBeamAmplRe[1] - m_pBeamAmplRe[3];
//    double dSumAzIm = m_pBeamAmplIm[0] + m_pBeamAmplIm[2] - m_pBeamAmplIm[1] - m_pBeamAmplIm[3];

//    double dSumElRe = m_pBeamAmplRe[2] + m_pBeamAmplRe[3] - m_pBeamAmplRe[0] - m_pBeamAmplRe[1];
//    double dSumElIm = m_pBeamAmplIm[2] + m_pBeamAmplIm[3] - m_pBeamAmplIm[0] - m_pBeamAmplIm[1];

//    double dRatioAz = dSumAzRe*dSumRe+dSumAzIm*dSumIm; dRatioAz/=dSum2;
//    double dRatioEl = dSumElRe*dSumRe+dSumElIm*dSumIm; dRatioEl/=dSum2;

    bool bOk0,bOk1,bOk2,bOk3,bOkAvr;
    double dPhi0=getArg(m_pBeamAmplRe[0],m_pBeamAmplIm[0],bOk0);
    double dPhi1=getArg(m_pBeamAmplRe[1],m_pBeamAmplIm[1],bOk1);
    double dPhi2=getArg(m_pBeamAmplRe[2],m_pBeamAmplIm[2],bOk2);
    double dPhi3=getArg(m_pBeamAmplRe[3],m_pBeamAmplIm[3],bOk3);
    double xC=cos(dPhi0)+cos(dPhi1)+cos(dPhi2)+cos(dPhi3);
    double yC=sin(dPhi0)+sin(dPhi1)+sin(dPhi2)+sin(dPhi3);
    double dPhiAvr=getArg(xC,yC,bOkAvr);
    bool bAllOk = bOk0 && bOk1 && bOk2 && bOk3 && bOkAvr;
    if (!bAllOk) qDebug() << "getArg() failed for strob: " << m_uStrobeNo;
    #ifndef AMPLITUDE_METHOD
    if (bAllOk) {
        dPhi0=180.0e0/dPI*(dPhi0-dPhiAvr);
        dPhi1=180.0e0/dPI*(dPhi1-dPhiAvr);
        dPhi2=180.0e0/dPI*(dPhi2-dPhiAvr);
        dPhi3=180.0e0/dPI*(dPhi3-dPhiAvr);
        dPhi0=dPhi0-360.0e0*qRound(dPhi0/360.0e0);
        dPhi1=dPhi1-360.0e0*qRound(dPhi1/360.0e0);
        dPhi2=dPhi2-360.0e0*qRound(dPhi2/360.0e0);
        dPhi3=dPhi3-360.0e0*qRound(dPhi3/360.0e0);
        m_tsPhaseCoherence << m_dTsRounded
            << "\t" << (int)dPhi0
            << "\t" << (int)dPhi1
            << "\t" << (int)dPhi2
            << "\t" << (int)dPhi3
            << endl;
                    // amplitude method
                    // << "\t" <<  dAmplRatioAz
                    // << "\t" <<  dAmplRatioEl
    }
    #endif
    #ifdef AMPLITUDE_METHOD
    // amplitude method
    double dA0,dA1,dA2,dA3;
    double dAmplRatioAz,dAmplRatioEl;
    {
        dA0=m_pBeamAmplRe[0]*m_pBeamAmplRe[0]+m_pBeamAmplIm[0]*m_pBeamAmplIm[0];
        dA0=sqrt(dA0);
        dA1=m_pBeamAmplRe[1]*m_pBeamAmplRe[1]+m_pBeamAmplIm[1]*m_pBeamAmplIm[1];
        dA1=sqrt(dA1);
        dA2=m_pBeamAmplRe[2]*m_pBeamAmplRe[2]+m_pBeamAmplIm[2]*m_pBeamAmplIm[2];
        dA2=sqrt(dA2);
        dA3=m_pBeamAmplRe[3]*m_pBeamAmplRe[3]+m_pBeamAmplIm[3]*m_pBeamAmplIm[3];
        dA3=sqrt(dA3);
        double dAsum = dA0 + dA1 + dA2 + dA3;
        // Azimuth
        dAmplRatioAz = (dA0 + dA2 - dA1 - dA3)/dAsum;
        // Elevation
        dAmplRatioEl = (dA2 + dA3 - dA0 - dA1)/dAsum;
    }
    double dFitting = 18.95 - 0.4071*m_dTsRounded;
    double dY2Max = pTarData->y2mc_rep;
    double dYAbsMax = sqrt(dY2Max);

    m_tsPhaseCoherence << m_dTsRounded
                     << "\t" << dElevationScan
                     << "\t" << dFitting
                     << "\t" << (dFitting - dElevationScan)
                     << "\t" << dAmplRatioEl
    //                                << "\t" << dA0
    //                                << "\t" << dA1
    //                                << "\t" << dA2
    //                                << "\t" << dA3
                     << "\t" << sqrt(dSum2)
                     << "\t" << dYAbsMax
                     << endl;
    #endif   // AMPLITUDE_MATHOD
#endif   // end of phase coherence ==================================================================
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::restart() {
    // clear the bStarted flag
    m_bStarted = false;
    // clear stale target markers
    m_pTargetsMap->clearMarkers();
    // stop active simulation, if any
    m_pOwner->toggleTimer(/* bool bStop=*/ true);
    // (redundant) security check
    if (!m_pPoi || !m_pSqlModel) {
        qDebug() << "QSimuMgr::restart(): !m_pPoi || !m_pSqlModel";
        return;
    }
    // reset FA counter
    m_pPoi->uTotalElemDecisions=0;
    m_pOwner->m_plbFalseAlarmInfo->setText(m_pOwner->m_qsFalseAlarmInfo.arg(0));
    // reset strob info
    m_pOwner->m_plbStrobNo->setText(QString("Stopped"));
    // reset detections counter
    m_pPoi->uTotalDetections=0;
    m_pOwner->m_plbDetectionsInfo->setText(m_pOwner->m_qsDetectionsInfo.arg(m_pPoi->uTotalDetections));

    // reconnect to DB
    if (!m_pSqlModel->isDBOpen()) {
        m_pOwner->m_plbStatusArea->setText(CONN_STATUS_DISCONN);
        if (m_pSqlModel->openDatabase()) {
            m_pOwner->m_plbStatusArea->setText(CONN_STATUS_SQLITE);
        }
        else {
            qDebug() << "Failed to open DB " << m_pSqlModel->getDBFileName();
        }
    }
    // exec main SELECT query
    bool bMainSelectQueryExecOk=false;
    if (m_pSqlModel->isDBOpen()) {
        // finish currently active query, if any
        if (m_pSqlModel->m_mainSelectQuery.isActive()) {
            m_pSqlModel->m_mainSelectQuery.finish();
        }
        if (m_pSqlModel->execMainSelectQuery()) {
            bMainSelectQueryExecOk = true;
        }
        else {
            qDebug() << "m_pSqlModel->execQuery() failed";
        }
    }
    if (bMainSelectQueryExecOk) {
        // restart timer with new interval (msec)
        // qDebug() << "Main query exec() ok. Starting simulation timer";
        m_pOwner->m_simulationTimer.start(m_pTargetsMap->m_uTimerMSecs);
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSimuMgr::filterMarkers() {
    QList <sVoiFilterInfo> qlFiltersInfo;
    m_pVoi->listFilters(qlFiltersInfo);
    QListIterator<sVoiFilterInfo> i(qlFiltersInfo);
    while (i.hasNext()) {
        sVoiFilterInfo sInfo = i.next();
        m_pTargetsMap->addTargetMarker(&sInfo);
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::onSimulationTimeout() {
    static bool bLocked=false;
    if (bLocked) return;
    bLocked = true;
    // qDebug() << "calling processStrob()";
    if (m_pSimuMgr) m_pSimuMgr->processStrob();
    bLocked = false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QIndicatorWindow::toggleTimer(bool bStop /* =false */) {
    bool bIsActive = m_simulationTimer.isActive();
    if (bIsActive && bStop) {
        m_simulationTimer.stop();
        return;
    }
    if (bIsActive) {
        m_simulationTimer.stop();
    }
    else {
        m_simulationTimer.start();
    }
}
