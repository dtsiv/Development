#ifndef QSERIAL_H
#define QSERIAL_H

#include <QSettings>
#include <QRect>
#include <QColor>

#define SETTINGS_GROUP_NAME         "PARAMETERS"
#define COMPANY_NAME                "Tristan"
#define APPLICATION_NAME            "RadarIndicator"

class QSerial {
public:
	explicit QSerial(QString &qsBase64);
	explicit QSerial(const QRect &r);
	explicit QSerial(const QColor &c);
	~QSerial(void);

	QRect toRect(bool *pbOk);
	QColor toColor(bool *pbOk);
	QString toBase64();
	static QSettings::Format registerXmlFormat();

private:
    static bool readXmlFile(QIODevice &device, QSettings::SettingsMap &map);
    static bool writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map);

	QString m_qsBase64;
	QByteArray m_baData; // serialized object (using QDataStream & operator << (Type) )
	static QSettings::Format m_ftXml;
};

#endif  // QSERIAL_H
