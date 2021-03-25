#ifndef QPOI_H
#define QPOI_H

#include <QObject>
#include <QVector>
#include "qproppages.h"

#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include "nr.h"
using namespace std;

#define QPOI_PROP_TAB_CAPTION    "POI"

#define QPOI_SAMPLING_TIME_USEC             "TSamplingMicroSec"
#define QPOI_CARRIER_FREQUENCY              "CarrierFreqMHz"
// signal detection threshold
#define QPOI_THRESHOLD                      "threshold"
// false alarm probability
#define QPOI_PFALARM                        "falseAlarmProb"
// use noise scaling in doppler domain
#define QPOI_USE_NOISEMAP                   "useNoiseMap"
// output simulation log
#define QPOI_USE_LOG                        "simulationLog"
// detection threshold on target size
#define QPOI_TARGET_SZTHRESH                "tarSizeThresh"
// limitation on linear size of target
#define QPOI_TARGET_MAXTGSIZE               "maxTargetSize"
// beam relative offset Delta_0 from scan line in each direction (see manuscript for details)
#define QPOI_BEAM_OFFSET_D0                 "beamOffsetD0"
// antenna size along Az dir in meters
#define QPOI_ANTENNA_SIZE_AZ                "antennaSizeAz"
// antenna size along El dir in meters
#define QPOI_ANTENNA_SIZE_EL                "antennaSizeEl"
// antenna weighting type
#define QPOI_ANTENNA_WEIGHTING              "antennaWeight"
// interference rejection algorithm
#define QPOI_REJECTOR_TYPE                  "interferenceRejectorType"

#define QPOI_WEIGHTING_RCOSINE_P065         0

#define QPOI_REJECTOR_NONE                  0
#define QPOI_REJECTOR_NARROW_BAND           1
#define QPOI_REJECTOR_CHE_PE_KA             2

#define QPOI_NUMBER_OF_BEAMS                4
#define QPOI_MAXIMUM_TG_MISMATCH            10

#define GENERATE_PROGRESS_BAR_STEP          1
#define GENERATE_PROGRESS_BAR_MAX           100

// Noise averaging in sliding window:
//           (2*NOISE_AVERAGING_N-2) complex samples for noise;
//           1 complex sample for signal
#define NOISE_AVERAGING_N          7

class QNoiseMap;

class QPoi : public QObject {
    Q_OBJECT
public:
    explicit QPoi(QObject *parent = 0);
    ~QPoi();

    Q_INVOKABLE void addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx);
    Q_INVOKABLE void propChanged(QObject *pPropDlg);

    bool detectTargets(QByteArray &baSamplesDP, QByteArray &baStrTargets, int &nTargets);

    struct sTarget {
        unsigned int uCandNum;   // number of candidates attributed to this target
        QPointF      qpf_wei;    // (dimensional) mass center for target delay index l, Doppler index k (0,...,m_NFFT)
                                 // weighted over candidates (with y2mc) and multiplied by m_dDistCoef, m_dVelCoef
        double       y2mc_sum;   // total energy of target
        QPoint       qp_rep;     // representative candidate resolution element (lc_rep, kc_rep)
        double       y2mc_rep;   // energy for representative candidate
        double       SNRc_rep;   // signal-to-noise ratio for representative candidate
    };

signals:

public slots:
    void onFalseAlarmProbabilitySet();

