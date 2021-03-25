#include "qregfileparser.h"
#include "qpulsesamplingdlg.h"
#include "qindicatorwindow.h"
#include "qexceptiondialog.h"
#include "qnoisemap.h"

#include <windows.h>
#include <stdlib.h>

#define MAX_GUI_BLOCKING_TIME_MSECS 100
#define MAX_PROCESS_EVENTS_TIME_MSECS 500
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QRegFileParser::parseRegDataFile(QObject *pSender) {
    // get access to QPropPages dynamic object (!take care - only exists while dlg)
    QWidget *parent = qobject_cast<QWidget *> (pSender);
    // for profiling
    // quint64 uLastT;
    // qDebug() << "Parsing file " << m_pRegFileParser->m_qfRegFile.fileName();
    QString qsFilePath   = m_fiRegFile.absoluteFilePath();
    // quint32 nRec         = m_pRegFileParser->m_sFileHdr.nRec;
    quint32 nRec         = 0;  // at first, we do not know number of recrds with type ACM_TYPE::STROBE_DATA
    qint64 uTimeStamp    = m_uTimeStamp;
    quint32 uFileVersion = m_uFileVersion;
    static qint64 iLastProcessEventsTime = 0;
    // uLastT = QDateTime::currentMSecsSinceEpoch();
    // start transaction for whole file
    m_pSqlModel->startTransaction();
    qint64 iFileId=m_pSqlModel->addFileRec(uTimeStamp,qsFilePath,nRec,QString::number(uFileVersion,16));
    // m_uDB +=QDateTime::currentMSecsSinceEpoch()-uLastT;
    if (iFileId==-1) {
        throw RmoException(QString("addFileRec failed: %1 (nRec=%2, uFileVer=%3)").arg(qsFilePath).arg(nRec).arg(uFileVersion));
        return;
    }
    // qDebug() << "Added file record: " << qsFilePath;
    bool bReset=true;
    if (m_pParseMgr) m_pParseMgr->doUpdateParseProgressBar(bReset);
    while(!isAtEnd()) {
        if (m_pOwner->m_bShutDown) return;
        // uLastT = QDateTime::currentMSecsSinceEpoch();
        QByteArray *pbaStrobe = getStrobe();
        // m_uHDD +=QDateTime::currentMSecsSinceEpoch()-uLastT;
        if (pbaStrobe==NULL || m_pDataHeader==NULL) {
            throw RmoException(QString("Could not get strob info from file ")+m_qfRegFile.fileName());
            return;
        }
        // codogram for defining angle offset for four beams along azimuth ans elevation
        if (m_pDataHeader->dwType == SNC_TYPE::SET_ROSE_DELTA) {
            if (m_pDataHeader->dwSize == sizeof(ACM::ROSE_DELTA_SETTINGS)) {
                ACM::ROSE_DELTA_SETTINGS *pRoseDeltaSettings = (ACM::ROSE_DELTA_SETTINGS *)pbaStrobe->data();
                //---- struct ACM::ROSE_DELTA_SETTINGS --------------------------------------------------------------------------------------------------
                // int16_t azimuth;      // ЦМР 180 градусов / 32768, как и в указании углов отклонения луча.
                // int16_t elevation;    // значение указывает отклонение 1 луча от нормали к плоскости антенны, то есть между лучами удвоенное значение.
                //---------------------------------------------------------------------------------------------------------------------------------------
                double dBeamOffsetAzimuth   = pRoseDeltaSettings->azimuth*180.0e0/32768;
                double dBeamOffsetElevation = pRoseDeltaSettings->elevation*180.0e0/32768;
                qDebug() << "pRoseDeltaSettings: azimuth=" << pRoseDeltaSettings->azimuth << " elevation=" << pRoseDeltaSettings->elevation;
                qDebug() << "Beam rose delta (az)=" << dBeamOffsetAzimuth << " (el)=" << dBeamOffsetElevation;
            }
            else {
                throw RmoException(QString("SET_ROSE_DELTA: m_pDataHeader->dwSize=%1; sizeof(ACM::ROSE_DELTA_SETTINGS)=%2")
                                   .arg(m_pDataHeader->dwSize).arg(sizeof(ACM::ROSE_DELTA_SETTINGS)));
                return;
            }
        }
        // most typical for now. Later - maybe more cases here
        if (m_pDataHeader->dwType == ACM_TYPE::STROBE_DATA && m_uFileVersion==REG::FORMAT_VERSION) {
            ACM::STROBE_DATA *pStrobeData = (ACM::STROBE_DATA *)pbaStrobe->data();
            CHR::STROBE_HEADER *pStrobeHeader = &pStrobeData->header;
            qint16* pSamples=(qint16*)(pStrobeData+1);
            quint32 uBeamCountsNum=pStrobeData->beamCountsNum;
            quint32 uStrobeNo=pStrobeHeader->strobeNo;
            QByteArray baStructStrobeData=QByteArray::fromRawData(pbaStrobe->data(),sizeof(ACM::STROBE_DATA));

            // qDebug() << "Parsing uStrobeNo " << uStrobeNo;
            // check if DB contains signal type (pStrobeHeader->signalID)
            qint64 iSignalsGUID;  // contains GUID of record of signals table if the record exists
            if (!checkStrobSignalType(baStructStrobeData, iSignalsGUID, parent)) {
                qDebug() << "uStrobeNo " << uStrobeNo << " has unknown signal type";
                continue; // cannot process strobe with undefined signal
            }
            // qDebug() << "addStrobeToNoiseMap(pbaStrobe): uStrobeNo " << uStrobeNo;
            // call processEvents() periodically to ensure GUI responsibility
            qint64 iCurrentTime = QDateTime::currentMSecsSinceEpoch();
            if (iCurrentTime-iLastProcessEventsTime > MAX_GUI_BLOCKING_TIME_MSECS) {
                // qDebug() << "calling processEvents()";
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents,MAX_PROCESS_EVENTS_TIME_MSECS);
                iLastProcessEventsTime = iCurrentTime;
            }
            // create new QNoiseMap object if needed and add current strob
            if (!addStrobeToNoiseMap(pbaStrobe)) {
                throw RmoException(QString("addStrobeToNoiseMap failed for strobe %1").arg(uStrobeNo));
                return;
            }
            // obtain current noisemap GUID
            if (!m_pNoiseMap) {
                throw RmoException(QString("!m_pNoiseMap for strobe %1").arg(uStrobeNo));
                return;
            }
            qint64 iNoiseMapGUID = m_pNoiseMap->getGUID();
            // uLastT = QDateTime::currentMSecsSinceEpoch();
            qint64 iStrobId=m_pSqlModel->addStrobe(uStrobeNo, uBeamCountsNum, baStructStrobeData,
                                                   iFileId, iSignalsGUID, iNoiseMapGUID);
            if (iStrobId==-1) {
                throw RmoException(QString("Could not add strobe record: StrobeNo=%1").arg(uStrobeNo));
                return;
            }
            // m_uDB +=QDateTime::currentMSecsSinceEpoch()-uLastT;
            for (int iBeam=0; iBeam<iNumberOfBeams; iBeam++) {
                // uLastT = QDateTime::currentMSecsSinceEpoch();
                if (int iRetVal=m_pSqlModel->addSamples(iStrobId,iBeam,(char*)pSamples,uBeamCountsNum*iSizeOfComplex)) {
                    throw RmoException(QString("AddSamples returned: %1").arg(iRetVal));
                    return;
                }
                // quint64 uDelta=QDateTime::currentMSecsSinceEpoch()-uLastT;
                // m_uDB +=uDelta;
                // m_uSam +=uDelta;
                pSamples+=uBeamCountsNum*2;
            }
            nRec++;
        }
        //------------------------------------- unknown strobe type
        // else {
        //     qDebug() << "Found type: 0x" << QString::number(m_pDataHeader->dwType,16);
        // }
        // here we explicitly delete QByteArray object
        //-------------------------------------
        delete pbaStrobe;
        if (m_pParseMgr) m_pParseMgr->doUpdateParseProgressBar();
    }
    // uLastT = QDateTime::currentMSecsSinceEpoch();
    if (!m_pSqlModel->setNumberOfRecords(nRec,iFileId)) {
        qDebug() << "Failed to update number of records for fileId: " << iFileId;
    }
    qDebug() << "commitTransaction(): nRec = " << nRec << " iFileId = " << iFileId;
    m_pSqlModel->commitTransaction();
    // m_uDB +=QDateTime::currentMSecsSinceEpoch()-uLastT;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QRegFileParser::addStrobeToNoiseMap(QByteArray *pbaStrobe) {
    // bOk flag
    bool bOk;
    // strobe structures
    ACM::STROBE_DATA *pStrobeData = (ACM::STROBE_DATA *)pbaStrobe->data();
    CHR::STROBE_HEADER *pStrobeHeader = &pStrobeData->header;
    qint16* pSamples=(qint16*)(pStrobeData+1);
    // quint32 uBeamCountsNum=pStrobeData->beamCountsNum;
    quint32 uStrobeNo=pStrobeHeader->strobeNo;
    QByteArray baStructStrobeData=QByteArray::fromRawData(pbaStrobe->data(),sizeof(ACM::STROBE_DATA));

    // if no current noiseMap or strobe parameteres changed - create new noise map
    bool bStrobeUnchanged = m_pPoi->checkStrobeParams(baStructStrobeData);
    if (!m_pNoiseMap || !bStrobeUnchanged) { // we need new noise map
        // qDebug() << "addStrobeToNoiseMap(): m_pNoiseMap=" << m_pNoiseMap << " bStrobeChanged=" << bStrobeUnchanged;
        // this strob's signal type was found in DB - update m_pPoi fields
        m_pPoi->m_iSigType = pStrobeHeader->signalID;
        m_pPoi->m_iSignalLen = pStrobeHeader->pDuration;
        if (!m_pSqlModel->seekSignal(m_pPoi->m_iSigType,m_pPoi->m_iSignalLen,m_pPoi->m_baSignalSamplData)) {
            throw RmoException(QString("seekSignal() failed for strobe %1").arg(uStrobeNo));
            return false;
        }
        if (!m_pPoi->updateStrobeParams(baStructStrobeData)) {
            throw RmoException("addStrobeToNoiseMap(): updateStrobeParams failed");
            return false;
        }
        if (!m_pPoi->updateNFFT()) {
            throw RmoException("addStrobeToNoiseMap(): updateNFFT() failed");
            return false;
        }
        int NFFT = m_pPoi->m_NFFT;
        int iFilteredN = m_pPoi->m_iFilteredN;
        // if current noise map exists, write it to DB
        if (m_pNoiseMap) {
            QByteArray baNoiseMap;
            qint64 iGUID_previous;
            if (!m_pNoiseMap->getMap(iGUID_previous,baNoiseMap)) {
                throw RmoException("#1 addStrobeToNoiseMap(): getMap() failed");
                return false;
            }
            qDebug() << "m_pSqlModel->updateNoiseMap() iGUID_previous = " << iGUID_previous;
            if (!m_pSqlModel->updateNoiseMap(iGUID_previous, baNoiseMap)) {
                throw RmoException("addStrobeToNoiseMap(): updateNoiseMap() failed");
                return false;
            }
            delete m_pNoiseMap;
            m_pNoiseMap = 0;
        }
        // create new noise map in DB and in memory
        qint64 iGUID_new = m_pSqlModel->addNewNoiseMap(NFFT,iFilteredN,&bOk);
        if (!bOk) {
            throw RmoException("addStrobeToNoiseMap(): addNewNoiseMap() failed");
            return false;
        }
        qDebug() << "m_pSqlModel->addNewNoiseMap(" << NFFT << ", "
                                                   << iFilteredN <<") returned "
                                                   << "iGUID_new = " << iGUID_new;
        m_pNoiseMap = new QNoiseMap(NFFT,iFilteredN,iGUID_new);
        if (!m_pNoiseMap) {
            throw RmoException("addStrobeToNoiseMap(): new QNoiseMap() failed");
            return false;
        }
    }
    // go on with regular operation - append current strobe to noise map
    if (!m_pNoiseMap) {
        throw RmoException("addStrobeToNoiseMap(): !m_pNoiseMap");
        return false;
    }
    // sum of beams
    QByteArray baBeamsSumDP;
    baBeamsSumDP.resize(m_pPoi->m_iBeamCountsNum*2*sizeof(double));
    double *pSum = (double *)baBeamsSumDP.data();
    for (int i=0; i<2*m_pPoi->m_iBeamCountsNum; i++) {
        pSum[i]=0.0e0;
        for (int iBeam=0; iBeam<QPOI_NUMBER_OF_BEAMS; iBeam++) {
            quint32 idx = iBeam * (2*m_pPoi->m_iBeamCountsNum) + i;
            pSum[i]+=pSamples[idx];
        }
    }
    // calculate Doppler representation
    QByteArray baDopplerRep = m_pPoi->dopplerRepresentation(baBeamsSumDP);
    if (baDopplerRep.isEmpty()) {
        throw RmoException("addStrobeToNoiseMap(): baDopplerRep.isEmpty()");
        return false;
    }
    if (!m_pNoiseMap->appendStrobe(baDopplerRep)) {
        throw RmoException("addStrobeToNoiseMap(): appendStrobe(baDopplerRep) failed");
        return false;
    }
    return true;
}
