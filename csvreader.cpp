#include "csvreader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

CsvReader::CsvReader(QObject *parent)
    : QObject{parent}
    , m_FileName("")
{

}

void CsvReader::startTiming(const QString &operation)
{
    m_timer.start();
    m_performanceData[operation] = 0; // 初始化
}

void CsvReader::endTiming(const QString &operation)
{
    if (m_timer.isValid()) {
        qint64 elapsed = m_timer.elapsed();
        m_performanceData[operation] = elapsed;
    }
}

const QMap<QString, qint64>& CsvReader::getPerformanceData() const
{
    return m_performanceData;
}

CsvInitializationData CsvReader::getInitializeData(const QString &fileName)
{
    CsvInitializationData data;
    data.totalRows = 0;
    
    startTiming("读取文件");
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file:" << fileName;
        return data;
    }
    
    QTextStream in(&file);
    
    // 读取表头
    if (!in.atEnd()) {
        qint64 pos = file.pos(); // 记录表头位置
        QString headerLine = in.readLine();
        // 简单地以逗号分割表头，实际应用中可能需要更复杂的解析
        data.headers = headerLine.split(",");
        data.rowPositions[0] = pos; // 记录表头行位置
    }
    
    // 计算总行数并记录每行位置
    while (!in.atEnd()) {
        qint64 pos = file.pos(); // 记录每行起始位置
        in.readLine();
        data.rowPositions[data.totalRows + 1] = pos; // 记录第n行位置
        data.totalRows++;
    }
    
    // 加上表头行
    data.totalRows++;
    
    file.close();
    
    // 默认分隔符为逗号
    data.delimiter = ",";
    
    // 复制性能数据
    data.performanceData = m_performanceData;
    
    endTiming("读取文件");
    
    return data;
}

void CsvReader::init(const QString &fileName)
{
    startTiming("初始化");
    m_FileName = fileName;
    // 获取初始化数据
    CsvInitializationData data = getInitializeData(fileName);
    // 发送表头数据给主窗口
    emit initializationDataReady(data.headers);
    endTiming("初始化");
}

void CsvReader::processFile(const QString &fileName)
{
    // TODO: 实现文件处理逻辑
    // 这里将添加处理CSV文件的代码
    Q_UNUSED(fileName)
}