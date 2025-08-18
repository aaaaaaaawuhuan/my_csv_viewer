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

// 添加一个新的结构体来存储读取的数据
struct CsvRowData {
    QVector<QStringList> rows;  // 数据行
    QMap<QString, qint64> performanceData; // 性能数据
};

class CsvReader : public QObject
{
    Q_OBJECT
public:
    explicit CsvReader(QObject *parent = nullptr);
    
    CsvInitializationData getInitializeData(const QString &fileName);
    CsvRowData getRowsData(const QString &fileName, qint64 startRow, qint64 rowCount); // 添加读取数据行的方法
    const QMap<QString, qint64>& getPerformanceData() const; // 添加获取性能数据的公共方法

private:
    QString m_FileName;
    CsvInitializationData m_initData; // 保存初始化数据
    QElapsedTimer m_timer; // 计时器
    QMap<QString, qint64> m_performanceData; // 性能数据
    void startTiming(const QString &operation);
    void endTiming(const QString &operation);
    bool isFileChanged(const QString &fileName); // 检查文件是否发生变化
    QStringList parseCsvLine(const QString &line, const QString &delimiter); // 添加CSV行解析函数

signals:
    void initializationDataReady(const QVector<QString> &headers);
    void rowDataReady(const CsvRowData &rowData, qint64 startRow); // 添加数据行读取完成信号

public slots:
    void init(const QString &fileName);
    void processFile(const QString &fileName);
    void readRows(qint64 startRow, qint64 rowCount); // 添加读取数据行的槽函数

private slots:
};

#endif // CSVREADER_H