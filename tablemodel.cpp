#include "tablemodel.h"
#include <QDebug>

TableModel::TableModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_fullDataStartRow(0)
    , m_visibleStartRow(0)
    , m_visibleRows(0)
{
}

int TableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    
    return m_visibleRows; // 返回可视区域的行数
}

int TableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    
    return m_headers.size();
}

QVariant TableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_visibleRows || index.column() >= m_headers.size())
        return QVariant();
    
    // 计算在完整数据中的实际行号
    int actualRow = m_visibleStartRow + index.row();
    if (actualRow >= m_fullData.size())
        return QVariant();
    
    // 检查该行是否有足够的列数据
    const QStringList& rowData = m_fullData.at(actualRow);
    if (index.column() >= rowData.size()) {
        return QVariant(); // 如果该行没有足够的列数据，则返回空
    }
    
    if (role == Qt::DisplayRole) {
        return rowData.at(index.column());
    }
    
    return QVariant();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal && section < m_headers.size()) {
            return m_headers.at(section);
        } else if (orientation == Qt::Vertical) {
            // 显示实际的行号
            return QString::number(m_fullDataStartRow + m_visibleStartRow + section + 1);
        }
    }
    
    return QAbstractTableModel::headerData(section, orientation, role);
}

void TableModel::setHeaders(const QVector<QString> &headers)
{
    beginResetModel();
    m_headers = headers;
    m_fullData.clear();
    m_fullDataStartRow = 0;
    m_visibleStartRow = 0;
    m_visibleRows = 0;
    endResetModel();
}

void TableModel::addRow(const QStringList &row)
{
    // 在双倍窗口机制下，不建议使用此方法
    // 可以保持空实现或添加调试信息
    qDebug() << "Warning: addRow is not recommended in double window mode";
}

void TableModel::addRows(const QVector<QStringList> &rows)
{
    // 在双倍窗口机制下，不建议使用此方法
    // 可以保持空实现或添加调试信息
    qDebug() << "Warning: addRows is not recommended in double window mode";
}

void TableModel::clear()
{
    beginResetModel();
    m_headers.clear();
    m_fullData.clear();
    m_fullDataStartRow = 0;
    m_visibleStartRow = 0;
    m_visibleRows = 0;
    endResetModel();
}

void TableModel::setDataWindow(const QVector<QStringList> &data, qint64 startRow)
{
    qDebug() << "设置数据窗口: 数据行数=" << data.size() << ", 起始行=" << startRow;
    
    beginResetModel();
    m_fullData = data;
    m_fullDataStartRow = startRow;
    m_visibleStartRow = 0;
    // 假设传入的data大小就是3倍的可视行数
    m_visibleRows = data.size() / 3;
    endResetModel();
    
    qDebug() << "数据窗口设置完成: 完整数据行数=" << m_fullData.size() 
             << ", 可视行数=" << m_visibleRows << ", 起始行=" << m_fullDataStartRow;
}

qint64 TableModel::getCurrentWindowStartRow() const
{
    return m_fullDataStartRow + m_visibleStartRow;
}

// 双倍窗口新增方法的实现
void TableModel::setFullData(const QVector<QStringList> &data, qint64 startRow)
{
    qDebug() << "设置完整数据: 数据行数=" << data.size() << ", 起始行=" << startRow;
    
    beginResetModel();
    m_fullData = data;
    m_fullDataStartRow = startRow;
    m_visibleStartRow = 0;
    // 假设传入的data大小就是3倍的可视行数
    if (data.size() >= 3) {
        m_visibleRows = data.size() / 3;
    } else {
        m_visibleRows = data.size();
    }
    endResetModel();
    
    qDebug() << "完整数据设置完成: 完整数据行数=" << m_fullData.size() 
             << ", 可视行数=" << m_visibleRows << ", 起始行=" << m_fullDataStartRow;
}

void TableModel::adjustVisibleWindow(qint64 relativeStartRow)
{
    if (relativeStartRow >= 0 && 
        relativeStartRow + m_visibleRows <= m_fullData.size()) {
        m_visibleStartRow = relativeStartRow;
        // 只需要通知视图更新，不需要重置整个模型
        emit dataChanged(
            index(0, 0), 
            index(m_visibleRows - 1, columnCount() - 1)
        );
        qDebug() << "调整可视窗口: 新的可视起始行=" << m_visibleStartRow;
    } else {
        qDebug() << "调整可视窗口失败: relativeStartRow=" << relativeStartRow
                 << ", 可视行数=" << m_visibleRows << ", 完整数据大小=" << m_fullData.size();
    }
}

qint64 TableModel::getFullDataStartRow() const
{
    return m_fullDataStartRow;
}

int TableModel::getFullDataSize() const
{
    return m_fullData.size();
}

void TableModel::clearDataOnly()
{
    beginResetModel();
    m_fullData.clear();
    m_fullDataStartRow = 0;
    m_visibleStartRow = 0;
    m_visibleRows = 0;
    endResetModel();
}