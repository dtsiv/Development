#ifndef QPARSEMGR_H
#define QPARSEMGR_H

#include <QtGlobal>
#include <QObject>

#define     PARSE_PROGRESS_BAR_STEP         10
#define     PARSE_PROGRESS_BAR_MAX          100

class QIndicatorWindow;
class QRegFileParser;
class QPoi;
class QSqlModel;
class QTimer;

class QParseMgr {
public:
    explicit QParseMgr(QIndicatorWindow *pOwner);
    ~QParseMgr();
    void startParsing(QObject *pSender);
    void doUpdateParseProgressBar(bool bReset=false);

public:
    // quint64 m_uTot;
    // quint64 m_uDB;
    // quint64 m_uHDD;
    // quint64 m_uSam;

private:
    QIndicatorWindow *m_pOwner;
    QRegFileParser *m_pRegFileParser;
    QSqlModel *m_pSqlModel;
    QPoi *m_pPoi;
};

#endif // QPARSEMGR_H
