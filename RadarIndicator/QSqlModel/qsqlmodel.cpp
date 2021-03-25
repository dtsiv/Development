#include "qsqlmodel.h"
#include "qinisettings.h"
#include "qexceptiondialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSqlModel::QSqlModel() : QObject(0)
        , m_qsDBFile("Uninitialized") {
    QIniSettings &iniSettings = QIniSettings::getInstance();
    QIniSettings::STATUS_CODES scRes;
    iniSettings.setDefault(QSQLMODEL_SQLITE_FILE,"ChronicaParser.sqlite3");
    m_qsDBFile = iniSettings.value(QSQLMODEL_SQLITE_FILE,scRes).toString();
    if (!QSqlDatabase::contains("QSQLITE")) {
        QSqlDatabase::addDatabase("QSQLITE");
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSqlModel::~QSqlModel() {
    QIniSettings &iniSettings = QIniSettings::getInstance();
    iniSettings.setValue(QSQLMODEL_SQLITE_FILE, m_qsDBFile);
    closeDatabase();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSqlModel::addTab(QObject *pPropDlg, QObject *pPropTabs, int iIdx) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    QTabWidget *pTabWidget = qobject_cast<QTabWidget *> (pPropTabs);

    QWidget *pWidget=new QWidget;
    QVBoxLayout *pVLayout=new QVBoxLayout;
    QHBoxLayout *pHLayout=new QHBoxLayout;
    pPropPages->m_pleDBFileName=new QLineEdit(m_qsDBFile);
    pHLayout->addWidget(new QLabel("DB file name"));
    pHLayout->addWidget(pPropPages->m_pleDBFileName);
    // custom verification of DB file name
    QObject::connect(pPropPages,SIGNAL(DBFileNameEditingFinished()),SLOT(onDBFileNameEditingFinished()));
    QObject::connect(pPropPages,SIGNAL(DBValidationFailed()),SLOT(onDBValidationFailed()));

    pPropPages->m_ppbChoose=new QPushButton(QIcon(":/Resources/open.ico"),"Choose");
    QObject::connect(pPropPages->m_ppbChoose,SIGNAL(clicked()),pPropPages,SIGNAL(chooseSqliteFile()));
    QObject::connect(pPropPages,SIGNAL(chooseSqliteFile()),SLOT(onSQLiteFileChoose()));

    pHLayout->addWidget(pPropPages->m_ppbChoose);
    pVLayout->addLayout(pHLayout);
    QHBoxLayout *pHLayout01=new QHBoxLayout;
    pPropPages->m_plbDBValidationErr = new QLabel;
    pPropPages->m_plbDBValidationErr->setFont(QFont());
    QPalette qpDBValidationErr = pPropPages->m_plbDBValidationErr->palette();
    qpDBValidationErr.setColor(QPalette::WindowText,QColor(Qt::darkRed));
    pPropPages->m_plbDBValidationErr->setPalette(qpDBValidationErr);
    QFont qfDBValidationErr("Times", 10, QFont::Bold);
    pPropPages->m_plbDBValidationErr->setFont(qfDBValidationErr);
    pHLayout01->addWidget(pPropPages->m_plbDBValidationErr);
    pVLayout->addLayout(pHLayout01);
    pVLayout->addStretch();
    pWidget->setLayout(pVLayout);

    pTabWidget->insertTab(iIdx,pWidget,QSQLMODEL_PROP_TAB_CAPTION);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSqlModel::propChanged(QObject *pPropDlg) {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (pPropDlg);
    if (!changeDatabaseName(pPropPages->m_pleDBFileName->text())) {
        qDebug() << "changeDatabaseName() failed";
    }
}
//======================================================================================================
//
//======================================================================================================
void QSqlModel::startTransaction() {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) {
        qDebug() << "startTransaction(): database is not open";
        return;
    }
    db.transaction();
    return;
}
//======================================================================================================
//
//======================================================================================================
void QSqlModel::commitTransaction() {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) {
        qDebug() << "commitTransaction(): database is not open";
        return;
    }
    db.commit();
    return;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::openDatabase() {

    // obtain the defaultConnection and close it
    QSqlDatabase db=QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    // defaultConnection must be valid
    if (!db.isValid()) {
        qDebug() << "openDatabase(): defaultConnection is not valid";
        return false;
    }
    // close the database if currently open
    if (db.isOpen()) db.close();
    // verify feature QSqlDriver::LastInsertId is supported
    bool bDBSupportsLastInsertId = db.driver()->hasFeature(QSqlDriver::LastInsertId);
    if (!bDBSupportsLastInsertId) {
        qDebug() << "openDatabase():  db.driver()->hasFeature(QSqlDriver::LastInsertId) returns false";
        return false;
    }
    //---------------------------
    // check for a valid file<<<- Correction!
    //  ... checkDatabaseName() will now be only
    //  ... used for m_pleDBFileName validation
    // QString qsErrorString;
    // if (!checkDatabaseName(m_qsDBFile,qsErrorString)) {
    //     qDebug() << "checkDatabaseName(): " << qsErrorString;
    //     return false;
    // }
    //---------------------------
    // open & verify format. Try to open
    db.setDatabaseName(m_qsDBFile); // after DB was closed - need call setDatabaseName() first
    if (!db.open()) {
        qDebug() << "openDatabase(): db.open() failed";
        return false;
    }
    // process :memory: database separately
    if (this->m_qsDBFile == ":memory:") {
        int iRetVal = createTables(db);
        if (iRetVal) {
            qDebug() << "openDatabase(): for :memory: DB createTables(db) returned " << iRetVal;
            return false;
        }
        // tables are created and valid. Prepare queries
        if (!prepareQueries()) {
            qDebug() << "openDatabase(): for :memory: DB prepareQueries() failed";
            return false;
        }
        return true;
    }
    // regular files on hard disk
    QFileInfo fiDBFile(m_qsDBFile);
    // for empty file create table anew
    if (!fiDBFile.size()) {
        int iRetVal = createTables(db);
        if (iRetVal) {
            qDebug() << "openDatabase(): for :memory: DB createTables(db) returned " << iRetVal;
            return false;
        }
    }
    // Now defaultConnection was reopened. verify its format
    if (!verifyDBFormat(db)) {
        qDebug() << "openDatabase(): ill-formatted SQLITE DB " << m_qsDBFile;
        return false;
    }
    // DB format is valid. Prepare queries
    if (!prepareQueries()) {
        qDebug() << "openDatabase(): prepareQueries() failed for DB " << m_qsDBFile;
        return false;
    }
    // qDebug() << "openDatabase(): prepareQueries() ok";
    return true;
}
//======================================================================================================
//
//======================================================================================================
void QSqlModel::closeDatabase() {
    // ":memory:" - proceed as for regular file
    // if (this->m_qsDBFile == ":memory:") return;

    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.open()) {
        qDebug() << "closeDatabase(): database is not open";
        return;
    }
    db.close();
    // qDebug() << "closeDatabase(): ok";
    return;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::checkDatabaseName(QString qsDbFileName, QString &qsErrorString) {
    // clear error message
    qsErrorString.clear();
    // nothing to do for ":memory:"
    if (qsDbFileName == ":memory:") return true;
    // empty SQLITE file name makes no sence
    if (qsDbFileName.trimmed().isEmpty()) {
        qsErrorString = "empty name of database";
        return false;
    }
    // if this DB is already open: verify DB format and return
    QFileInfo fiDbFile(qsDbFileName), fiCurrDbFileName(m_qsDBFile);
    if (fiDbFile.absoluteFilePath() == fiCurrDbFileName.absoluteFilePath()) {
        QSqlDatabase db=QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
        if (db.isOpen()) {
            if (verifyDBFormat(db)) return true;
            // otherwise ill-formatted
            qsErrorString = QString("#1 ill-formatted SQLITE DB ") + fiDbFile.fileName();
            return false;
        }
    }
    // This is another DB file. Check if the file exists && isWritable
    bool bNewFile=false;
    if (       !fiDbFile.exists()
            || !fiDbFile.isReadable()
            || !fiDbFile.isWritable()
            || !fiDbFile.isFile()
            || fiDbFile.isSymLink() ) {
        if (fiDbFile.exists() && (!fiDbFile.isFile() || fiDbFile.isSymLink())) {
            qsErrorString = QString("not a regular file ") + fiDbFile.fileName();
            return false;
        }
        QFile qfDbFile(fiDbFile.absoluteFilePath());
        if (fiDbFile.exists() && !qfDbFile.setPermissions(
                    QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                    QFileDevice::ReadUser | QFileDevice::WriteUser |
                    QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                    QFileDevice::ReadOther | QFileDevice::WriteOther)) {
            qsErrorString = QString("setPermissions() failed for file ") + fiDbFile.fileName();
            return false;
        }
        if (!fiDbFile.exists()) { // this is a new file
            if (!qfDbFile.open(QIODevice::WriteOnly)) {
                qsErrorString = QString("can't create new file ") + fiDbFile.fileName();
                return false;
            }
            qfDbFile.close();
            bNewFile=true;
        }
    }
    // The file exists, is regular. Try to connect it as SQLITE database
    QString qsTestConnection("test_SQLITE_Connection");
    bool bRetVal=true;
    // make inner scope for QSqlDatabase dbTest
    {
        QSqlDatabase dbTest = QSqlDatabase::addDatabase("QSQLITE",qsTestConnection);
        if (!dbTest.isValid()) {
            qsErrorString = QString("addDatabase failed for connection") + qsTestConnection;
            bRetVal=false;
        }
        dbTest.setDatabaseName(qsDbFileName); // we just closed the QSqlDatabase::defaultConnection
        if (bRetVal && dbTest.isOpen()) {
            qsErrorString = QString("unexpectedly open connection") + qsTestConnection;
            bRetVal=false;
        }
        if (bRetVal && !dbTest.open()) {
            qsErrorString = QString("could not open SQLITE file ") + fiDbFile.fileName();
            bRetVal=false;
        }
        if (bRetVal && bNewFile) {
            int iRet=createTables(dbTest);
            if (iRet) {
                qsErrorString = QString("createTables() failed with code %1 for file %2")
                        .arg(iRet).arg(fiDbFile.fileName());
                bRetVal=false;
            }
        }
        if (bRetVal && !verifyDBFormat(dbTest)) {
            qsErrorString = QString("#2 ill-formatted SQLITE DB ") + fiDbFile.fileName();
            bRetVal=false;
        }
        if (dbTest.isOpen()) dbTest.close();
    }
    // remove test database connection
    QSqlDatabase::removeDatabase(qsTestConnection);
    return bRetVal;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::verifyDBFormat(QSqlDatabase &db) {
    bool bOk;
    // the database must be open
    if (!db.isOpen()) {
        qDebug() << "verifyDBFormat(): database is not open";
        return false;
    }
    // check necessary tables
    QSqlQuery query("SELECT COUNT(*) AS cnt FROM sqlite_master"
                    " WHERE type='table' AND name"
                    " IN ('files','strobs','samples','dbver','signals','noisemaps')",db);
    QSqlRecord rec = query.record();
    int iFld=rec.indexOf("cnt");
    if (!query.next() || iFld==-1) return false;
    int iCnt=query.value(iFld).toInt(&bOk);
    if (!bOk) return false;
    // check DB scheme version
    QString qsVersion;
    bOk= query.exec("SELECT version FROM dbver LIMIT 1");
    rec = query.record();
    int iVer=rec.indexOf("version");
    if (query.next() && iVer!=-1) {
        qsVersion=query.value(iVer).toString();
    }
    // If DB format is ill (wrong tables, version etc) then return false
    if (iCnt!=6 || qsVersion!=DATA_BASE_VERSION) return false;
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::prepareQueries() {
    bool bOk;
    QSqlDatabase db=QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    // defaultConnection must be valid
    if (!db.isValid()) {
        qDebug() << "prepareQueries(): defaultConnection is not valid";
        return false;
    }
    // defaultConnection must be open
    if (!db.isOpen()) {
        qDebug() << "prepareQueries(): defaultConnection is not open";
        return false;
    }
    // The default connection is now open - can prepare all repeating queries
    // Beam data
    m_queryBeamData = QSqlQuery(db);
    bOk=m_queryBeamData.prepare("SELECT sa.complexdata AS complexdata FROM"
                    " samples sa WHERE sa.strobid = :strobid AND sa.beam = :beam");
    if (!bOk) return false;
    // Total strobes
    m_queryTotStrobes = QSqlQuery(db);
    bOk=m_queryTotStrobes.prepare("SELECT SUM(nstrobs) AS totStrobes FROM files");
    if (!bOk) return false;
    // File record
    m_queryFileRec01 = QSqlQuery(db);
    bOk = m_queryFileRec01.prepare("SELECT id AS filesid FROM files"
                       " WHERE timestamp=:timestamp AND filepath=:filepath;");
    if (!bOk) return false;
    // ------------
    m_queryFileRec02 = QSqlQuery(db);
    bOk = m_queryFileRec02.prepare("DELETE FROM samples"
                                   " WHERE strobid IN (SELECT id FROM strobs s"
                                   " WHERE s.fileid=:fileid);");
    if (!bOk) return false;
    // ------------
    m_queryFileRec08 = QSqlQuery(db);
    bOk = m_queryFileRec08.prepare("DELETE FROM noisemaps WHERE id IN"
                                   " (SELECT DISTINCT nm1.id FROM noisemaps nm1"
                                   "  LEFT OUTER JOIN (SELECT DISTINCT nm.id AS nmid, f.id AS fid FROM noisemaps nm"
                                   "  INNER JOIN strobs st ON st.noisemapid=nm.id"
                                   "  INNER JOIN files f ON f.id=st.fileid) tbl ON tbl.nmid=nm1.id AND tbl.fid<>:fileid"
                                   " WHERE tbl.fid IS NULL);");
    if (!bOk) return false;
    // ------------
    m_queryFileRec03 = QSqlQuery(db);
    bOk = m_queryFileRec03.prepare("DELETE FROM signals WHERE id IN"
                                   " (SELECT DISTINCT si1.id FROM signals si1"
                                   "  LEFT OUTER JOIN (SELECT DISTINCT si.id AS siid, f.id AS fid FROM signals si"
                                   "  INNER JOIN strobs st ON st.sigid=si.id"
                                   "  INNER JOIN files f ON f.id=st.fileid) tbl ON tbl.siid=si1.id AND tbl.fid<>:fileid"
                                   "  LEFT OUTER JOIN strobs st1 ON st1.sigid=si1.id"
                                   " WHERE tbl.fid IS NULL AND st1.id IS NOT NULL);");
    if (!bOk) return false;
    // ------------
    m_queryFileRec04 = QSqlQuery(db);
    bOk = m_queryFileRec04.prepare("DELETE FROM strobs"
                                   " WHERE fileid=:fileid;");
    if (!bOk) return false;
    // ------------
    m_queryFileRec05 = QSqlQuery(db);
    bOk = m_queryFileRec05.prepare("DELETE FROM files"
                 " WHERE id=:fileid;");
    if (!bOk) return false;
    // ------------
    m_queryFileRec06 = QSqlQuery(db);
    bOk = m_queryFileRec06.prepare("INSERT INTO files (timestamp,filepath,nstrobs,filever) VALUES"
                 " (:timestamp,:filepath,:nstrobs,:filever);");
    if (!bOk) return false;
    // ------------
    m_queryAddStrobe01 = QSqlQuery(db);
    bOk = m_queryAddStrobe01.prepare("INSERT INTO strobs (seqnum,ncnt,fileid,structStrobData,sigid,noisemapid) VALUES"
                 " (:seqnum,:ncnt,:fileid,:structStrobData,:sigid,:mapid);");
    if (!bOk) return false;
    // ------------
    m_queryAddStrobe03 = QSqlQuery(db);
    bOk = m_queryAddStrobe03.prepare("SELECT id, samplcnt, filedata FROM signals WHERE sigtype=:sigtype;");
    if (!bOk) return false;
    // ------------
    m_queryAddStrobe04 = QSqlQuery(db);
    bOk = m_queryAddStrobe04.prepare("INSERT INTO signals (sigtype, samplcnt, filepath, filedata, asciidata)"
                                     " VALUES (:sigtype, :samplcnt, :filepath, :filedata, :asciidata);");
    if (!bOk) return false;
    // ------------
    m_queryAddStrobe06 = QSqlQuery(db);
    bOk = m_queryAddStrobe06.prepare("UPDATE noisemaps SET map = :map"
                                     " WHERE id=:noisemapid;");
    if (!bOk) return false;
    // ------------
    m_queryAddStrobe07 = QSqlQuery(db);
    bOk = m_queryAddStrobe07.prepare("INSERT INTO noisemaps (nfft, ifilteredn)"
                                     " VALUES (:nfft, :ifilteredn);");
    if (!bOk) return false;
    // ------------
    m_queryAddStrobe08 = QSqlQuery(db);
    bOk = m_queryAddStrobe08.prepare("SELECT map, nfft, ifilteredn FROM noisemaps"
                                     " WHERE id=:noisemapid;");
    if (!bOk) return false;
    // ------------
    m_querySetNumOfRecords = QSqlQuery(db);
    bOk = m_querySetNumOfRecords.prepare("UPDATE files SET nstrobs=:nstrobs WHERE id=:fileid;");
    if (!bOk) return false;
    // Add samples
    // ------------
    m_queryAddSamples = QSqlQuery(db);
    bOk = m_queryAddSamples.prepare("INSERT INTO samples (strobid,beam,complexdata) VALUES"
                 " (:strobid,:beam,:complexdata);");
    if (!bOk) return false;

    // all queries successfully prepared - return true
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::changeDatabaseName(QString qsDbFileName) {

    // if file name is the same as the current one - nothing to do
    if (qsDbFileName == m_qsDBFile) return true;

    // obtain the defaultConnection
    QSqlDatabase db=QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    // if the defaultConnection is not valid - return
    if (!db.isValid()) {
        qDebug() << "changeDatabaseName(): defaultConnection is not valid";
        return false;
    }
    // if the defaultConnection is currently open - close it
    if (db.isOpen()) {
        db.close();
    }

    // in-memory DB - just update the field m_qsDBFile
    if (qsDbFileName == ":memory:") {
        m_qsDBFile = qsDbFileName;
        return true;
    }

    // the defaultConnection is now closed. Can safely update m_qsDBFile
    m_qsDBFile = qsDbFileName;
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::execMainSelectQuery() {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isValid()) {
        qDebug() << "execQuery(): defaultConnection is not valid";
        return false;
    }
    if (!db.isOpen()) {
        qDebug() << "execQuery(): defaultConnection is not open";
        return false;
    }
    m_mainSelectQuery = QSqlQuery("SELECT f.filever,s.id AS guid, s.seqnum,"
                    " s.ncnt, f.timestamp, s.structStrobData,"
                    " sig.filedata, sig.sigtype, sig.samplcnt,"
                    " nm.id AS noisemapid, nm.nfft, nm.ifilteredn FROM"
                    " strobs s LEFT OUTER JOIN files f ON f.id=s.fileid"
                    " LEFT OUTER JOIN signals sig ON s.sigid=sig.id"
                    " LEFT OUTER JOIN noisemaps nm ON s.noisemapid=nm.id"
                    " ORDER BY s.seqnum ASC",
                    db);
    if (!m_mainSelectQuery.exec()) {
        qDebug() << "execQuery(): m_mainSelectQuery.exec() failed";
        return false;
    }
    m_mainSelectRecord = m_mainSelectQuery.record();
    // qDebug() << "execQuery() returns true";
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::getStrobRecord(qint64 &iRecId, int &iStrob, int &iBeamCountsNum, qint64 &iTimestamp,
                               QByteArray &baStructStrobeData, quint32 &uFileVer,
                               QByteArray &baSignalSamplData, qint32 &iSigType, qint32 &iSamplCnt,
                               qint64 &iNoisemapGUID, qint32 &iNFFT, qint32 &iFilteredN) {
    bool bOk;
    if (!m_mainSelectQuery.isActive()) {
        qDebug() << "getStrobRecord(): m_query is not active";
        return false;
    }
    if (!m_mainSelectQuery.next()) {
        qDebug() << "getStrobRecord(): m_query.next() failed";
        return false;
    }
    // get values of record fields
    iRecId=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("guid")).toInt(&bOk);
    if (!bOk) return false;
    iStrob=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("seqnum")).toInt(&bOk);
    if (!bOk) return false;
    iBeamCountsNum=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("ncnt")).toInt(&bOk);
    if (!bOk) return false;
    iTimestamp=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("timestamp")).toLongLong(&bOk);
    if (!bOk) return false;
    baStructStrobeData=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("structStrobData")).toByteArray();
    if (baStructStrobeData.isEmpty()) return false;
    QString qsFileVer=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("filever")).toString();
    uFileVer=qsFileVer.toUInt(&bOk);
    if (!bOk) return false;
    baSignalSamplData=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("filedata")).toByteArray();
    if (baSignalSamplData.isEmpty()) return false;
    iSigType=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("sigtype")).toInt(&bOk);
    if (!bOk) return false;
    iSamplCnt=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("samplcnt")).toInt(&bOk);
    if (!bOk) return false;
    // baNoiseMap=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("map")).toByteArray();
    // if (baNoiseMap.isEmpty()) return false;
    iNoisemapGUID=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("noisemapid")).toLongLong(&bOk);
    if (!bOk) return false;
    iNFFT=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("nfft")).toInt(&bOk);
    if (!bOk) return false;
    iFilteredN=m_mainSelectQuery.value(m_mainSelectRecord.indexOf("ifilteredn")).toInt(&bOk);
    if (!bOk) return false;
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::getBeamData(qint64 &iRecId, int &iBeam, QByteArray &baSamples) {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return false;
    m_queryBeamData.bindValue(":strobid",iRecId);
    m_queryBeamData.bindValue(":beam",iBeam);
    if (!m_queryBeamData.exec()) return false;
    if (!m_queryBeamData.next()) return false;
    m_recBeamData=m_queryBeamData.record();
    baSamples = m_queryBeamData.value(m_recBeamData.indexOf("complexdata")).toByteArray();
    if (m_queryBeamData.next()) return false;
    return true;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::getTotStrobes(quint32 &iTotStrobes) {
    bool bOk;
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return false;
    if (!m_queryTotStrobes.exec()) return false;
    if (!m_queryTotStrobes.next()) return false;
    m_recTotStrobes = m_queryTotStrobes.record();
    int iRetVal = m_queryTotStrobes.value(m_recTotStrobes.indexOf("totStrobes")).toInt(&bOk);
    if (!bOk) return false;
    if (m_queryTotStrobes.next()) return false;
    iTotStrobes=iRetVal;
    return true;
}
//======================================================================================================
//
//======================================================================================================
//int QSqlModel::dropTables() {
//    bool bOk;
//    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
//    if (!db.isOpen()) {
//        qDebug() << "dropTables(): defaultConnection is not open";
//        return 2;
//    }
//    QSqlQuery query(db);
//    bOk= query.exec("DROP TABLE IF EXISTS signals;");
//    if (!bOk) return 3;
//    bOk= query.exec("DROP TABLE IF EXISTS samples;");
//    if (!bOk) return 4;
//    bOk= query.exec("DROP TABLE IF EXISTS strobs;");
//    if (!bOk) return 5;
//    bOk= query.exec("DROP TABLE IF EXISTS files;");
//    if (!bOk) return 6;
//    bOk= query.exec("DROP TABLE IF EXISTS dbver;");
//    if (!bOk) return 7;
//    qDebug() << "drop tables ok";
//    return 0;
//}
//======================================================================================================
//
//======================================================================================================
int QSqlModel::createTables(QSqlDatabase &db) {
    bool bOk;

    if (!db.isOpen()) return 2;
    QSqlQuery query(db);
    bOk= query.exec("CREATE TABLE dbver (id INTEGER PRIMARY KEY ASC,"
                    " version TEXT);");
    if (!bOk) return 3;
    bOk = query.prepare("INSERT INTO dbver (version) VALUES (:version);");
    if (!bOk) return 4;
    query.bindValue(":version",DATA_BASE_VERSION);
    if (!query.exec()) return 5;

    bOk= query.exec("CREATE TABLE files (id INTEGER PRIMARY KEY ASC,"
                    " timestamp BIGINT,"
                    " filepath TEXT,"
                    " nstrobs INTEGER,"
                    " filever TEXT);");
    if (!bOk) return 6;
    bOk= query.exec("CREATE TABLE strobs (id INTEGER PRIMARY KEY ASC,"
                    " seqnum INTEGER,"
                    " ncnt INTEGER,"
                    " fileid INTEGER,"
                    " structStrobData BLOB,"
                    " sigid INTEGER,"
                    " noisemapid INTEGER,"
                    " FOREIGN KEY(fileid) REFERENCES files(id),"
                    " FOREIGN KEY(noisemapid) REFERENCES noisemaps(id),"
                    " FOREIGN KEY(sigid) REFERENCES signals(id));");
    if (!bOk) return 7;
    bOk= query.exec("CREATE TABLE samples (id INTEGER PRIMARY KEY ASC,"
                    " strobid INTEGER,"
                    " beam INTEGER,"
                    " complexdata BLOB,"
                    " FOREIGN KEY(strobid) REFERENCES strobs(id));");
    if (!bOk) return 8;
    bOk= query.exec("CREATE TABLE signals (id INTEGER PRIMARY KEY ASC,"
                    " sigtype INTEGER,"
                    " samplcnt INTEGER,"
                    " filepath TEXT,"
                    " filedata BLOB,"
                    " asciidata TEXT);");
    if (!bOk) return 9;
    bOk= query.exec("CREATE TABLE noisemaps (id INTEGER PRIMARY KEY ASC,"
                    " map BLOB,"
                    " nfft INTEGER,"
                    " ifilteredn INTEGER);");
    if (!bOk) return 10;

    return 0;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::isDBOpen() {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (db.isOpen()) return true;
    return false;
}
//======================================================================================================
//
//======================================================================================================
QString QSqlModel::getDBFileName() {
    return m_qsDBFile;
}
//======================================================================================================
//
//======================================================================================================
qint64 QSqlModel::addFileRec(qint64 uTimeStamp, QString qsFilePath, int nStrobs, QString qsFileVer) {
    quint64 iFilesId;
    bool bOk;
    // qDebug() << "addFileRec(): getting db ";
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return -1;

    // qDebug() << "addFileRec(): before m_queryFileRec01";
    m_queryFileRec01.bindValue(":timestamp",uTimeStamp);
    m_queryFileRec01.bindValue(":filepath",qsFilePath);
    if (!m_queryFileRec01.exec()) return -1;
    m_recFileRec01 = m_queryFileRec01.record();
    while (m_queryFileRec01.next()) {
        iFilesId=m_queryFileRec01.value(m_recFileRec01.indexOf("filesid")).toLongLong(&bOk);
        if (!bOk) return -1;
        // qDebug() << "deleting iFilesId " << iFilesId;
        m_queryFileRec02.bindValue(":fileid",iFilesId);
        if (!m_queryFileRec02.exec()) {
            qDebug() << "DELETE FROM samples failed";
            return -1;
        }
        // m_queryFileRec08: "DELETE FROM noisemaps WHERE id IN"
        //                               " (SELECT nm.id AS noisemapid"
        //                               "  FROM noisemaps nm LEFT OUTER JOIN"
        //                               "  strobs st ON nm.id=st.noisemapid"
        //                               "  LEFT OUTER JOIN files f"
        //                               "  ON f.id=st.fileid AND f.id<>:fileid"
        //                               " WHERE f.id IS NULL);");
        // delete from noisemaps
        m_queryFileRec08.bindValue(":fileid",iFilesId);
        if (!m_queryFileRec08.exec()) {
            qDebug() << "DELETE FROM noisemaps failed";
            return -1;
        }
        // delete from signals
        m_queryFileRec03.bindValue(":fileid",iFilesId);
        if (!m_queryFileRec03.exec()) {
            qDebug() << "DELETE FROM signals failed";
            return -1;
        }
        m_queryFileRec04.bindValue(":fileid",iFilesId);
        if (!m_queryFileRec04.exec()) return -1;
        m_queryFileRec05.bindValue(":fileid",iFilesId);
        if (!m_queryFileRec05.exec()) {
            qDebug() << "DELETE FROM strobs failed";
            return -1;
        }
    }
    m_queryFileRec06.bindValue(":timestamp",uTimeStamp);
    m_queryFileRec06.bindValue(":filepath",qsFilePath);
    m_queryFileRec06.bindValue(":nstrobs",nStrobs);
    m_queryFileRec06.bindValue(":filever",qsFileVer);
    if (!m_queryFileRec06.exec()) {
        qDebug() << "INSERT INTO files failed";
        return -1;
    }
    QVariant qvFilesId = m_queryFileRec06.lastInsertId();
    iFilesId = qvFilesId.toLongLong(&bOk);
    if (!qvFilesId.isValid() || !bOk) {
        qDebug() << "INSERT INTO files failed";
        return -1;
    }
    return iFilesId;
}
//======================================================================================================
//
//======================================================================================================
qint64 QSqlModel::addStrobe(int strobeNo, int beamCountsNum, QByteArray &baStructStrobeData,
                            qint64 iFileId, qint64 iSigId, qint64 iMapId) {
    // m_queryAddStrobe01: "INSERT INTO strobs (seqnum,ncnt,fileid,structStrobData,sigid,noisemapid) VALUES"
    //                     " (:seqnum,:ncnt,:fileid,:structStrobData,:sigid,:mapid);"
    m_queryAddStrobe01.bindValue(":seqnum",strobeNo);
    m_queryAddStrobe01.bindValue(":ncnt",beamCountsNum);
    //qDebug() << "iFileId=" << iFileId;
    m_queryAddStrobe01.bindValue(":fileid",iFileId);
    m_queryAddStrobe01.bindValue(":structStrobData",baStructStrobeData);
    m_queryAddStrobe01.bindValue(":sigid",iSigId);
    m_queryAddStrobe01.bindValue(":mapid",iMapId);
    //qDebug() << "query.exec(): INSERT";
    if (!m_queryAddStrobe01.exec()) {
        qDebug() << "INSERT query failed: m_queryAddStrobe01.exec()";
        return -1;
    }
    QVariant qvLastInsertId = m_queryAddStrobe01.lastInsertId();
    bool bOk;
    qint64 iStrobId=qvLastInsertId.toLongLong(&bOk);
    if (!qvLastInsertId.isValid() || !bOk) {
        qDebug() << "INSERT INTO strobs failed";
        return -1;
    }
    return iStrobId;
}
//======================================================================================================
//
//======================================================================================================
qint64 QSqlModel::addSignal(int iSignalType, int iSignalLen,
                            QByteArray &baSignalFileData, QByteArray &baSignalFileText,
                            QString qsSignalFileName /* =QString() */,
                            bool *pbOk /* = NULL */) {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return -1;
    // seek signal of this type and return it's GUID on success
    qint64 iSigId;
    QByteArray baSigDataInDB;
    bool bOkLocal;
    iSigId = seekSignal(iSignalType, iSignalLen, baSigDataInDB, &bOkLocal);
    // signal record found. It matches baSignalFileData if this bytearray was provided
    if (bOkLocal && (baSignalFileData.isEmpty() || baSignalFileData==baSigDataInDB)) {
        if (pbOk) *pbOk = true;
        return iSigId;
    }
    // no signals of this type were found - add new record and obtain its id
    // m_queryAddStrobe04:
    //    "INSERT INTO signals "
    //    "(sigtype, samplcnt, filepath, filedata, asciidata)"
    //    " VALUES (:sigtype, :samplcnt, :filepath, :filedata, :asciidata);");
    m_queryAddStrobe04.bindValue(":sigtype",iSignalType);
    m_queryAddStrobe04.bindValue(":samplcnt",iSignalLen);
    m_queryAddStrobe04.bindValue(":filepath",qsSignalFileName);
    m_queryAddStrobe04.bindValue(":filedata",baSignalFileData);
    m_queryAddStrobe04.bindValue(":asciidata",baSignalFileText);
    if (!m_queryAddStrobe04.exec()) {
        qDebug() << "addSignal: m_queryAddStrobe04.exec() failed";
        throw RmoException(QString("addStrobe(): add signals records failed (id=%1,sigtype=%2)")
                           .arg(iSigId).arg(iSignalType));
        return -1;
    }
    // verify if the signal was added
    QVariant qvSignalsId = m_queryAddStrobe04.lastInsertId();
    bool bOk;
    iSigId = qvSignalsId.toLongLong(&bOk);
    if (!qvSignalsId.isValid() || !bOk) {
        qDebug() << "INSERT INTO signals failed";
        if (pbOk) *pbOk = false;
        return -1;
    }
    if (pbOk) *pbOk = true;
    return iSigId;
}
//======================================================================================================
//
//======================================================================================================
qint64 QSqlModel::seekSignal(int iSignalType, int iSignalLen,
                             QByteArray &baSigFileDataOut,
                             bool *pbOk /* = NULL */) {
    bool bOkLocal;
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return -1;
    // check for existing records with sigtype=iSignalType
    m_queryAddStrobe03.bindValue(":sigtype",iSignalType);
    if (!m_queryAddStrobe03.exec()) {
        qDebug() << "m_queryAddStrobe03.exec() failed";
        return -1;
    }
    m_recAddStrobe03 = m_queryAddStrobe03.record();
    quint32 uNRec=0;
    qint64 iSigGUID=-1;
    int iLen;
    bool bIsEqualRec=false;
    baSigFileDataOut.clear();
    while (m_queryAddStrobe03.next()) {
        uNRec++;
        baSigFileDataOut = m_queryAddStrobe03.value(m_recAddStrobe03.indexOf("filedata")).toByteArray();
        if (baSigFileDataOut.isNull()) {
            throw RmoException(QString("seekSignal(): filedata is NULL"));
            return -1;
        }
        iSigGUID = m_queryAddStrobe03.value(m_recAddStrobe03.indexOf("id")).toLongLong(&bOkLocal);
        if (!bOkLocal) {
            throw RmoException(QString("seekSignal(): signals record incorrect (sigtype=%1)").arg(iSignalType));
            return -1;
        }
        iLen = m_queryAddStrobe03.value(m_recAddStrobe03.indexOf("samplcnt")).toInt(&bOkLocal);
        if (!bOkLocal) {
            throw RmoException(QString("seekSignal(): signals record incorrect (id=%1,sigtype=%2)")
                               .arg(iSigGUID).arg(iSignalType));
            return -1;
        }
        // if signal record with the same type and length was found - raise bIsEqualRec flag
        if (iLen == iSignalLen) {
            bIsEqualRec = true;
        }
    }
    // qDebug() << "Obtained from DB: iSigGUID=" << iSigGUID << " iLen=" << iLen << " uNRec=" << uNRec;
    // if same signal was found in signals table - return its id
    if (uNRec == 1 && bIsEqualRec) {
        if (pbOk) *pbOk = true;
        return iSigGUID;
    }
    // if signal of type iSignalType was found, but length is incorrect
    if (uNRec == 1 && !bIsEqualRec) {
        if (pbOk) *pbOk = false;
        throw RmoException(QString("addStrobe(): one signal found for sigtype=%1 but length=%2 (expected %3)")
                           .arg(iSignalType).arg(iLen).arg(iSignalLen));
        return -1;
    }
    // multiple signals of type iSignalType were found. This is error
    if (uNRec > 1) {
        if (pbOk) *pbOk = false;
        throw RmoException(QString("addStrobe(): %1 signals found for sigtype=%2")
                           .arg(uNRec).arg(iSignalType));
        return -1;
    }
    // no signals of this type were found - *pbOk=false
    if (pbOk) *pbOk = false;
    return -1;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::updateNoiseMap(qint64 iNoiseMapGUID, QByteArray &baNoiseMap) {
    // get QSqlDatabase::defaultConnection
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return false;
    // update table noisemaps with BLOB map
    m_queryAddStrobe06.bindValue(":noisemapid", iNoiseMapGUID);
    m_queryAddStrobe06.bindValue(":map", baNoiseMap);
    if (!m_queryAddStrobe06.exec()) {
        qDebug() << "m_queryAddStrobe06.exec() failed";
        return false;
    }
    return true;
}
//======================================================================================================
//
//======================================================================================================
qint64 QSqlModel::addNewNoiseMap(int NFFT, int iFilteredN, bool *pbOk /* = NULL */) {
    // clear bOk flag
    if (pbOk) *pbOk = false;
    // get QSqlDatabase::defaultConnection
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return -1;
    m_queryAddStrobe07.bindValue(":nfft",NFFT);
    m_queryAddStrobe07.bindValue(":ifilteredn",iFilteredN);
    if (!m_queryAddStrobe07.exec()) {
        qDebug() << "INSERT INTO noisemaps failed";
        return -1;
    }
    QVariant qvNoiseMapGUID = m_queryAddStrobe07.lastInsertId();
    bool bOkLocal;
    qint64 iNoiseMapGUID = qvNoiseMapGUID.toLongLong(&bOkLocal);
    if (!qvNoiseMapGUID.isValid() || !bOkLocal) {
        qDebug() << "INSERT INTO noisemaps failed #2";
        return -1;
    }
    if (pbOk) *pbOk = true;
    return iNoiseMapGUID;
}
//======================================================================================================
//
//======================================================================================================
QByteArray QSqlModel::getNoiseMap(qint64 iNoiseMapGUID) {
    QByteArray baRetVal;
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return baRetVal;

    // m_queryAddStrobe08: "SELECT map, nfft, ifilteredn FROM noisemaps"
    //                     " WHERE id=:noisemapid;"
    m_queryAddStrobe08.bindValue(":noisemapid", iNoiseMapGUID);
    if (!m_queryAddStrobe08.exec()) {
        qDebug() << "m_queryAddStrobe08.exec() failed";
        return baRetVal;
    }
    m_recAddStrobe08 = m_queryAddStrobe08.record();
    qint32 iNRec = 0;
    while (m_queryAddStrobe08.next()) {
        iNRec++;
        if (iNRec > 1) {
            qDebug() << "QSqlModel::getNoiseMap() multiple records found";
            return baRetVal;
        }
        bool bOk;
        qint32 iNFFT = m_queryAddStrobe08.value(m_recAddStrobe08.indexOf("nfft")).toInt(&bOk);
        if (!bOk) {
            qDebug() << "QSqlModel::getNoiseMap() failed at nfft";
            return baRetVal;
        }
        qint32 iFilteredN = m_queryAddStrobe08.value(m_recAddStrobe08.indexOf("ifilteredn")).toInt(&bOk);
        if (!bOk) {
            qDebug() << "QSqlModel::getNoiseMap() failed at ifilteredn";
            return baRetVal;
        }
        QByteArray baMap = m_queryAddStrobe08.value(m_recAddStrobe08.indexOf("map")).toByteArray();
        if (baMap.size() != iNFFT*iFilteredN*sizeof(double)) {
            qDebug() << "QSqlModel::getNoiseMap() baMap.size() check failed";
            return baRetVal;
        }
        return baMap;
    }
    return baRetVal;
}
//======================================================================================================
//
//======================================================================================================
bool QSqlModel::setNumberOfRecords(int nRec, qint64 iFileId) {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return false;

    m_querySetNumOfRecords.bindValue(":nstrobs",nRec);
    m_querySetNumOfRecords.bindValue(":fileid",iFileId);
    if (!m_querySetNumOfRecords.exec()) {
        qDebug() << "setNumberOfRecords(): m_querySetNumOfRecords.exec() failed";
        return false;
    }
    return true;
}
//======================================================================================================
//
//======================================================================================================
int QSqlModel::addSamples(qint64 iStrobId, int iBeam, char *pSamples, int iSize) {
    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::defaultConnection,false);
    if (!db.isOpen()) return -1;

    m_queryAddSamples.bindValue(":strobid",iStrobId);
    m_queryAddSamples.bindValue(":beam",iBeam);
    m_queryAddSamples.bindValue(":complexdata",QByteArray(pSamples,iSize));
    if (!m_queryAddSamples.exec()) return 1;
    return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSqlModel::onSQLiteFileChoose() {
    // qDebug() << "In onSQLiteFileChoose()";
    QPropPages *pPropPages = qobject_cast<QPropPages *> (QObject::sender());

    QFileDialog dialog(pPropPages);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("SQLite db file (*.db *.sqlite *.sqlite3)"));
    // if current DB file exists - set its directory in dialog
    QString qsCurrFile = pPropPages->m_pleDBFileName->text();
    QFileInfo fiCurrFile(qsCurrFile);
    if (qsCurrFile.left(1)!=":" && fiCurrFile.absoluteDir().exists()) {
        dialog.setDirectory(fiCurrFile.absoluteDir());
    }
    else { // otherwise set current directory in dialog
        dialog.setDirectory(QDir::current());
    }
    if (dialog.exec()) {
        QStringList qsSelection=dialog.selectedFiles();
        if (qsSelection.size() != 1) {
            pPropPages->m_pleDBFileName->setFocus();
            return;
        }
        QString qsSelFilePath=qsSelection.at(0);
        QFileInfo fiSelFilePath(qsSelFilePath);
        if (fiSelFilePath.isFile() && fiSelFilePath.isReadable())	{
            QDir qdCur = QDir::current();
            QDir qdSelFilePath = fiSelFilePath.absoluteDir();
            if (qdCur.rootPath() == qdSelFilePath.rootPath()) { // same Windows drives
                pPropPages->m_pleDBFileName->setText(qdCur.relativeFilePath(fiSelFilePath.absoluteFilePath()));
            }
            else { // different Windows drives
                pPropPages->m_pleDBFileName->setText(fiSelFilePath.absoluteFilePath());
            }
        }
        else {
            pPropPages->m_pleDBFileName->setText("error");
        }
    }
    pPropPages->m_pleDBFileName->setFocus();
    pPropPages->m_plbDBValidationErr->clear();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSqlModel::onDBFileNameEditingFinished() {
    // qDebug() << "In onDBFileNameEditingFinished()" ;
    QPropPages *pPropPages = qobject_cast<QPropPages *> (QObject::sender());
    // allow QDialog accept() by default
    pPropPages->m_bMayAccept=true;
    pPropPages->m_plbDBValidationErr->clear();
    QString qsErrorMsg;
    QString qsDbFileName = pPropPages->m_pleDBFileName->text();
    if (!checkDatabaseName(qsDbFileName,qsErrorMsg)) {
        pPropPages->m_bMayAccept=false;
        pPropPages->m_plbDBValidationErr->setText(qsErrorMsg);
        // return focus to m_pleDBFileName
        QTimer::singleShot(0,pPropPages,SLOT(onDBValidationFailed()));
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void QSqlModel::onDBValidationFailed() {
    QPropPages *pPropPages = qobject_cast<QPropPages *> (QObject::sender());

    QTabWidget *pTabWidget = pPropPages->m_pTabWidget;

    // if m_pleDBFileName, m_ppbCancel or m_pleDBFileName has focus - do nothing
    if (pPropPages->m_ppbCancel->hasFocus()) return;
    if ((pTabWidget->tabText(pTabWidget->currentIndex()) == QSQLMODEL_PROP_TAB_CAPTION)
        && (pPropPages->m_pleDBFileName->hasFocus()
         || pPropPages->m_ppbChoose->hasFocus())) return;
    // ensure DB tab is current tab
    if (pTabWidget->tabText(pTabWidget->currentIndex()) != QSQLMODEL_PROP_TAB_CAPTION) {
        for (qint32 idx=0; idx<pTabWidget->count(); idx++) {
            if (pTabWidget->tabText(idx) == QSQLMODEL_PROP_TAB_CAPTION) {
                pTabWidget->setCurrentIndex(idx);
            }
        }
        if (pTabWidget->tabText(pTabWidget->currentIndex()) != QSQLMODEL_PROP_TAB_CAPTION)
            qDebug() << "Could not find tab " << QSQLMODEL_PROP_TAB_CAPTION;
    }
    pPropPages->m_pleDBFileName->setFocus();
}
