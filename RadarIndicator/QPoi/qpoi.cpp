#include "qpoi.h"
#include "qinisettings.h"
#include "qexceptiondialog.h"

#include "qchrprotosnc.h"
#include "qchrprotoacm.h"

#include "nr.h"
using namespace std;

const char *QPoi::m_pRejectorType[] = {
    "Нет",
    "Узкополосный",
    "ЧПК"
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QPoi::QPoi(QObject *parent)
          : QObject(parent)
          , m_dCarrierF(9500.0e0)
          , m_dVLight(299.792e0)
          , m_dVelCoef(0.0e0)
          , m_dDistCoef(0.0e0)
          , m_dTs(0.12e0)  // microseconds
          , m_Np(0)
          , m_Ntau(0)
          , m_NT(0)
          , m_NT_(0)
          , m_iBlank(0)
          , m_NFFT(0)
          , m_iBeamCountsNum (0)
          , m_iSigID_fromStrobeHeader(-1)
          , m_iSigType(0)
          , m_iSignalLen(0)
          , m_iSizeOfComplex(2*sizeof(qint16))
          , m_uTargSzThresh(0)
          , uTotalElemDecisions(0)
          , uTotalDetections(0)
          , m_uMaxTgSize(0)
          , m_dBeamDelta0(0)
          , m_dAntennaSzAz(0)
          , m_dAntennaSzEl(0)
          , m_iWeighting(0)
          , m_pPelengInv(NULL)
          , m_dDeltaBound(0)
          , m_dPelengBound(0)
          , m_dAzLOverD(0)
          , m_dElLOverD(0)
          , m_dPelengIncr(0) {
    QIniSettings &iniSettings = QIniSettings::getInstance();
    QIniSettings::STATUS_CODES scRes;
    iniSettings.setDefault(QPOI_CARRIER_FREQUENCY,9500.0e0);
    m_dCarrierF = iniSettings.value(QPOI_CARRIER_FREQUENCY,scRes).toDouble();
    iniSettings.setDefault(QPOI_SAMPLING_TIME_USEC,0.120e0);
    m_dTs = iniSettings.value(QPOI_SAMPLING_TIME_USEC,scRes).toDouble();
    iniSettings.setDefault(QPOI_PFALARM,1.0e-8);
    m_dFalseAlarmProb = iniSettings.value(QPOI_PFALARM,scRes).toDouble();
    iniSettings.setDefault(QPOI_USE_NOISEMAP,true);
    m_bUseNoiseMap = iniSettings.value(QPOI_USE_NOISEMAP,scRes).toBool();
    iniSettings.setDefault(QPOI_USE_LOG,true);
    m_bUseLog = iniSettings.value(QPOI_USE_LOG,scRes).toBool();
    iniSettings.setDefault(QPOI_TARGET_SZTHRESH,5);
    m_uTargSzThresh = iniSettings.value(QPOI_TARGET_SZTHRESH,scRes).toInt();
    iniSettings.setDefault(QPOI_TARGET_MAXTGSIZE,20);
    m_uMaxTgSize = iniSettings.value(QPOI_TARGET_MAXTGSIZE,scRes).toInt();
    iniSettings.setDefault(QPOI_BEAM_OFFSET_D0,0.487e0);
    m_dBeamDelta0 = iniSettings.value(QPOI_BEAM_OFFSET_D0,scRes).toDouble();
    iniSettings.setDefault(QPOI_ANTENNA_SIZE_AZ,0.6e0);
    m_dAntennaSzAz = iniSettings.value(QPOI_ANTENNA_SIZE_AZ,scRes).toDouble();
    iniSettings.setDefault(QPOI_ANTENNA_SIZE_EL,0.9e0);
    m_dAntennaSzEl = iniSettings.value(QPOI_ANTENNA_SIZE_EL,scRes).toDouble();
    iniSettings.setDefault(QPOI_ANTENNA_WEIGHTING,QPOI_WEIGHTING_RCOSINE_P065);
    m_iWeighting = iniSettings.value(QPOI_ANTENNA_WEIGHTING,scRes).toUInt();
    iniSettings.setDefault(QPOI_REJECTOR_TYPE,QPOI_REJECTOR_NONE);
    m_iRejectorType = iniSettings.value(QPOI_REJECTOR_TYPE,scRes).toUInt();
    // set name of simulation log file
    m_qsSimuLogFName = QDir::current().absolutePath()+"/simulog.txt";
    QFile qfSimuLog(m_qsSimuLogFName);
    qfSimuLog.resize(0);

    // initialize random numbers
    initializeRandomNumbers();
    // initialize inverse pelengation curve
    initPeleng();
}
//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QPoi::~QPoi() {
    QIniSettings &iniSettings = QIniSettings::getInstance();
    iniSettings.setNum(QPOI_CARRIER_FREQUENCY, m_dCarrierF);
    iniSettings.setNum(QPOI_SAMPLING_TIME_USEC, m_dTs);
    iniSettings.setNum(QPOI_PFALARM, m_dFalseAlarmProb);
    iniSettings.setValue(QPOI_USE_NOISEMAP, m_bUseNoiseMap);
    iniSettings.setValue(QPOI_USE_LOG, m_bUseLog);
    iniSettings.setNum(QPOI_TARGET_SZTHRESH, m_uTargSzThresh);
    iniSettings.setNum(QPOI_TARGET_MAXTGSIZE, m_uMaxTgSize);
    iniSettings.setNum(QPOI_BEAM_OFFSET_D0, m_dBeamDelta0);
    iniSettings.setNum(QPOI_ANTENNA_SIZE_AZ, m_dAntennaSzAz);
    iniSettings.setNum(QPOI_ANTENNA_SIZE_EL, m_dAntennaSzEl);
    iniSettings.setNum(QPOI_ANTENNA_WEIGHTING, m_iWeighting);
    iniSettings.setNum(QPOI_REJECTOR_TYPE, m_iRejectorType);

    if (m_pPelengInv) delete m_pPelengInv;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QPoi::addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    QTabWidget *pTabWidget = qobject_cast<QTabWidget *> (pPropTabs);

    QDoubleValidator *pdv;
    QLocale locale(QLocale::English);

    QWidget *pWidget=new QWidget;
    QVBoxLayout *pVLayout=new QVBoxLayout;

    QGridLayout *pGLayout=new QGridLayout;
    pGLayout->addWidget(new QLabel("Carrier frequency (MHz)"),0,0);
    pPropPages->m_pleFCarrier = new QLineEdit(QString::number(m_dCarrierF,'f',0));
    pPropPages->m_pleFCarrier->setMaximumWidth(PROP_LINE_WIDTH);
    pGLayout->addWidget(pPropPages->m_pleFCarrier,0,1);

    pGLayout->addWidget(new QLabel("Sampling time (microsec)"),0,3);
    pPropPages->m_pleTSampl = new QLineEdit(QString::number(m_dTs,'f',3));
    pPropPages->m_pleTSampl->setMaximumWidth(PROP_LINE_WIDTH);
    pGLayout->addWidget(pPropPages->m_pleTSampl,0,4);

    pGLayout->addWidget(new QLabel("Target size threshold"),1,0);
    pPropPages->m_pleTargSzThresh = new QLineEdit(QString::number(m_uTargSzThresh));
    pPropPages->m_pleTargSzThresh->setMaximumWidth(PROP_LINE_WIDTH);
    pPropPages->m_pleTargSzThresh->setValidator(new QIntValidator(0,100));
    pGLayout->addWidget(pPropPages->m_pleTargSzThresh,1,1);

    pGLayout->addWidget(new QLabel("Max target size"),1,3);
    pPropPages->m_pleMaxTgSize = new QLineEdit(QString::number(m_uMaxTgSize));
    pPropPages->m_pleMaxTgSize->setMaximumWidth(PROP_LINE_WIDTH);
    pPropPages->m_pleMaxTgSize->setValidator(new QIntValidator(0,100));
    pGLayout->addWidget(pPropPages->m_pleMaxTgSize,1,4);

    pGLayout->addWidget(new QLabel("Antenna size Az (m)"),2,0);
    pPropPages->m_pleAntennaSzAz = new QLineEdit(QString::number(m_dAntennaSzAz));
    pdv = new QDoubleValidator(0,10,2); pdv->setLocale(locale);
    pPropPages->m_pleAntennaSzAz->setValidator(pdv);
    pPropPages->m_pleAntennaSzAz->setMaximumWidth(PROP_LINE_WIDTH);
    pGLayout->addWidget(pPropPages->m_pleAntennaSzAz,2,1);

    pGLayout->addWidget(new QLabel("Antenna size El (m)"),2,3);
    pPropPages->m_pleAntennaSzEl = new QLineEdit(QString::number(m_dAntennaSzEl));
    pPropPages->m_pleAntennaSzEl->setMaximumWidth(PROP_LINE_WIDTH);
    pdv = new QDoubleValidator(0,10,2); pdv->setLocale(locale);
    pPropPages->m_pleAntennaSzEl->setValidator(pdv);
    pGLayout->addWidget(pPropPages->m_pleAntennaSzEl,2,4);

    pGLayout->addWidget(new QLabel("Beam relative offset"),3,0);
    pPropPages->m_pleBeamOffsetD0 = new QLineEdit(QString::number(m_dBeamDelta0));
    pPropPages->m_pleBeamOffsetD0->setMaximumWidth(PROP_LINE_WIDTH);
    pdv = new QDoubleValidator(0,1,3); pdv->setLocale(locale);
    pPropPages->m_pleBeamOffsetD0->setValidator(pdv);
    pGLayout->addWidget(pPropPages->m_pleBeamOffsetD0,3,1);

    pGLayout->addWidget(new QLabel("Antenna weighting"),3,3);
    pPropPages->m_pcbbWeighting=new QComboBox;
    pPropPages->m_pcbbWeighting->addItem(m_pWeightingType[QPOI_WEIGHTING_RCOSINE_P065]);
    pPropPages->m_pcbbWeighting->setCurrentIndex(m_iWeighting);
    pGLayout->addWidget(pPropPages->m_pcbbWeighting,3,4);

    pGLayout->addWidget(new QLabel("Interference rejection"),4,0);
    pPropPages->m_pcbbRejector=new QComboBox;
    for (int i=0; i<sizeof(m_pRejectorType)/sizeof(m_pRejectorType[0]); i++) {
        pPropPages->m_pcbbRejector->addItem(QString::fromUtf8(m_pRejectorType[i]));
    }
    pPropPages->m_pcbbRejector->setCurrentIndex(m_iRejectorType);
    pGLayout->addWidget(pPropPages->m_pcbbRejector,4,1);

    pGLayout->addWidget(new QLabel("Noise estimation"),4,3);
    pPropPages->m_pcbUseNoiseMap=new QCheckBox("use noise map");
    pGLayout->addWidget(pPropPages->m_pcbUseNoiseMap,4,4);
    pPropPages->m_pcbUseNoiseMap->setChecked(m_bUseNoiseMap);

    pGLayout->addWidget(new QLabel("False alarm probability"),5,0);
    pPropPages->m_pleFalseAlarmP = new QLineEdit(QString::number(m_dFalseAlarmProb,'e',1));
    pdv = new QDoubleValidator(0,1,2); pdv->setLocale(locale);
    pPropPages->m_pleFalseAlarmP->setValidator(pdv);
    pPropPages->m_pleFalseAlarmP->setMaximumWidth(PROP_LINE_WIDTH);
    pGLayout->addWidget(pPropPages->m_pleFalseAlarmP,5,1);
    QObject::connect(pPropPages->m_pleFalseAlarmP,SIGNAL(editingFinished()),pPropPages,SIGNAL(falseAlarmProbabilitySet()));
    QObject::connect(pPropPages,SIGNAL(falseAlarmProbabilitySet()),SLOT(onFalseAlarmProbabilitySet()));

    pGLayout->addWidget(new QLabel("Detection threshold (dB)"),5,3);
    pPropPages->m_pleDetectionThreshold = new QLineEdit(QString::number(getDetectionThreshold_dB(m_dFalseAlarmProb)));
    pPropPages->m_pleDetectionThreshold->setReadOnly(true);
    pPropPages->m_pleDetectionThreshold->setMaximumWidth(PROP_LINE_WIDTH);
    pGLayout->addWidget(pPropPages->m_pleDetectionThreshold,5,4);

    pGLayout->setColumnStretch(2,100);
    pGLayout->setColumnStretch(5,100);
    pVLayout->addLayout(pGLayout);

    pVLayout->addStretch();
    pWidget->setLayout(pVLayout);
    pTabWidget->insertTab(iIdx,pWidget,QPOI_PROP_TAB_CAPTION);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QPoi::propChanged(QObject *pPropDlg) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    bool bOk;
    QString qs;

    qs = pPropPages->m_pleFCarrier->text();
    double dCarrierF = qs.toDouble(&bOk);
    if (bOk) m_dCarrierF = qBound(5000.0e0,dCarrierF,15000.0e0);

    qs = pPropPages->m_pleTSampl->text();
    double dTs = qs.toDouble(&bOk);
    if (bOk) m_dTs = qBound(0.01e0,dTs,10.0e0); // microseconds

    qs = pPropPages->m_pleTargSzThresh->text();
    quint32 uTargSzThresh = qs.toUInt(&bOk);
    if (bOk) m_uTargSzThresh = qMax((quint32)0,uTargSzThresh);

    qs = pPropPages->m_pleBeamOffsetD0->text();
    double dBeamDelta0 = qs.toDouble(&bOk);
    if (bOk) m_dBeamDelta0 = qMax(0.0e0,dBeamDelta0);

    qs = pPropPages->m_pleAntennaSzAz->text();
    double dAntennaSzAz = qs.toDouble(&bOk);
    if (bOk) m_dAntennaSzAz = qMax(0.0e0,dAntennaSzAz);

    qs = pPropPages->m_pleAntennaSzEl->text();
    double dAntennaSzEl = qs.toDouble(&bOk);
    if (bOk) m_dAntennaSzEl = qMax(0.0e0,dAntennaSzEl);

    m_iWeighting = pPropPages->m_pcbbWeighting->currentIndex();

    qs = pPropPages->m_pleMaxTgSize->text();
    quint32 uMaxTgSize = qs.toUInt(&bOk);
    if (bOk) m_uMaxTgSize = qMax((quint32)0,uMaxTgSize);

    m_iRejectorType = pPropPages->m_pcbbRejector->currentIndex();

    qs = pPropPages->m_pleFalseAlarmP->text();
    double dFalseAlarmP = qs.toDouble(&bOk);
    if (bOk) m_dFalseAlarmProb = qMax(0.0e0,dFalseAlarmP);

    m_bUseNoiseMap = pPropPages->m_pcbUseNoiseMap->isChecked();
}
//====================================================================
//
//====================================================================
void QPoi::avevar_poi(Vec_I_DP &data, DP &ave, DP &var) {
    DP s;
    int j;

    int n=data.size();
    ave=0.0;
    var=0.0;
    for (j=0;j<n;j++) {
        s=data[j]-ave;
        var += s*s;
    }
    var=var/n;
}
//======================================================================================================
//
//======================================================================================================
bool QPoi::updateStrobeParams(QByteArray &baStructStrobeData) {
    ACM::STROBE_DATA *pStructStrobeData = (ACM::STROBE_DATA*)baStructStrobeData.data();
    CHR::STROBE_HEADER *pStructStrobeHeader = (CHR::STROBE_HEADER*)(&pStructStrobeData->header);
    // copy params from pStructStrobeData and pStructStrobeHeader
    if (pStructStrobeData->beamCountsNum != pStructStrobeHeader->distance * pStructStrobeHeader->pCount) {
        return false;
    }
    this->m_Np   = pStructStrobeHeader->pCount;         // количество импульсов в стробе (число периодов = 1024)
    this->m_NT   = pStructStrobeHeader->pPeriod;        // Полное число выборок в одном периоде (повторения импульсов = 200)
    this->m_NT_  = pStructStrobeHeader->distance;       // Дистанция приема (число выборок, регистрируемых в одном периоде = 80)
    this->m_iBlank = pStructStrobeHeader->blank;        // Бланк (число выборок, пропускаемое до начала приёма в каждом периоде)
    this->m_Ntau = pStructStrobeHeader->pDuration;      // Длительность импульса (число выборок в одном импульсе = 8)
    this->m_iFilteredN = pStructStrobeHeader->distance
                 + pStructStrobeHeader->pDuration-1;    // число выборок после согласованного фильра: NT_+Ntau-1
    this->m_iBeamCountsNum = pStructStrobeData->beamCountsNum;
    this->m_iSigID_fromStrobeHeader = pStructStrobeHeader->signalID; // Тип сигнала

    // check parameters of transmitted signal
    if (this->m_iSigType != this->m_iSigID_fromStrobeHeader) {
        throw RmoException(QString("m_iSigType(%1) != m_iSigID_fromStrobeHeader(%2)")
                           .arg(m_iSigType).arg(m_iSigID_fromStrobeHeader));
        return false;
    }
    if (this->m_iSignalLen != this->m_Ntau) {
        throw RmoException("this->m_Ntau != this->m_iSignalLen");
        return false;
    }
    if (m_baSignalSamplData.size() != 2*this->m_iSignalLen*sizeof(qint16)) {
        throw RmoException("m_baSignalSamplData.size() != 2*this->m_iSignalLen*sizeof(qint16)");
        return false;
    }
    return true;
}
//======================================================================================================
//
//======================================================================================================
void QPoi::updatePhyScales() {
    // phy scales
    double dDoppFmin;                                // This is smallest Doppler frequency (MHz) possible for this sampling interval dTs
    dDoppFmin=1.0e0/(m_NT*m_dTs)/m_NFFT;             // This is smallest Doppler frequency (MHz) possible for this sampling interval dTs
    double dVelIncr = 1.0e6*m_dVLight/2/m_dCarrierF; // Increment of target velocity (m/s) per 1 MHz of Doppler shift
    m_dVelCoef = dVelIncr*dDoppFmin;                 // m/s per frequency count (the "smallest" Doppler frequency possible)
    m_dDistCoef = m_dTs*m_dVLight/2;                 // m per sample (target distance increment per sampling interval dTs)
}
//======================================================================================================
//
//======================================================================================================
bool QPoi::checkStrobeParams(QByteArray &baStructStrobeData) {
    ACM::STROBE_DATA *pStructStrobeData = (ACM::STROBE_DATA*)baStructStrobeData.data();
    CHR::STROBE_HEADER *pStructStrobeHeader = (CHR::STROBE_HEADER*)(&pStructStrobeData->header);
    // compare fields of pStructStrobeData and pStructStrobeHeader
    bool bRetVal = true;
    bRetVal = bRetVal && (this->m_Np   == pStructStrobeHeader->pCount);        // количество импульсов в стробе (число периодов = 1024)
    bRetVal = bRetVal && (this->m_NT   == pStructStrobeHeader->pPeriod);       // Полное число выборок в одном периоде (повторения импульсов = 200)
    bRetVal = bRetVal && (this->m_NT_  == pStructStrobeHeader->distance);      // Дистанция приема (число выборок, регистрируемых в одном периоде = 80)
    bRetVal = bRetVal && (this->m_iBlank == pStructStrobeHeader->blank);       // Бланк (число выборок, пропускаемое до начала приёма в каждом периоде)
    bRetVal = bRetVal && (this->m_Ntau == pStructStrobeHeader->pDuration);     // Длительность импульса (число выборок в одном импульсе = 8)
    bRetVal = bRetVal && (this->m_iBeamCountsNum == pStructStrobeData->beamCountsNum); // Полное число регистрируемых выборок в стробе
    bRetVal = bRetVal && (this->m_iSigID_fromStrobeHeader == pStructStrobeHeader->signalID); // Тип сигнала
    bRetVal = bRetVal && (this->m_iSignalLen == pStructStrobeHeader->pDuration); // Длина реализации сигнала
    bRetVal = bRetVal && (this->m_baSignalSamplData.size() == 2*pStructStrobeHeader->pDuration*sizeof(qint16)); // Данные реализации

    // if all parameters match return true. Otherwise return false
    return bRetVal;
}
//====================================================================
//
//====================================================================
void QPoi::appendSimuLog() {
    QFile qfSimuLog(m_qsSimuLogFName);
    qint64 iFileSize = qfSimuLog.size();
    if (!qfSimuLog.open(QIODevice::Append)) {
        QExceptionDialog *pDlg = new QExceptionDialog("QPoi::updateStrobeParams(): Failed to open log "+qfSimuLog.fileName());
        pDlg -> setAttribute(Qt::WA_DeleteOnClose);
        pDlg -> open();
        return;
    }
    QTextStream tsSimuLog(&qfSimuLog);
    if (!iFileSize) tsSimuLog << QString("=====================================================================") << endl;
    tsSimuLog << QString("Параметры строба во время моделирования: ") << QDateTime::currentDateTime().toString("dddd, dd MMMM yyyy, hh:mm:ss") << endl;
    tsSimuLog << QString("количество импульсов в стробе (число периодов): ") << this->m_Np << endl;
    tsSimuLog << QString("Длительность импульса (число выборок в одном импульсе): ") << this->m_Ntau << endl;
    tsSimuLog << QString("Полное число выборок в одном периоде (повторения импульсов): ") << this->m_NT << endl;
    tsSimuLog << QString("Дистанция приема (число выборок, регистрируемых в одном периоде): ") << this->m_NT_ << endl;
    tsSimuLog << QString("Бланк (число выборок, пропускаемых до начала приёма): ") << this->m_iBlank << endl;
    tsSimuLog << QString("число выборок после согласованного фильра: ") << this->m_iFilteredN << endl;
    tsSimuLog << QString("Полное регистрируемое число выборок в одном стробе: ") << this->m_iBeamCountsNum << endl;
    tsSimuLog << QString("Тип сигнала: ") << this->m_iSigID_fromStrobeHeader << endl;
    tsSimuLog << QString("Время выборки (мкс): ") << this->m_dTs << endl;
    tsSimuLog << QString("Несущая частота (МГц): ") << this->m_dCarrierF << endl;
    tsSimuLog << endl;

    // close simulation file
    qfSimuLog.close();
}
//====================================================================
//
//====================================================================
void QPoi::onFalseAlarmProbabilitySet() {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (QObject::sender());
    bool bOk;
    double dProb = pPropPages->m_pleFalseAlarmP->text().toDouble(&bOk);
    if (bOk) {
        pPropPages->m_pleDetectionThreshold->setText(QString::number(getDetectionThreshold_dB(dProb)));
    }
}
//====================================================================
//
//====================================================================
qint32 QPoi::getDetectionThreshold_dB(double dProb) {
    // Weinberg 2017 parameter
    // SNR-related parameter: dTau = dFrac/(0.5*n_noise);
    // false alarm probability: dProb = exp(-(0.5*n_noise)*log(1+dTau));
    // Sliding window size: n_noise = 2*(2*NOISE_AVERAGING_N-2)  <TWO QUADRATURES!!!>
    if (dProb < 1.0e-99 || dProb-1.0e0 > 1.0e-10) {
        throw RmoException("QPoi::getDetectionThreshod_dB: dProb < 1.0e-99 || dProb-1.0e0 > 1.0e-10");
        return 999;
    }
    double n_noise = 2*(2*NOISE_AVERAGING_N-2);
    double dTau = exp(-2.0e0*log(dProb)/n_noise) - 1.0e0;
    double dFrac = 0.5e0*n_noise*dTau;
    if (dFrac < 1.0e-99) {
        throw RmoException("QPoi::getDetectionThreshod_dB: dFrac < 1.0e-99");
        return 999;
    }
    return qRound(10.0e0*log(dFrac)/log(10.0e0));
}
//====================================================================
//
//====================================================================
void QPoi::initializeRandomNumbers() {
    // initialize seed of stdlib.h pseudo-random numbers (iSeed is read-only)
    unsigned int iSeed=1;
    std::srand(iSeed);

    // initialize seed of Numerical Recipes Gaussian random numbers (m_idum is read-write)
    this->m_idum=-1;
    NR::gasdev(this->m_idum);
}
//====================================================================
//
//====================================================================
double QPoi::getGaussDev() {
    return NR::gasdev(this->m_idum);
}
//====================================================================
//
//====================================================================
//int QPoi::exptDetection() {
//    Vec_DP dSignal(2);
//    dSignal[0]=getGaussDev();
//    dSignal[1]=getGaussDev();
//    Vec_DP dNoise((2*NOISE_AVERAGING_N-2)*2);
//    int idxNoise=0;
//    for (int iShift=-NOISE_AVERAGING_N; iShift<=NOISE_AVERAGING_N; iShift++) {
//        if (iShift==-1 || iShift==0 || iShift==1) continue;
//        if (idxNoise>dNoise.size()-2) {
//            throw RmoException("QPoi: idxNoise>dNoise.size()-2");
//        }
//        dNoise[idxNoise++]=getGaussDev();
//        dNoise[idxNoise++]=getGaussDev();
//    }
//    int n_noise=dNoise.size();
//    int n_signal=dSignal.size();
//    if (n_signal!=2 || n_noise!=idxNoise) {
//        throw RmoException("QPoi: n_signal!=2 || n_noise!=idxNoise");
//    }
//    DP ave_noise,ave_signal;
//    DP var_noise,var_signal;
//    avevar_poi(dNoise,ave_noise,var_noise);      // var_noise  = SUM_i [(Re Noise_i)^2  + (Im Noise_i)^2 ] / (2*(2*NOISE_AVERAGING_N-2))
//    avevar_poi(dSignal,ave_signal,var_signal);   // var_signal =       [(Re Signal_i)^2 + (Im Signal_i)^2] / 2
//    DP dFrac,dProb;
//    dFrac=var_signal/var_noise; // signal-to-noise
//    // Weinberg 2017 parameter
//    double dTau = dFrac/(0.5*n_noise);
//    // false alarm probability
//    dProb = exp(-(0.5*n_noise)*log(1+dTau));
//    // detection
//    if (dProb<m_dFalseAlarmProb) return 1;
//    // no detection
//    return 0;
//}
