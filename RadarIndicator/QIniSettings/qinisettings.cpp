#include "qinisettings.h"

#include <QtGlobal>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

bool QIniSettings::bInit=false;
QString QIniSettings::m_qsIniFileName="RadarIndicator.xml";
QSettings * QIniSettings::m_pSettings = NULL;
QMap<QString,QVariant> QIniSettings::m_qmParamDefaults;

//========================================================================
//
//========================================================================
QIniSettings::~QIniSettings() {
    if (m_pSettings) delete m_pSettings;
}

//========================================================================
//
//========================================================================
QIniSettings& QIniSettings::getInstance() {
    if (!bInit) {
        bInit=true;
        openIniSource();
    }
    static QIniSettings instance;
    return instance;
}
//========================================================================
//
//========================================================================
void QIniSettings::openIniSource() {
    // application settings
    QFile qfSettings(QDir::current().absolutePath()+"/"+m_qsIniFileName);
    // detect de facto open mode for settings xml file
    QIODevice::OpenMode omSettings=QIODevice::NotOpen;
    if (qfSettings.exists()) {
        if (!qfSettings.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                    | QFileDevice::ReadUser | QFileDevice::WriteUser
                    | QFileDevice::ReadGroup | QFileDevice::WriteGroup
                    | QFileDevice::ReadOther | QFileDevice::WriteOther)) {
            qDebug() << "qfSettings.setPermissions failed!";
        }
        if (qfSettings.open(QIODevice::ReadWrite)) {
            omSettings=QIODevice::ReadWrite;
            qfSettings.close();
        }
        else if (qfSettings.open(QIODevice::ReadOnly)) {
            omSettings=QIODevice::ReadOnly;
            qfSettings.close();
        }
    }
    // regisger xml format
    QSettings::Format XmlFormat = QSerial::registerXmlFormat();
    if (omSettings!=QIODevice::NotOpen && XmlFormat != QSettings::InvalidFormat) {
        QFileInfo fiSettings(qfSettings);
        if (omSettings==QIODevice::ReadOnly) { // for readOnly settings file - use registry
            qDebug() << "omSettings==QIODevice::ReadOnly: possibly CD-ROM?";
            QSettings tmpSettings(fiSettings.absoluteFilePath(),XmlFormat);
            m_pSettings = new QSettings(COMPANY_NAME,APPLICATION_NAME);
            // on fresh OS -- just copy from xml into registry
            if (!m_pSettings->childGroups().contains(SETTINGS_GROUP_NAME)) {
                tmpSettings.beginGroup(SETTINGS_GROUP_NAME);
                m_pSettings->beginGroup(SETTINGS_GROUP_NAME);
                QStringList qslKeys = tmpSettings.allKeys();
                for (int i=0; i<qslKeys.size(); i++) {
                    m_pSettings->setValue(qslKeys.at(i),tmpSettings.value(qslKeys.at(i)));
                }
                m_pSettings->endGroup();
            }
        }
        else { // ReadWrite open mode
            m_pSettings = new QSettings(fiSettings.absoluteFilePath(),XmlFormat);
        }
    }
    else { // something is wrong with file access
        if (XmlFormat == QSettings::InvalidFormat) { // xml format registering failed
            qDebug() << "XmlFormat == QSettings::InvalidFormat";
        }
        else { // no access to xml file
            qDebug() << "no settings xml file found! Using default settings";
        }
        qDebug() << "XmlFormat == QSettings::InvalidFormat: " << QString(XmlFormat == QSettings::InvalidFormat);
        m_pSettings = new QSettings(COMPANY_NAME,APPLICATION_NAME);
    }
}
//========================================================================
//
//========================================================================
QVariant QIniSettings::value(const QString &key, STATUS_CODES &iStatus) {
    if (!m_pSettings) {
        iStatus=QIniSettings::INIT_ERROR;
        return QVariant();
    }
    m_pSettings->beginGroup(SETTINGS_GROUP_NAME);
    bool bHasKey = m_pSettings->contains(key);
    m_pSettings->endGroup();
    if (bHasKey) {
        iStatus=QIniSettings::READ_VALID;
        m_pSettings->beginGroup(SETTINGS_GROUP_NAME);
        QVariant retval = m_pSettings->value(key);
        m_pSettings->endGroup();
        return retval;
    }
    if (m_qmParamDefaults.count(key)>1) {
        iStatus=QIniSettings::INIT_ERROR;
        return QVariant();
    }
    if (m_qmParamDefaults.contains(key)) {
        iStatus=QIniSettings::READ_DEFAULT;
        return m_qmParamDefaults.value(key);
    }
    // else key was not found
    iStatus=QIniSettings::KEY_NOT_FOUND;
    return QVariant();
}
//========================================================================
//
//========================================================================
void QIniSettings::setDefault(const QString &key, const QVariant &value) {
    m_qmParamDefaults.insert(key,value);
}
//========================================================================
//
//========================================================================
QVariant QIniSettings::getDefault(const QString &key) {
    return m_qmParamDefaults.value(key);
}
//========================================================================
//
//========================================================================
void QIniSettings::setDefault(const QMap<QString,QVariant> &qmNewDefaults) {
    QMapIterator<QString,QVariant> i(qmNewDefaults);
    while (i.hasNext()) {
        i.next();
        QString qsKey=i.key();
        if (m_qmParamDefaults.contains(qsKey)) m_qmParamDefaults.remove(qsKey);
        m_qmParamDefaults.insert(qsKey,i.value());
    }
}
//========================================================================
//
//========================================================================
void QIniSettings::setValue(const QString &key, const QVariant &value) {
    if (!m_pSettings) return;
    m_pSettings->beginGroup(SETTINGS_GROUP_NAME);
    m_pSettings->setValue(key,value);
    m_pSettings->endGroup();
}
//========================================================================
//   different setters for numbers with rounding
//========================================================================
void QIniSettings::setNum(const QString &key, int n, int base /*= 10*/){
    QByteArray ba;
    ba.setNum(n,base);
    this->setValue(key,ba);
}
//========================================================================
//
//========================================================================
void QIniSettings::setNum(const QString &key, ushort n, int base /*= 10*/){
    QByteArray ba;
    ba.setNum(n,base);
    this->setValue(key,ba);
}
//========================================================================
//
//========================================================================
void QIniSettings::setNum(const QString &key, short n, int base /*= 10*/){
    QByteArray ba;
    ba.setNum(n,base);
    this->setValue(key,ba);
}
//========================================================================
//
//========================================================================
void QIniSettings::setNum(const QString &key, uint n, int base /*= 10*/){
    QByteArray ba;
    ba.setNum(n,base);
    this->setValue(key,ba);
}
//========================================================================
//
//========================================================================
void QIniSettings::setNum(const QString &key, qlonglong n, int base /*= 10*/){
    QByteArray ba;
    ba.setNum(n,base);
    this->setValue(key,ba);
}
//========================================================================
//
//========================================================================
void QIniSettings::setNum(const QString &key, qulonglong n, int base /*= 10*/){
    QByteArray ba;
    ba.setNum(n,base);
    this->setValue(key,ba);
}
//========================================================================
//
//========================================================================
void QIniSettings::setNum(const QString &key, float n, char f /*= 'g'*/, int prec /*= 6*/){
    QByteArray ba;
    ba.setNum(n,f,prec);
    this->setValue(key,ba);
}
//========================================================================
//
//========================================================================
void QIniSettings::setNum(const QString &key, double n, char f /*= 'g'*/, int prec /*= 6*/){
    QByteArray ba;
    ba.setNum(n,f,prec);
    this->setValue(key,ba);
}