public:
    void avevar_poi(Vec_I_DP &data, DP &ave, DP &var);
    // void ftest_poi(Vec_I_DP &data_noise, Vec_I_DP &data_signal, DP &f, DP &prob);
    QByteArray dopplerRepresentation(QByteArray &baSamplesDP);
    bool matchedFilter(QByteArray &baSamplesDP, int iDelay, int jPeriod, double &dRe, double &dIm);
    QByteArray getHammingWindow();
    bool getPointDopplerRep(int iDelay, int kDoppler,
        QByteArray pbaBeamDataDP[QPOI_NUMBER_OF_BEAMS],
        double dBeamAmplRe[QPOI_NUMBER_OF_BEAMS],
        double dBeamAmplIm[QPOI_NUMBER_OF_BEAMS]);
    bool updateNFFT();
    void updatePhyScales();
    bool checkStrobeParams(QByteArray &baStructStrobeData);
    bool updateStrobeParams(QByteArray &baStructStrobeData);
    bool getAngles(double dBeamAmplRe[4], double dBeamAmplIm[4], double &dAzimuth, double &dElevation);
    double dsinc(double x);
    double raised_cos_spec(double Delta, double p=0.65);
    double raised_cos_charact(double Delta);
    double dichotomy(double dFuncValue, double dLeft, double dRight, double (QPoi::*pFunc)(double), bool *pbOk = NULL);
    void initPelengInv(double (QPoi::*pFunc)(double));
    void initPeleng();
    double getRelAngOffset(double dPelengRatio, bool *pbOk = NULL);
    void appendSimuLog();
    qint32 getDetectionThreshold_dB(double dProb);
    void initializeRandomNumbers();
    double getGaussDev();
    // int exptDetection();

public:
    double m_dCarrierF;  // Irradiation carrier frequency (MHz)
    double m_dTs;        // sampling time interval (microsecs)
    int m_NT_;    // m_NT_ - recorded data samples per period, typically at 20191016 this is 80 samples
    int m_iBlank; // m_iBlank - blank, or number of samples skipped before reciever turns on at the beginning of each period
    int m_NT;     // m_NT - number of samples per period, , typically at 20191016 this 200 samples
    int m_Np;     // m_Np - number of periods, typically at 20191016 this is 1024 periods
    int m_Ntau;   // m_Ntau - pulse duration (samples), typically at 20191016 this is 8 samples
    int m_iFilteredN; // in-place array size at filter output: m_iFilteredN=m_NT_-m_Ntau+1; m_Ntau -pulse duration (samples)
                    // 20200502: changed to m_iFilteredN=m_NT_
                    // 20210216: changed to m_iFilteredN=m_NT_+m_Ntau-1;
    int m_NFFT;   // the least power of 2 to cover m_Np, typically at 20191016 this is 1024 periods
    int m_iBeamCountsNum; // number of complex samples per beam = m_NT_*m_Np
    int m_iSigID_fromStrobeHeader; // type of transmitted signal
    double m_dFalseAlarmProb; // probability of false alarm
    bool m_bUseLog;
    QByteArray m_baStructFileHdr;
    QByteArray m_baStructStrobeData;
    quint32 m_uFileVer;
    quint32 m_uStrobeNoForDebugging;

    int m_iSizeOfComplex;

    quint32 m_uTargSzThresh;
    quint32 m_uMaxTgSize;

    // Peleng-related
    double m_dBeamDelta0;
    double m_dAntennaSzAz;
    double m_dAntennaSzEl;
    int m_iWeighting;
    static const char *m_pWeightingType[];
    static const char *m_pRejectorType[];
    double m_dAzLOverD;
    double m_dElLOverD;
    double *m_pPelengInv;
    double m_dDeltaBound;
    double m_dPelengBound;
    double m_dPelengIncr;
    int m_iRejectorType;
    bool m_bUseNoiseMap;
    quint64 uTotalElemDecisions;
    quint64 uTotalDetections;

    // phy scales
    const double m_dVLight; // m/usec
    double m_dVelCoef;      // Increment of target velocity (m/s) per 1 MHz of Doppler shift
                            // m/s per frequency count (the "smallest" Doppler frequency possible)
    double m_dDistCoef;     // m per sample (target distance increment per sampling interval dTs)

    // Transmitted signal time sampling (as of DM table signals)
    QByteArray m_baSignalSamplData;
    int m_iSigType;
    int m_iSignalLen; // length of transmitted signal

    // noise map
    QByteArray m_baNoiseMap;

    // simulation log file name
    QString m_qsSimuLogFName;

    // random seed for NR::ran1()
    int m_idum;

    // skip Doppler channels
    QMultiMap<int,int> m_qmmSkipDoppler;
};

#endif // QPOI_H
