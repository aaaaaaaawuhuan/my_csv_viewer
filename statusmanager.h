#ifndef STATUSMANAGER_H
#define STATUSMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QElapsedTimer>
#include <QLabel>
#include <QStatusBar>

// 调试宏定义，可通过注释掉这行来关闭所有调试信息
#define STATUS_DEBUG_PRINT(msg) qDebug() << "[STATUS_MANAGER]" << msg

/**
 * @class StatusManager
 * @brief 状态栏管理器，负责集中处理状态栏显示和性能统计
 */
class StatusManager : public QObject
{
    Q_OBJECT
public:
    explicit StatusManager(QStatusBar *statusBar, QObject *parent = nullptr);
    
    /**
     * @brief 开始计时
     * @param operation 操作名称
     */
    void startTiming(const QString &operation);
    
    /**
     * @brief 结束计时并记录性能数据
     * @param operation 操作名称
     * @return 计时时长（毫秒）
     */
    qint64 endTiming(const QString &operation);
    
    /**
     * @brief 更新状态栏显示
     * @param positionInfo 当前位置信息
     * @param additionalInfo 额外信息
     */
    void updateStatusBar(const QString &positionInfo = "", const QString &additionalInfo = "");
    
    /**
     * @brief 更新文件相关的状态栏信息
     * @param fileName 文件名
     * @param startRow 起始行
     * @param visibleRows 可见行数
     * @param totalRows 总行数
     */
    void updateFileStatus(const QString &fileName, qint64 startRow, qint64 visibleRows, qint64 totalRows);
    
    /**
     * @brief 合并其他来源的性能数据
     * @param source 数据源名称
     * @param performanceData 性能数据
     */
    void mergePerformanceData(const QString &source, const QMap<QString, qint64> &performanceData);
    
    /**
     * @brief 清除所有性能数据
     */
    void clearPerformanceData();
    
    /**
     * @brief 显示临时消息
     * @param message 消息内容
     * @param timeout 显示时长（毫秒）
     */
    void showTemporaryMessage(const QString &message, int timeout = 3000);
    
    /**
     * @brief 获取指定操作的性能数据
     * @param operation 操作名称
     * @return 计时时长（毫秒），如果不存在返回-1
     */
    qint64 getPerformanceData(const QString &operation) const;
    
private:
    QStatusBar *m_statusBar; // 状态栏指针
    QElapsedTimer m_timer; // 计时器
    QMap<QString, qint64> m_performanceData; // 性能数据
    
    /**
     * @brief 格式化性能统计信息
     * @return 格式化后的性能统计字符串
     */
    QString formatPerformanceInfo() const;
};

#endif // STATUSMANAGER_H