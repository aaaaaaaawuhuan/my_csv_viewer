#include "mainwindow.h"
#include "csvreader.h"
#include "tablemodel.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char *argv[])
{
    qRegisterMetaType<CsvInitializationData>("CsvInitializationData");
    qRegisterMetaType<CsvRowData>("CsvRowData");
    qRegisterMetaType<Encoding>("Encoding");
    qRegisterMetaType<QVector<QString>>("QVector<QString>");
    qRegisterMetaType<QMap<qint64, qint64>>("QMap<qint64, qint64>");
    qRegisterMetaType<QMap<QString, qint64>>("QMap<QString, qint64>");
    qRegisterMetaType<QVector<QStringList>>("QVector<QStringList>");
    
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}