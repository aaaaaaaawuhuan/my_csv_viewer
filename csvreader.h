#ifndef CSVREADER_H
#define CSVREADER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QElapsedTimer>

struct CsvInitializationData {
    QVector<QString> headers;
    qint64 totalRows;
    QString delimiter;
    QMap<qint64, qint64> rowPositions; // 行号到文件位置的映射
    QMap<QString, qint64> performanceData; // 性能数据
};

class CsvReader : public QObject
{
    Q_OBJECT
public:
    explicit CsvReader(QObject *parent = nullptr);
    
    CsvInitializationData getInitializeData(const QString &fileName);
    const QMap<QString, qint64>& getPerformanceData() const; // 添加获取性能数据的公共方法

private:
    QString m_FileName;
    QElapsedTimer m_timer; // 计时器
    QMap<QString, qint64> m_performanceData; // 性能数据
    void startTiming(const QString &operation);
    void endTiming(const QString &operation);

signals:
    void initializationDataReady(const QVector<QString> &headers);

public slots:
    void init(const QString &fileName);
    void processFile(const QString &fileName);

private slots:
};

#endif // CSVREADER_H