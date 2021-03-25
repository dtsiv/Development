#ifndef QSQLMODEL_H
#define QSQLMODEL_H

#include <QtGui>
#include <QtWidgets>
#include <QtSql>
#include <QMetaObject>

#include "qinisettings.h"
#include "qproppages.h"

#define QSQLMODEL_SQLITE_FILE                    "SQLiteFile"
#define DATA_BASE_VERSION                        "20210210"

#define QSQLMODEL_PROP_TAB_CAPTION "DB connection"

#define CONN_STATUS_SQLITE   "Connected to SQLite"
#define CONN_STATUS_DISCONN  "Disconnected from DB"

class QSqlModel : public QObject {
	Q_OBJECT

public:
    QSqlModel();
	~QSqlModel();
    // elementary methods of DB interaction
    bool openDatabase();
    void closeDatabase();
    void startTransaction();
    void commitTransaction();
    bool isDBOpen();
    QString getDBFileName();
    bool verifyDBFormat(QSqlDatabase &db);
    bool changeDatabaseName(QString qsDbFileName);
    bool checkDatabaseName(QString qsDbFileName, QString &qsErrorString);
    // int dropTables();
    int createTables(QSqlDatabase &db);
    // data input/output queries
    bool prepareQueries();
    bool execMainSelectQuery();
    bool getStrobRecord(qint64 &iRecId, int &iStrob, int &iBeamCountsNum, qint64 &iTimestamp,
                        QByteArray &baStructStrobeData, quint32 &uFileVer,
                        QByteArray &baSignalSamplData, qint32 &iSigType, qint32 &iSamplCnt,
                        qint64 &iNoisemapGUID, qint32 &iNFFT, qint32 &iFilteredN);
    bool getBeamData(qint64 &iRecId, int &iBeam, QByteArray &baSamples);
    qint64 addFileRec(qint64 uTimeStamp, QString qsFilePath, int nStrobs, QString qsFileVer);
    qint64 addStrobe(int strobeNo, int beamCountsNum, QByteArray &baStructStrobeData,
                     qint64 iFileId, qint64 iSignalsGUID, qint64 iMapId);
    qint64 addSignal(int iSignalType, int iSignalLen,
                     QByteArray &baSignalFileData, QByteArray &baSignalFileText,
                     QString qsSignalFileName /* =QString() */, bool *pbOk = NULL);
    qint64 seekSignal(int iSignalType, int iSignalLen, QByteArray &baSigFileDataOut, bool *pbOk = NULL);
    bool updateNoiseMap(qint64 iNoiseMapGUID, QByteArray &baNoiseMap);
    QByteArray getNoiseMap(qint64 iNoiseMapGUID);
    qint64 addNewNoiseMap(int NFFT, int iFilteredN, bool *pbOk = NULL);

    int addSamples(qint64 iStrobId, int iBeam, char *pSamples, int iSize);
    bool setNumberOfRecords(int nRec, qint64 iFileId);
    bool getTotStrobes(quint32 &iTotStrobes);
    // QPropPages-related methods
    Q_INVOKABLE void addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx);
    Q_INVOKABLE void propChanged(QObject *pPropDlg);

public slots:
    void onSQLiteFileChoose();
    void onDBFileNameEditingFinished();
    void onDBValidationFailed();

private:
	QString m_qsDBFile;

public:
    // main SELECT query
    QSqlQuery m_mainSelectQuery;
private:
    QSqlRecord m_mainSelectRecord;

    // other (prepared) queries & their records
    QSqlQuery m_queryBeamData;
    QSqlRecord m_recBeamData;
    QSqlQuery m_queryTotStrobes;
    QSqlRecord m_recTotStrobes;
    QSqlQuery m_queryFileRec01;
    QSqlRecord m_recFileRec01;
    QSqlQuery m_queryFileRec02;
    QSqlQuery m_queryFileRec03;
    QSqlQuery m_queryFileRec04;
    QSqlQuery m_queryFileRec05;
    QSqlQuery m_queryFileRec06;
    QSqlQuery m_queryFileRec08;
    QSqlRecord m_recFileRec07;
    QSqlQuery m_queryAddStrobe01;
    QSqlQuery m_queryAddStrobe03;
    QSqlRecord m_recAddStrobe03;
    QSqlQuery m_queryAddStrobe04;
    QSqlQuery m_queryAddStrobe06;
    QSqlQuery m_recAddStrobe06;
    QSqlQuery m_queryAddStrobe07;
    QSqlQuery m_queryAddStrobe08;
    QSqlRecord m_recAddStrobe08;
    QSqlQuery m_querySetNumOfRecords;
    QSqlQuery m_queryAddSamples;
};


#endif // QSQLMODEL_H
