#include "qregfileparser.h"
#include "qpulsesamplingdlg.h"
#include "qindicatorwindow.h"
#include "qexceptiondialog.h"
#include "qnoisemap.h"
#include "qsqlmodel.h"

#include <windows.h>
#include <stdlib.h>

//====================================================================
//
//====================================================================
QRegFileParser::QRegFileParser(QIndicatorWindow *pOwner) : QObject(0)
         , m_uTimeStamp(0)
         , m_pFileHdr(NULL)
         , m_pStrobeData(NULL)
         , m_bParseWholeDir(false)
         , m_uFileVersion(0)
         , m_pOffsetsArray(NULL)
         , m_pOffsets(NULL)
         , m_uOffset(0)
         , m_uSize(0)
         , m_pDataHeader(0)
         , m_iRec(0)
         , m_pOwner(pOwner)
         , m_pPoi(NULL)
         , m_pSqlModel(NULL)
         , m_pNoiseMap(NULL)
         , iNumberOfBeams(4)
         , iSizeOfComplex(2*sizeof(qint16))
         , m_pParseMgr(NULL) {

    m_pDataHeader = new struct s_dataHeader;

    // settings object and main window adjustments
    QIniSettings &iniSettings = QIniSettings::getInstance();
    QIniSettings::STATUS_CODES scRes;
    iniSettings.setDefault(SETTINGS_KEY_DATAFILE,QString("C:/PROGRAMS/DATA/RegFixed/2019-03-01T14-12-02/00000000"));
    m_qsRegFile = iniSettings.value(SETTINGS_KEY_DATAFILE,scRes).toString();
    iniSettings.setDefault(SETTINGS_KEY_PARSE_WHOLE_DIR,true);
    m_bParseWholeDir = iniSettings.value(SETTINGS_KEY_PARSE_WHOLE_DIR,scRes).toBool();
    m_pPoi = m_pOwner->m_pPoi;
    m_pSqlModel = m_pOwner->m_pSqlModel;
    if (!m_pPoi || !m_pSqlModel) qDebug() << "QRegFileParser: !m_pPoi || !m_pSqlModel";
}
//====================================================================
//
//====================================================================
QRegFileParser::~QRegFileParser() {
    QIniSettings &iniSettings = QIniSettings::getInstance();
    iniSettings.setValue(SETTINGS_KEY_DATAFILE, m_qsRegFile);
    iniSettings.setValue(SETTINGS_KEY_PARSE_WHOLE_DIR, m_bParseWholeDir);
    m_pOffsetsArray = NULL;
    m_pOffsets = NULL;
    m_pFileHdr = NULL;
    m_pStrobeData = NULL;
    if (m_pDataHeader) delete m_pDataHeader; m_pDataHeader = NULL;
    if (m_pNoiseMap) delete m_pNoiseMap; m_pNoiseMap = NULL;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QRegFileParser::addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    QTabWidget *pTabWidget = qobject_cast<QTabWidget *> (pPropTabs);

    QWidget *pWidget=new QWidget;
    QHBoxLayout *pHLayout=new QHBoxLayout;
    pPropPages->m_pleRegFileName=new QLineEdit(m_qsRegFile);
    pHLayout->addWidget(new QLabel("Reg file name"));
    pHLayout->addWidget(pPropPages->m_pleRegFileName);
    QPushButton *ppbChoose=new QPushButton(QIcon(":/Resources/open.ico"),"Choose");
    QObject::connect(ppbChoose,SIGNAL(clicked()),pPropPages,SIGNAL(chooseRegFile()));
    QObject::connect(pPropPages,SIGNAL(chooseRegFile()),SLOT(onRegFileChoose()));
    pHLayout->addWidget(ppbChoose);
    QVBoxLayout *pVLayout=new QVBoxLayout;
    pVLayout->addLayout(pHLayout);
    QHBoxLayout *pHLayout01=new QHBoxLayout;
    pPropPages->m_pcbParseWholeDir=new QCheckBox("Parse whole dirrectory");
    pHLayout01->addWidget(pPropPages->m_pcbParseWholeDir);
    pPropPages->m_pcbParseWholeDir->setChecked(m_bParseWholeDir);
    pPropPages->m_ppbParse=new QPushButton("Parse");
    QObject::connect(pPropPages->m_ppbParse,SIGNAL(clicked(bool)),pPropPages,SIGNAL(doParse()));
    pHLayout01->addWidget(pPropPages->m_ppbParse);
    pPropPages->m_ppbarParseProgress=new QProgressBar;
    pPropPages->m_ppbarParseProgress->setValue(0);
    pHLayout01->addWidget(pPropPages->m_ppbarParseProgress);
    // pHLayout01->addStretch();
    pVLayout->addLayout(pHLayout01);
    QHBoxLayout *pHLayout02=new QHBoxLayout;
    pHLayout02->addWidget(new QLabel("Status"));
    pPropPages->m_plbParsingMsg=new QLabel;
    pHLayout02->addWidget(pPropPages->m_plbParsingMsg);
    pVLayout->addLayout(pHLayout02);
    pVLayout->addStretch();
    pWidget->setLayout(pVLayout);
    pTabWidget->insertTab(iIdx,pWidget,QREGFILEPARSER_PROP_TAB_CAPTION);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QRegFileParser::propChanged(QObject *pPropDlg) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    m_bParseWholeDir = pPropPages->m_pcbParseWholeDir->isChecked();
    m_qsRegFile = pPropPages->m_pleRegFileName->text();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QRegFileParser::onRegFileChoose() {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (QObject::sender());

    QFileDialog dialog(pPropPages);
    QString qsRegFile = pPropPages->m_pleRegFileName->text();
    QFileInfo fiDataFile(qsRegFile);
    QDir qdRoot;
    if (!fiDataFile.exists() || !fiDataFile.isFile()) {
        qdRoot=QDir::current();
    }
    else {
        qdRoot=fiDataFile.absoluteDir();
    }
    dialog.setDirectory(qdRoot);
    dialog.setFileMode(QFileDialog::ExistingFile);
    // dialog.setNameFilter(tr("SQLite db file (*.db *.sqlite *.sqlite3)"));
    if (dialog.exec()) {
        QStringList qsSelection=dialog.selectedFiles();
        if (qsSelection.size() != 1) return;
        QString qsSelFilePath=qsSelection.at(0);
        QFileInfo fiSelFilePath(qsSelFilePath);
        if (fiSelFilePath.isFile() && fiSelFilePath.isReadable())	{
            QDir qdCur = QDir::current();
            QDir qdSelFilePath = fiSelFilePath.absoluteDir();
            if (qdCur.rootPath() == qdSelFilePath.rootPath()) { // same Windows drives
                pPropPages->m_pleRegFileName->setText(qdCur.relativeFilePath(fiSelFilePath.absoluteFilePath()));
            }
            else { // different Windows drives
                pPropPages->m_pleRegFileName->setText(fiSelFilePath.absoluteFilePath());
            }
        }
        else {
            pPropPages->m_pleRegFileName->setText("error");
        }
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int QRegFileParser::openRegDataFile(QString qsRegFile) {
    // check if file does not exist or cannot be opened
    QFileInfo fiRegFile(qsRegFile);
    if (!fiRegFile.exists() || !fiRegFile.isFile() || !fiRegFile.isReadable()) {
        qDebug() << "Cannot open file: " << fiRegFile.fileName();
        return 1;
    }
    // assign name of the registration file and open it
    m_qfRegFile.setFileName(qsRegFile);
    if (!m_qfRegFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file: " << qsRegFile;
        return 2;
    }
    // read file header
    if (m_qfRegFile.read((char*)&m_sFileHdr,sizeof(m_sFileHdr)) != sizeof(m_sFileHdr)) return 3;
    if (QString(QByteArray(m_sFileHdr.sMagic,sizeof(m_sFileHdr.sMagic)))!=REG::FILE_MAGIC) return 4;
    // version of the registration file
    if (  m_sFileHdr.uVer != REG_OLD_VER1::FORMAT_VERSION
       && m_sFileHdr.uVer != REG::FORMAT_VERSION) return 5;
    m_uFileVersion = m_sFileHdr.uVer;
    m_uSize = m_qfRegFile.size();
    if (m_uFileVersion==REG_OLD_VER1::FORMAT_VERSION) {
        if (m_sFileHdr.nRec > m_sFileHdr.nRecMax
         || sizeof(m_sFileHdr)+m_sFileHdr.nRecMax*sizeof(quint32)+m_sFileHdr.nRec*sizeof(struct ACM_OLD_VER1::ACM_STROBE_DATA) > m_uSize) return 6;
    }
    // in format version 4: pointers table has (m_sFileHdr.nRec) elements, each element is a 4-Bytes pointer
    if (m_uFileVersion==REG::FORMAT_VERSION) {
        if (m_sFileHdr.nRec > m_sFileHdr.nRecMax
         || sizeof(struct sFileHdr)                             // file header size
           +m_sFileHdr.nRec*sizeof(quint32)                     // pointers table size
           +m_sFileHdr.nRec*sizeof(struct s_dataHeader)         // data headers size
                > m_uSize) return 7;
    }
    // DTSIV: наверное, uProtoVersion не используется и может привести к ошибке потом
    // 20200722: In format version 4 uProtoVersion was removed, see
    // if (m_uFileVersion==REG::FORMAT_VERSION) {
    //    quint32 uProtoVersion=0;
    //    if (m_qfRegFile.read((char*)&uProtoVersion,sizeof(quint32)) != sizeof(quint32)) return 1;
    //    // tsStdOut << "uProtoVersion = " << uProtoVersion << endl;
    //}
    m_pOffsetsArray=new quint32[m_sFileHdr.nRec];
    m_pOffsets=m_pOffsetsArray;
    if (m_qfRegFile.read((char*)m_pOffsets,m_sFileHdr.nRec*sizeof(quint32)) != m_sFileHdr.nRec*sizeof(quint32)) return 8;
    m_uOffset=0;
    if (m_uFileVersion==REG_OLD_VER1::FORMAT_VERSION) {
        m_uOffset = *m_pOffsets++;
    }
    else if (m_uFileVersion==REG::FORMAT_VERSION) {
        m_uOffset = m_qfRegFile.pos();
    }
    else {
        return 9;
    }
    m_iRec=0;
    return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int QRegFileParser::closeRegDataFile() {
    m_uFileVersion=0;
    m_uSize=0;
    if (m_pOffsetsArray) delete[]m_pOffsetsArray;
    m_pOffsetsArray=NULL;
    m_pOffsets=NULL;
    m_uOffset=0;
    std::memset((char*)&m_sFileHdr,'\0',sizeof(m_sFileHdr));
    m_iRec=0;
    if (!m_qfRegFile.isOpen()) {
        qDebug() << "file not open: " << QFileInfo(m_qfRegFile).absoluteFilePath();
        return 1;
    }
    m_qfRegFile.close();
    return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QRegFileParser::isAtEnd() {
    if (m_qfRegFile.isOpen() && m_iRec < m_sFileHdr.nRec && m_uOffset+sizeof(struct s_dataHeader) < m_uSize) return false;
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double QRegFileParser::getProgress() {
    if (m_sFileHdr.nRec < 1 || m_iRec>m_sFileHdr.nRec) return 0.0;
    return (double)m_iRec/m_sFileHdr.nRec;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QByteArray* QRegFileParser::getStrobe() {
    // qDebug() << "iRec=" << iRec << " m_sFileHdr.nRec=" << m_sFileHdr.nRec;
    if (m_iRec>m_sFileHdr.nRec) return NULL;
    m_uOffset = *m_pOffsets++;
    // qDebug() << "m_uOffset+sizeof(struct s_dataHeader)=" << m_uOffset+sizeof(struct s_dataHeader) << " m_uSize=" << m_uSize;
    if (m_uOffset+sizeof(struct s_dataHeader) > m_uSize) return NULL;
    // qDebug() << "m_uOffset=" << m_uOffset;
    m_qfRegFile.seek(m_uOffset);
    quint32 uDataHeaderSize = m_qfRegFile.read((char*)m_pDataHeader,sizeof(struct s_dataHeader));
    // qDebug() << "uDataHeaderSize=" << uDataHeaderSize << " sizeof(struct s_dataHeader)=" << sizeof(struct s_dataHeader);
    if (uDataHeaderSize != sizeof(struct s_dataHeader)) return NULL;
    // qDebug() << "m_pDataHeader->dwSize=" << m_pDataHeader->dwSize;
    // qDebug() << "m_uOffset + sizeof(struct s_dataHeader) + m_pDataHeader->dwSize=" << m_uOffset + sizeof(struct s_dataHeader) + m_pDataHeader->dwSize;
    // qDebug() << "m_uSize=" << m_uSize;
    if ((m_pDataHeader->dwSize == 0) || (m_uOffset + sizeof(struct s_dataHeader) + m_pDataHeader->dwSize > m_uSize)) return NULL;
    QByteArray *pbaRetVal = new QByteArray(m_pDataHeader->dwSize,'\0');
    // qDebug() << "m_pDataHeader->dwSize=" << m_pDataHeader->dwSize << " pbaRetVal=" <<pbaRetVal;
    if (!pbaRetVal) return NULL;
    quint32 uStrobSize = m_qfRegFile.read((char*)pbaRetVal->data(),m_pDataHeader->dwSize);
    // qDebug() << "uStrobSize=" << uStrobSize << " m_pDataHeader->dwSize=" << m_pDataHeader->dwSize;
    if (uStrobSize != m_pDataHeader->dwSize) {
        qDebug() << "QRegFileParser::getStrobe(): uStrobSize != m_pDataHeader->dwSize";
        delete pbaRetVal; return NULL;
    }
    m_iRec++;
    return pbaRetVal;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QRegFileParser::checkStrobSignalType(QByteArray &baStructStrobeData, qint64 &iSignalsGUID,
                                          QWidget *parent /* = 0 */) {
    // strobe params structure
    ACM::STROBE_DATA *pStructStrobeData = (ACM::STROBE_DATA*)baStructStrobeData.data();
    CHR::STROBE_HEADER *pStructStrobeHeader = (CHR::STROBE_HEADER*)(&pStructStrobeData->header);
    int signalID = pStructStrobeHeader->signalID;
    int pDuration = pStructStrobeHeader->pDuration;
    bool bOk;
    QByteArray baSigDataInDB; // dummy parameter, not used
    iSignalsGUID = m_pSqlModel->seekSignal(signalID, pDuration, baSigDataInDB, &bOk);
    if (bOk) return true;
    qDebug() << "checkStrobSignalType(): seekSignal failed for signalID=" << signalID << " pDuration=" << pDuration;

    // only need further action if seekSignal failed
    QPulseSamplingDlg pulseSamplingDlg(m_pOwner,pStructStrobeHeader,parent);
    // block unwanted user input during modal dialog
    m_pOwner->m_bSigSelInProgress = true;
    QDialog::DialogCode dcSignalDlgRslt=(QDialog::DialogCode)pulseSamplingDlg.exec();
    // unblock user input
    m_pOwner->m_bSigSelInProgress = false;
    // analyze dialog result
    if (dcSignalDlgRslt==QDialog::Accepted) {
        // parse signal file
        QString qsSignalFileName = pulseSamplingDlg.m_plePlsSmplFileName->text();
        // dummy variables for signal type ans length
        int iSigType;
        int iSigLen;
        QByteArray baSignalFileData;
        QByteArray baSignalFileText;
        // parse QFile(qsSignalFileName)
        if (parseSignalFile(qsSignalFileName,
                            baSignalFileData,
                            iSigType,
                            iSigLen,
                            baSignalFileText)) {
            if (iSigType != signalID || iSigLen != pDuration) {
                throw RmoException(qsSignalFileName + " - iSigType != signalID || iSigLen != pDuration");
                return false;
            }
            iSignalsGUID = m_pSqlModel->addSignal(iSigType,iSigLen,
                                   baSignalFileData,baSignalFileText,
                                   qsSignalFileName,&bOk);
            if (!bOk) {
                throw RmoException(qsSignalFileName + " - addSignal failed for file");
                return false;
            }
            return true;
        }
        else {
            qDebug() << "parseSignalFile() failed for file " << qsSignalFileName;
            return false;
        }
    }
    qDebug() << "Skipping strobe with unknown signal type " << signalID;
    return false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QRegFileParser::parseSignalFile(QString qsSignalFileName,
                                     QByteArray &baSignalFileData,
                                     int &iSignalID,
                                     int &iSignalLen,
                                     QByteArray &baSignalFileText) {

    QFile qfSignalFile(qsSignalFileName);
    if (!qfSignalFile.exists() || !qfSignalFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file: " << QFileInfo(qfSignalFile).fileName();
        return false;
    }
    baSignalFileText = qfSignalFile.readAll();
    if (baSignalFileText.isEmpty()) {
        qDebug() << "Cannot read file: " << QFileInfo(qfSignalFile).fileName();
        return false;
    }
    QTextStream tsSignalFile(&baSignalFileText,QIODevice::ReadOnly);
    bool bOk;
    QString qsLine;
    qsLine = tsSignalFile.readLine();
    iSignalID = qsLine.toInt(&bOk);
    if (!bOk) {
        qDebug() << "parseSignalFile(): failed to read iSignalID";
        return false;
    }
    qsLine = tsSignalFile.readLine();
    iSignalLen = qsLine.toInt(&bOk);
    if (!bOk) {
        qDebug() << "parseSignalFile(): failed to read iSignalLen";
        return false;
    }
    baSignalFileData.clear();
    baSignalFileData.resize(2*iSignalLen*sizeof(qint16));
    qint16 *pSignalData = (qint16 *)baSignalFileData.data();
    for (int i=0; i<2*iSignalLen; i++) {
        if (tsSignalFile.atEnd()) {
            qDebug() << "parseSignalFile(): tsSignalFile ended";
            return false;
        }
        tsSignalFile >> *pSignalData++;
    }
    qfSignalFile.close();
    return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QRegFileParser::listRegFilesDir(QObject *pSender) {
    // get access to QPropPages dynamic object (!take care - only exists while dlg)
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pSender);
    // start parsing registration file(s)
    m_bParseWholeDir = pPropPages->m_pcbParseWholeDir->isChecked();
    // selected registration data file
    QString qsRegFile = pPropPages->m_pleRegFileName->text();
    QFileInfo fiRegFile(qsRegFile);
    if (!fiRegFile.exists() || !fiRegFile.isFile()) {
        throw RmoException(QString("Could not find reg file ")+fiRegFile.absoluteFilePath());
        return;
    }
    m_fiRegFile = fiRegFile;
    // list the directory of selected registration data file
    QDir qdRoot=m_fiRegFile.absoluteDir();
    QStringList qslFiles = qdRoot.entryList(QDir::Files);
    // scan current direcoty for files of time sampling of transmitted signal "SignalXX.txt"
    for (int j=0; j<qslFiles.count(); j++) {
        QRegExp rx("^Signal\\d\\d\\.txt$");
        // skip filenames those did not match pattern
        if (!rx.exactMatch(qslFiles.at(j))) continue;
        QString qsCurSigFile = qdRoot.absoluteFilePath(qslFiles.at(j));
        // qDebug() << "Found exact match: " << qsCurSigFile;
        if (!qdRoot.exists(qsCurSigFile)) {
            throw RmoException(QString("Could not find signal file ")+qsCurSigFile);
            return;
        }
        // get absolute filename for current file
        QFileInfo fiSigFile(qsCurSigFile);
        if (!fiSigFile.isFile() || !fiSigFile.isReadable()) {
            throw RmoException(QString("Could not read signal file ")+qsCurSigFile);
            return;
        }
        qDebug() << "found signal file: " << fiSigFile.fileName();
        int iSignalType;
        int iSignalLen;
        QByteArray baSignalFileData;
        QByteArray baSignalFileText;
        // parse QFile(qsCurSigFile)
        if (!parseSignalFile(qsCurSigFile,baSignalFileData,
                            iSignalType,iSignalLen,baSignalFileText)) {
            throw RmoException(QString("parseSignalFile() failed for file ")+qsCurSigFile);
            return;
        }
        bool bOk;
        // try to seek this signal type in DB signals table. If none - try to add new record
        m_pSqlModel->addSignal(iSignalType,iSignalLen,
                               baSignalFileData,baSignalFileText,
                               qsCurSigFile,&bOk);
        if (!bOk) {
            throw RmoException(QString("addSignal() failed for file ")+qsCurSigFile);
            return;
        }
    }
    // extract time stamp
    QDateTime dtTimeStamp;
    QString qsCurRegFile = m_fiRegFile.absoluteFilePath();
    QStringList qslDirs=qsCurRegFile.split("/");
    if (qslDirs.count()>1) {
        QString qsTimeStamp=qslDirs.at(qslDirs.count()-2);
        QStringList qslDateTime=qsTimeStamp.split("T");
        if (qslDateTime.count()==2) {
            dtTimeStamp=QDateTime(
                QDate::fromString(qslDateTime.at(0),"yyyy-MM-dd"),
                QTime::fromString(qslDateTime.at(1),"hh-mm-ss"),
                Qt::LocalTime);
        }
    }
    // by default use file time stamp
    m_uTimeStamp = m_fiRegFile.created().toMSecsSinceEpoch();
    // if possible use time stamp from file path
    if (dtTimeStamp.isValid()) {
        int iYear= dtTimeStamp.date().year();
        if (iYear > 1990 && iYear < 2100) {
            m_uTimeStamp = dtTimeStamp.toMSecsSinceEpoch();
        }
    }

    if (!m_bParseWholeDir) { // parse single file
        if (openRegDataFile(m_fiRegFile.absoluteFilePath())) {
            throw RmoException(QString("Could not open reg file ")+qsCurRegFile);
            return;
        }
        emit pPropPages->setParsingMsg(QString("Parsing "+m_fiRegFile.fileName()+"..."));
        parseRegDataFile(pPropPages);
        emit pPropPages->setParsingMsg(QString("Parsing "+m_fiRegFile.fileName()+" done"));
        closeRegDataFile();
    }
    else { // parse whole directory
        // seek for registration files: 00000000, 00000001, ...
        for (int j=0; j<qslFiles.count(); j++) {
            if (m_pOwner->m_bShutDown) return;

            QRegExp rx("^\\d\\d\\d\\d\\d\\d\\d\\d$");
            // skip filenames those did not match pattern
            if (!rx.exactMatch(qslFiles.at(j))) continue;
            qsCurRegFile = qdRoot.absoluteFilePath(qslFiles.at(j));
            // qDebug() << "Found exact match: " << qsCurRegFile;
            if (!qdRoot.exists(qsCurRegFile)) {
                throw RmoException(QString("Could not find reg file ")+qsCurRegFile);
                return;
            }
            // get absolute filename for current file
            QFileInfo fiRegFile(qsCurRegFile);
            if (!fiRegFile.isFile() || !fiRegFile.isReadable()) {
                throw RmoException(QString("Could not read reg file ")+fiRegFile.fileName());
                return;
            }
            m_fiRegFile = fiRegFile;
            // open current file (read header and verify format)
            if (openRegDataFile(m_fiRegFile.absoluteFilePath())) {
                throw RmoException(QString("Could not open reg file ")+qsCurRegFile);
                return;
            }
            emit pPropPages->setParsingMsg(QString("Parsing "+qslFiles.value(j).trimmed()+" ..."));
            // quint64 uLastT = QDateTime::currentMSecsSinceEpoch();
            parseRegDataFile(pPropPages);
            // m_uTot +=QDateTime::currentMSecsSinceEpoch()-uLastT;
            emit pPropPages->setParsingMsg(QString("Parsing "+qslFiles.value(j).trimmed()+" done"));
            if (m_pParseMgr) m_pParseMgr->doUpdateParseProgressBar();
            closeRegDataFile();
        }
    }
    // if there is currently pending noise map - write it to DB and delete
    if (m_pNoiseMap) {
        QByteArray baNoiseMap;
        qint64 iGUID;
        if (!m_pNoiseMap->getMap(iGUID,baNoiseMap)) {
            throw RmoException(QString("final getMap() failed"));
            return;
        }
        qDebug() << "m_pSqlModel->updateNoiseMap() iGUID=" << iGUID;
        if (!m_pSqlModel->updateNoiseMap(iGUID,baNoiseMap)) {
            throw RmoException(QString("final updateNoiseMap() failed"));
            return;
        }
        delete m_pNoiseMap;
        m_pNoiseMap = NULL;
    }
    return;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QRegFileParser::changeAndReopenDB(QObject *pSender) {
    // get access to QPropPages dynamic object (!take care - only exists while dlg)
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pSender);
    // use current m_pleDBFileName->text() and connect to DB if necessary
    QString qsDbFileName = pPropPages->m_pleDBFileName->text();
    if (!m_pSqlModel->changeDatabaseName(qsDbFileName)) {
        throw RmoException(QString("Could not change DB to ")+qsDbFileName);
        return;
    }
    if (!m_pSqlModel->openDatabase()) {
        throw RmoException(QString("Could not open DB ")+qsDbFileName);
        return;
    }
}
