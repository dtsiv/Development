#include "qserial.h"

#include <QXmlStreamReader>
#include <QDataStream>
#include <QtDebug>
#include <QStringList>

QSettings::Format QSerial::m_ftXml=QSettings::InvalidFormat;

QSerial::QSerial(QString &qsBase64) {
    m_qsBase64 = qsBase64;
	m_baData.append(qsBase64); // conversion using toAscii()
	m_baData = QByteArray::fromBase64(m_baData);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSerial::~QSerial(void) {}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSettings::Format QSerial::registerXmlFormat() {
	if (m_ftXml!=QSettings::InvalidFormat) return m_ftXml;

	m_ftXml = QSettings::registerFormat("xml", &QSerial::readXmlFile, &QSerial::writeXmlFile, Qt::CaseSensitive);
	return(m_ftXml);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSerial::readXmlFile(QIODevice &device, QSettings::SettingsMap &map) {
    QXmlStreamReader xmlReader(&device);
	while (!xmlReader.atEnd()) {
		xmlReader.readNext();
		if (xmlReader.isStartElement()) {
			if (xmlReader.name().toString()==SETTINGS_GROUP_NAME) {
				QString qsKey;
				while (xmlReader.readNext()!=QXmlStreamReader::Invalid) {
					if (xmlReader.isStartElement()) {
						qsKey=xmlReader.name().toString();
					}
					if (xmlReader.isEndDocument()) {
						break;
					}
					if (xmlReader.isEndElement()) {
						continue;
					}
					if (xmlReader.isCharacters() && !xmlReader.isWhitespace()) {
						QString qsValue=xmlReader.text().toString();
						map.insert(SETTINGS_GROUP_NAME+QString::fromLatin1("/")+qsKey,qsValue);
						qsKey.clear();
					}
				}
			}
		}
	}
	QMapIterator<QString, QVariant> i(map);
    if (xmlReader.hasError() && xmlReader.error()!=QXmlStreamReader::PrematureEndOfDocumentError) {
		QString qsErrMsg("(line %1, col %2, chr %3) :");
		qDebug() << "QSerial: xml parsing error @" 
			<< qsErrMsg
			     .arg(xmlReader.lineNumber()) 
			     .arg(xmlReader.columnNumber()) 
			     .arg(xmlReader.characterOffset()) 
			<< xmlReader.errorString();
		return false;
	}
	return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool QSerial::writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map) {
	QXmlStreamWriter xmlWriter( &device );
	xmlWriter.setAutoFormatting( true );
 
	xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement(SETTINGS_GROUP_NAME);
	QSettings::SettingsMap::const_iterator mi = map.begin();
	for( mi; mi != map.end(); ++mi) {
		QString qsKey = mi.key();
		QStringList qslGroups = qsKey.split("/",QString::SkipEmptyParts,Qt::CaseSensitive);
		xmlWriter.writeStartElement(qslGroups.at(qslGroups.count()-1));
 		xmlWriter.writeCharacters( mi.value().toString() );
		xmlWriter.writeEndElement();
	}
	xmlWriter.writeEndDocument();
 
	return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSerial::QSerial(const QRect &r) {
	QDataStream ds(&m_baData,QIODevice::WriteOnly); 
	ds << r;
	m_qsBase64 = QString(m_baData.toBase64()); // conversion using fromAscii()
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QSerial::QSerial(const QColor &c) {
	QDataStream ds(&m_baData,QIODevice::WriteOnly); 
	int r,g,b,a;
	c.getRgb(&r,&g,&b,&a);
	ds << r << g << b << a;
	m_qsBase64 = QString(m_baData.toBase64()); // conversion using fromAscii()
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QRect QSerial::toRect(bool *pbOk) {
	if (m_baData.size() == 0) {
		if (pbOk) *pbOk=false;
		return (QRect());
	}
	QDataStream ds(m_baData);
	QRect r;
	ds >> r;
	if (pbOk) *pbOk=true;
	return (r);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QColor QSerial::toColor(bool *pbOk) {
	if (m_baData.size() == 0) {
		if (pbOk) *pbOk=false;
		return (QColor());
	}
	QDataStream ds(m_baData);
	int r,g,b,a;
	ds >> r;
	ds >> g;
	ds >> b;
	ds >> a;
	QColor c(r,g,b,a);
	if (pbOk) *pbOk=true;
	return (c);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
QString QSerial::toBase64() {
	return m_qsBase64;
}
