#include "csvreader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

CsvReader::CsvReader(QObject *parent)
    : QObject{parent}
    , m_FileName("")
    , m_encoding(Encoding::AutoDetect) // 默认自动检测编码
{

}

void CsvReader::setEncoding(Encoding encoding)
{
    m_encoding = encoding;
}

Encoding CsvReader::getEncoding() const
{
    return m_encoding;
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

Encoding CsvReader::detectEncoding(const QByteArray& data) const
{
    // 检查是否是UTF-8 BOM
    if (data.startsWith(QByteArray("\xEF\xBB\xBF", 3))) {
        return Encoding::UTF8;
    }
    
    // 尝试验证是否为有效的UTF-8
    bool isValidUtf8 = true;
    int i = 0;
    while (i < data.size()) {
        uchar byte = static_cast<uchar>(data[i]);
        
        // 单字节字符 (0xxxxxxx)
        if ((byte & 0x80) == 0) {
            i++;
            continue;
        }
        
        // 多字节字符
        int followingBytes = 0;
        if ((byte & 0xE0) == 0xC0) {  // 110xxxxx
            followingBytes = 1;
        } else if ((byte & 0xF0) == 0xE0) {  // 1110xxxx
            followingBytes = 2;
        } else if ((byte & 0xF8) == 0xF0) {  // 11110xxx
            followingBytes = 3;
        } else {
            isValidUtf8 = false;
            break;
        }
        
        // 检查后续字节是否符合 10xxxxxx 格式
        for (int j = 1; j <= followingBytes; j++) {
            if (i + j >= data.size() || (static_cast<uchar>(data[i + j]) & 0xC0) != 0x80) {
                isValidUtf8 = false;
                break;
            }
        }
        
        if (!isValidUtf8) {
            break;
        }
        
        i += followingBytes + 1;
    }
    
    if (isValidUtf8) {
        return Encoding::UTF8;
    } else {
        return Encoding::GBK;
    }
}

QString CsvReader::decodeData(const QByteArray& data) const
{
    // 处理UTF-8 BOM
    QByteArray rawData = data;
    if (rawData.size() >= 3 && rawData.startsWith(QByteArray("\xEF\xBB\xBF", 3))) {
        rawData = rawData.mid(3); // 移除BOM
    }
    
    // 如果是自动检测模式，先检测编码
    Encoding encoding = m_encoding;
    if (encoding == Encoding::AutoDetect) {
        encoding = detectEncoding(rawData);
    }
    
    switch (encoding) {
    case Encoding::UTF8:
        return QString::fromUtf8(rawData);
    case Encoding::GBK:
        return QString::fromLocal8Bit(rawData);
    case Encoding::ASCII:
        return QString::fromLatin1(rawData);
    default:
        return QString::fromUtf8(rawData);
    }
}

CsvInitializationData CsvReader::getInitializeData(const QString &fileName)
{
    CsvInitializationData data;
    data.totalRows = 0;
    
    startTiming("读取文件");
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) { // 移除Text标志以正确处理位置
        qDebug() << "Cannot open file:" << fileName;
        return data;
    }
    
    // 读取文件开头部分用于编码检测
    QByteArray sampleData = file.read(qMin(file.size(), static_cast<qint64>(1024 * 1024))); // 读取前1MB或整个文件
    Encoding detectedEncoding = (m_encoding == Encoding::AutoDetect) ? detectEncoding(sampleData) : m_encoding;
    
    // 重新打开文件以确保从头开始读取
    file.close();
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot reopen file:" << fileName;
        return data;
    }
    
    // 读取表头
    if (!file.atEnd()) {
        qint64 pos = file.pos(); // 记录表头位置
        QByteArray headerLine = file.readLine(); // 使用readLine而不是QTextStream
        // 解码并分割表头
        QString decodedHeader = decodeData(headerLine).trimmed();
        data.headers = parseCsvLine(decodedHeader, ",");
        data.rowPositions[0] = pos; // 记录表头行位置
    }
    
#ifdef DEBUG_PRINT
    // 计算总行数并记录每行位置
    int testRowCount = 0; // 用于限制打印的测试行数
    while (!file.atEnd() && testRowCount < 3) { // 只处理前三行数据用于测试
        qint64 pos = file.pos(); // 记录每行起始位置
        QByteArray dataLine = file.readLine();
        data.rowPositions[data.totalRows + 1] = pos; // 记录第n行位置
        data.totalRows++;
        testRowCount++;
        
        // 打印前三行数据及其位置用于测试
        QString decodedLine = decodeData(dataLine).trimmed();
        qDebug() << "数据行" << data.totalRows << "位置:" << pos << "内容:" << decodedLine;
    }
