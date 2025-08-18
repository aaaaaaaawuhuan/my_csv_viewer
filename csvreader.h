#ifndef CSVREADER_H
#define CSVREADER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>

struct CsvInitializationData {
    QVector<QString> headers;
    qint64 totalRows;
    QString delimiter;
    QMap<qint64, qint64> rowPositions; // 行号到文件位置的映射
};

class CsvReader : public QObject
{
    Q_OBJECT
public:
    explicit CsvReader(QObject *parent = nullptr);
    
    CsvInitializationData getInitializeData(const QString &fileName);

private:
    QString m_FileName;

signals:
    void initializationDataReady(const QVector<QString> &headers);

public slots:
    void init(const QString &fileName);
    void processFile(const QString &fileName);

private slots:
};

#endif // CSVREADER_H