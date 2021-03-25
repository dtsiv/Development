#ifndef QREGFILEPARSER_H
#define QREGFILEPARSER_H

// Changed 2020.07.22
// Introduced changed for format version 4

#include <QtCore>
#include <QMetaObject>

#include "qproppages.h"
#include "qinisettings.h"

#include "qchrprotosnc.h"
#include "qchrprotoacm.h"

#define QREGFILEPARSER_PROP_TAB_CAPTION "Parse registration file"

#define SETTINGS_KEY_DATAFILE           "dataFile"
#define SETTINGS_KEY_PARSE_WHOLE_DIR    "parseWholeDir"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4200)
#endif

class QIndicatorWindow;
class QPoi;
class QSqlModel;
class QParseMgr;
class QNoiseMap;

class QRegFileParser : public QObject {
    Q_OBJECT

    struct sFileHdr {
        char sMagic[4];
        quint32 uVer;              // format version = 0x00000004
        quint32 uCodogramsVer;     // version of codograms.h
        quint32 uPtrTblOffset;     // offset for table of pointers (offset=40=0x28)
        quint32 nRecMax;           // maximum number of records (=1000)
        quint32 nRec;              // de facto number of records
        char pVoid[16];            // empty space before table of ponters (offset 40=0x28)
    };

    struct s_dataHeader{
        quint32 dwSize;
        quint32 dwType;
        char pFileData[];
    };

    struct s_signalParams{
        int iSignalID;
        int iSignalLen;
    };

public:
    QRegFileParser(QIndicatorWindow *pOwner);
    ~QRegFileParser();
    void changeAndReopenDB(QObject *pSender);
    void listRegFilesDir(QObject *pSender);
    void parseRegDataFile(QObject *pSender);
    int openRegDataFile(QString qsRegFile);
    int closeRegDataFile();
    QByteArray *getStrobe();
    bool isAtEnd();
    double getProgress();
    bool checkStrobSignalType(QByteArray &baStructStrobeData, qint64 &iSignalsGUID, QWidget *parent = 0);
    bool parseSignalFile(QString qsSignalFileName, QByteArray &baSignalFileData, int &iSignalID,
                         int &iSignalLen, QByteArray &baSignalFileText);
    bool addStrobeToNoiseMap(QByteArray *pbaStrobe);

    Q_INVOKABLE void addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx);
    Q_INVOKABLE void propChanged(QObject *pPropDlg);

public slots:
    void onRegFileChoose();

public:
    QString              m_qsRegFile;
    quint64              m_uTimeStamp;
    QFile                m_qfRegFile;
    QFileInfo            m_fiRegFile;
private:
    struct sFileHdr     *m_pFileHdr;
    ACM::STROBE_DATA    *m_pStrobeData;
public:
    bool                 m_bParseWholeDir;
    struct sFileHdr      m_sFileHdr;
    quint32              m_uFileVersion;
private:
    quint32              *m_pOffsetsArray;
    quint32              *m_pOffsets;
    quint32              m_uOffset;
    quint32              m_uSize;
public:
    struct s_dataHeader  *m_pDataHeader;
private:
    quint32              m_iRec;
    QIndicatorWindow     *m_pOwner;
    QPoi                 *m_pPoi;
    QSqlModel            *m_pSqlModel;
    int                  iNumberOfBeams;
    int                  iSizeOfComplex;
    QNoiseMap            *m_pNoiseMap;
public:
    QParseMgr            *m_pParseMgr;
friend class QParseMgr;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // QREGFILEPARSER_H