#endif
    
    // 继续处理剩余行但不打印
    while (!file.atEnd()) {
        qint64 pos = file.pos(); // 记录每行起始位置
        file.readLine();
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

CsvRowData CsvReader::getRowsData(const QString &fileName, qint64 startRow, qint64 rowCount)
{
    CsvRowData data;
    
    startTiming(QString("读取数据行 %1-%2").arg(startRow).arg(startRow + rowCount - 1));
    
    // 检查是否需要重新初始化（文件是否改变）
    if (isFileChanged(fileName) || m_initData.rowPositions.isEmpty()) {
        m_initData = getInitializeData(fileName);
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) { // 移除Text标志以正确处理位置
        qDebug() << "Cannot open file:" << fileName;
        return data;
    }
    
    // 检查起始行是否有效
    if (startRow >= m_initData.totalRows || startRow < 0) {
        qDebug() << "Invalid start row:" << startRow;
        file.close();
        return data;
    }
    
    // 定位到起始行
    if (m_initData.rowPositions.contains(startRow)) {
        if (!file.seek(m_initData.rowPositions[startRow])) {
            qDebug() << "Failed to seek to position:" << m_initData.rowPositions[startRow];
            file.close();
            return data;
        }
    } else {
        qDebug() << "Row position not found for row:" << startRow;
        file.close();
        return data;
    }
    
    // 读取指定数量的行
    qint64 rowsRead = 0;
    while (rowsRead < rowCount && !file.atEnd()) {
        QByteArray line = file.readLine();
        QString lineStr = decodeData(line).trimmed();
        // 使用更健壮的CSV解析方法
        QStringList rowData = parseCsvLine(lineStr, m_initData.delimiter);
        data.rows.append(rowData);
        rowsRead++;
    }
    
    file.close();
    
    // 复制性能数据
    data.performanceData = m_performanceData;
    
    endTiming(QString("读取数据行 %1-%2").arg(startRow).arg(startRow + rowCount - 1));
    
    return data;
}

// 新增方法：解析单行CSV数据
QStringList CsvReader::parseCsvLine(const QString &line, const QString &delimiter)
{
    QStringList result;
    bool inQuotes = false;
    QString currentField;
    int delimiterLength = delimiter.length();
    
    for (int i = 0; i < line.length(); ++i) {
        QChar ch = line[i];
        
        // 处理双引号转义
        if (ch == '"') {
            if (i + 1 < line.length() && line[i + 1] == '"') {
                // 双引号表示一个引号字符
                currentField += '"';
                i++; // 跳过下一个引号
            } else {
                // 切换引号状态
                inQuotes = !inQuotes;
            }
            continue;
        }
        
        // 检查分隔符
        if (!inQuotes && i + delimiterLength <= line.length()) {
            QString substr = line.mid(i, delimiterLength);
            if (substr == delimiter) {
                result.append(currentField);
                currentField.clear();
                i += delimiterLength - 1; // 跳过分隔符（循环会自动+1）
                continue;
            }
        }
        
        currentField += ch;
    }
    
    // 添加最后一个字段
    result.append(currentField);
    
    return result;
}

void CsvReader::init(const QString &fileName)
{
    startTiming("初始化");
    m_FileName = fileName;
    // 获取初始化数据
    m_initData = getInitializeData(fileName);
    // 发送表头数据给主窗口
    emit initializationDataReady(m_initData.headers);
    endTiming("初始化");
}

bool CsvReader::isFileChanged(const QString &fileName)
{
    // 简单实现：检查文件名是否改变
    // TODO: 更复杂的实现可以检查文件修改时间或校验和
    return m_FileName != fileName;
}

void CsvReader::processFile(const QString &fileName)
{
    // TODO: 实现文件处理逻辑
    // 这里将添加处理CSV文件的代码
    Q_UNUSED(fileName)
}

void CsvReader::readRows(qint64 startRow, qint64 rowCount)
{
    if (m_FileName.isEmpty()) {
        qDebug() << "No file opened";
        return;
    }
    
    // 获取数据行
    CsvRowData rowData = getRowsData(m_FileName, startRow, rowCount);
    // 发送数据给主窗口
    emit rowDataReady(rowData, startRow);
}

qint64 CsvReader::getTotalRows() const
{
    return m_initData.totalRows;
}
