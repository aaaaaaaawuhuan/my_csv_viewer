#ifndef TABLEMODEL_H
#define TABLEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QStringList>

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TableModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 自定义方法
    void setHeaders(const QVector<QString> &headers);
    void addRow(const QStringList &row);
    void addRows(const QVector<QStringList> &rows);
    void clear();
    void setDataWindow(const QVector<QStringList> &data, qint64 startRow); // 添加设置数据窗口的方法
    qint64 getCurrentWindowStartRow() const; // 获取当前数据窗口的起始行
    
    // 双倍窗口新增方法
    void setModelData(const QVector<QStringList> &data, qint64 startRow); // 设置完整数据（3倍大小）
    void adjustVisibleWindow(qint64 relativeStartRow); // 调整可视窗口
    qint64 getFullDataStartRow() const; // 获取完整数据的起始行号
    qint64 getVisiableStartRow() const;
    int getFullDataSize() const; // 获取完整数据的大小
    void clearDataOnly(); // 只清空数据，不清空表头
    
    // 预加载数据整合方法
    void prependPreloadedData(const QVector<QStringList> &data); // 在前面添加预加载数据
    void appendPreloadedData(const QVector<QStringList> &data);  // 在后面添加预加载数据
    bool canPrependData(qint64 requestedStartRow) const; // 检查是否可以向前预加载
    bool canAppendData(qint64 requestedEndRow) const;   // 检查是否可以向后预加载
    void maintainTripleWindowSize(); // 维持三倍窗口大小
    void setVisibleRows(int visibleRows); // 设置可视行数

private:
    QVector<QString> m_headers;  // 表头数据
    QVector<QStringList> m_fullData; // 完整数据（3倍于可视区域）
    qint64 m_fullDataStartRow; // 完整数据在文件中的起始行号
    qint64 m_visibleStartRow;  // 可视区域在完整数据中的起始行号
    qint64 m_visibleRows;      // 可视区域行数
};

#endif // TABLEMODEL_H
