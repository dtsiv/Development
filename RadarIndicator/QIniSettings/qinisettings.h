#ifndef QINISETTINGS_H
#define QINISETTINGS_H

#include <QtCore>
#include <QSettings>
#include "qserial.h"

#define RADAR_INDICATOR_INI_FILE                 "RadarIndicator.xml"

class QIniSettings {
public:
    enum STATUS_CODES {
        READ_VALID      = 0,
        READ_DEFAULT    = 1,
        KEY_NOT_FOUND   = 2,
        INIT_ERROR      = 3
    };
	
// Singleton construction for QIniSettings class
private: 
    QIniSettings() {}
    ~QIniSettings();
    QIniSettings( const QIniSettings&);
    QIniSettings& operator=( QIniSettings& );

public:
    static QIniSettings& getInstance();

// payload part of the class
private:

    static void openIniSource();

    static bool bInit;
    static QSettings *m_pSettings;
    static QString m_qsIniFileName;
    static QMap<QString,QVariant> m_qmParamDefaults;

public:
    QVariant value(const QString &key, STATUS_CODES &iStatus);
    void setDefault(const QString &key, const QVariant &value);
    void setDefault(const QMap<QString,QVariant> &qmNewDefaults);
    QVariant getDefault(const QString &key);
    void setValue(const QString &key, const QVariant &value);

    // difference setters for numbers with rounding
    void setNum(const QString &key, int n, int base = 10);
    void setNum(const QString &key, ushort n, int base = 10);
    void setNum(const QString &key, short n, int base = 10);
    void setNum(const QString &key, uint n, int base = 10);
    void setNum(const QString &key, qlonglong n, int base = 10);
    void setNum(const QString &key, qulonglong n, int base = 10);
    void setNum(const QString &key, float n, char f = 'g', int prec = 6);
    void setNum(const QString &key, double n, char f = 'g', int prec = 6);
};

#endif // QINISETTINGS_H
