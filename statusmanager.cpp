#include "statusmanager.h"
#include <QApplication>
#include <QDebug>
#include <QFileInfo>

StatusManager::StatusManager(QStatusBar *statusBar, QObject *parent)
    : QObject(parent)
    , m_statusBar(statusBar)
{
    if (!m_statusBar) {
        qWarning() << "StatusManager initialized without valid QStatusBar pointer";
    }
}

void StatusManager::startTiming(const QString &operation)
{
    if (!m_statusBar) {
        STATUS_DEBUG_PRINT("Cannot start timing - no valid status bar");
        return;
    }
    
    m_timer.start();
    STATUS_DEBUG_PRINT("Started timing for operation: " + operation);
    
    // 在状态栏显示正在执行的操作
    m_statusBar->showMessage(tr("正在执行%1...").arg(operation));
}

qint64 StatusManager::endTiming(const QString &operation)
{
    if (!m_statusBar) {
        STATUS_DEBUG_PRINT("Cannot end timing - no valid status bar");
        return -1;
    }
    
    if (!m_timer.isValid()) {
        STATUS_DEBUG_PRINT("Timer not started for operation: " + operation);
        return -1;
    }
    
    qint64 elapsed = m_timer.elapsed();
    m_performanceData[operation] = elapsed;
    
    STATUS_DEBUG_PRINT(QString("Operation '%1' took %2 ms").arg(operation).arg(elapsed));
    
    // 更新状态栏显示
    updateStatusBar();
    
    return elapsed;
}

void StatusManager::updateStatusBar(const QString &positionInfo, const QString &additionalInfo)
{
    if (!m_statusBar) {
        return;
    }
    
    // 构建完整的状态栏消息
    QString message;
    
    // 添加位置信息
    if (!positionInfo.isEmpty()) {
        message = positionInfo;
    }
    
    // 添加性能统计信息
    QString performanceInfo = formatPerformanceInfo();
    if (!performanceInfo.isEmpty()) {
        if (!message.isEmpty()) {
            message += " ";
        }
        message += performanceInfo;
    }
    
    // 添加额外信息
    if (!additionalInfo.isEmpty()) {
        if (!message.isEmpty()) {
            message += " ";
        }
        message += additionalInfo;
    }
    
    // 如果没有任何信息，显示就绪状态
    if (message.isEmpty()) {
        message = tr("就绪");
    }
    
    // 更新状态栏
    m_statusBar->showMessage(message);
    
    STATUS_DEBUG_PRINT("Status bar updated: " + message);
}

void StatusManager::updateFileStatus(const QString &fileName, qint64 startRow, qint64 visibleRows, qint64 totalRows)
{
    // 构建文件位置信息
    QString positionInfo;
    if (totalRows > 0 && visibleRows > 0) {
        positionInfo = QString("显示行 %1-%2 (共%3行)")
            .arg(startRow + 1)
            .arg(qMin(startRow + visibleRows, totalRows))
            .arg(totalRows);
    }
    
    // 添加文件名信息
    QString fileInfo;
    if (!fileName.isEmpty()) {
        QFileInfo fileInfoObj(fileName);
        fileInfo = QString("[文件: %1]")
            .arg(fileInfoObj.fileName());
    }
    
    // 更新状态栏
    updateStatusBar(positionInfo, fileInfo);
}

void StatusManager::mergePerformanceData(const QString &source, const QMap<QString, qint64> &performanceData)
{
    if (performanceData.isEmpty()) {
        return;
    }
    
    // 合并性能数据
    for (auto it = performanceData.begin(); it != performanceData.end(); ++it) {
        QString key = source + "-" + it.key();
        m_performanceData[key] = it.value();
        STATUS_DEBUG_PRINT(QString("Merged performance data: %1 = %2 ms").arg(key).arg(it.value()));
    }
    
    // 更新状态栏
    updateStatusBar();
}

void StatusManager::clearPerformanceData()
{
    m_performanceData.clear();
    STATUS_DEBUG_PRINT("Performance data cleared");
}

void StatusManager::showTemporaryMessage(const QString &message, int timeout)
{
    if (m_statusBar) {
        m_statusBar->showMessage(message, timeout);
        STATUS_DEBUG_PRINT("Temporary message shown: " + message);
    }
}

qint64 StatusManager::getPerformanceData(const QString &operation) const
{
    if (m_performanceData.contains(operation)) {
        return m_performanceData[operation];
    }
    return -1;
}

QString StatusManager::formatPerformanceInfo() const
{
    if (m_performanceData.isEmpty()) {
        return "";
    }
    
    QString timingInfo = "[性能统计: ";
    for (auto it = m_performanceData.begin(); it != m_performanceData.end(); ++it) {
        if (it != m_performanceData.begin()) {
            timingInfo += ", ";
        }
        timingInfo += QString("%1: %2ms").arg(it.key()).arg(it.value());
    }
    timingInfo += "]";
    
    return timingInfo;
}