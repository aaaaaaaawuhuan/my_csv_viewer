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
    
    // 只返回选中的列数
    return m_selectedColumnIndexes.isEmpty() ? m_headers.size() : m_selectedColumnIndexes.size();
}

QVariant TableModel::data(const QModelIndex &index, int role) const
{
    static int old = 0U;

    if (!index.isValid() || index.row() >= m_visibleRows || index.column() >= columnCount())
        return QVariant();
    
    // 计算在完整数据中的实际行号
    int actualRow = m_visibleStartRow + index.row();
    if (actualRow >= m_fullData.size())
        return QVariant();
    
    // 确定实际的列索引
    int actualColumn = index.column();
    if (!m_selectedColumnIndexes.isEmpty() && actualColumn < m_selectedColumnIndexes.size()) {
        actualColumn = m_selectedColumnIndexes[actualColumn];
    }
    
    // 检查该行是否有足够的列数据
    const QStringList& rowData = m_fullData.at(actualRow);
    if (actualColumn >= rowData.size()) {
        return QVariant(); // 如果该行没有足够的列数据，则返回空
    }
    
    if (role == Qt::DisplayRole) {
        return rowData.at(actualColumn);
    }
    
    return QVariant();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            // 确定实际的列索引
            int actualSection = section;
            if (!m_selectedColumnIndexes.isEmpty() && section < m_selectedColumnIndexes.size()) {
                actualSection = m_selectedColumnIndexes[section];
            }
            
            if (actualSection < m_headers.size()) {
                return m_headers.at(actualSection);
            }
        } else if (orientation == Qt::Vertical) {
            // 显示实际的行号
            //qDebug()<<m_fullDataStartRow << m_visibleStartRow << section;
            return QString::number(m_fullDataStartRow + m_visibleStartRow + section);
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
    m_selectedColumnIndexes.clear();
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
    m_selectedColumnIndexes.clear();
    endResetModel();
}

// 待修改
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

// 得到请求的可视窗口数据后初始化模型数据
void TableModel::setModelData(const QVector<QStringList> &data, qint64 startRow)
{
    qDebug() << "设置完整数据: 数据行数=" << data.size() << ", 起始行=" << startRow;
    
    beginResetModel();
    m_fullData = data;
    m_fullDataStartRow = startRow;
    m_visibleStartRow = 0;
    m_visibleRows = data.size();
    endResetModel();
    
    qDebug() << "完整数据设置完成: 完整数据行数=" << m_fullData.size() 
             << ", 可视行数=" << m_visibleRows << ", 起始行=" << m_fullDataStartRow;
}

void TableModel::adjustVisibleWindow(qint64 relativeStartRow)
{
    if(m_visibleStartRow + relativeStartRow < 0)
        m_visibleStartRow = 0;
    else
        m_visibleStartRow += relativeStartRow;

    emit dataChanged(
        index(0, 0),
        index(m_visibleRows - 1, columnCount() - 1)
        );
    // 告诉视图：垂直表头的数据变了，需要重新获取 headerData
    emit headerDataChanged(Qt::Vertical, 0, m_visibleRows - 1);

    //qDebug() << "调整可视窗口:"<<"变更行数"<< relativeStartRow <<"新的可视起始行=" << m_visibleStartRow+m_fullDataStartRow<<"模型起="<<m_fullDataStartRow <<"模型终="<<m_fullDataStartRow + getFullDataSize();
    //qDebug() << "m_visibleRows-1 = " <<m_visibleRows <<"columnCount()"<<columnCount();
}

qint64 TableModel::getFullDataStartRow() const
{
    return m_fullDataStartRow;
}

qint64 TableModel::getVisiableStartRow() const
{
    return m_visibleStartRow;
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

// 预加载数据整合方法的实现
void TableModel::prependPreloadedData(const QVector<QStringList> &data)
{
    if (data.isEmpty()) return;
    
    qDebug() << "向前预加载数据: 数据行数=" << data.size();
    
    // 将新数据添加到前面
    for (int i = data.size() - 1; i >= 0; --i) {
        m_fullData.prepend(data[i]);
    }
    
    // 更新起始行号
    m_fullDataStartRow -= data.size();
    m_visibleStartRow += data.size();
    
    // 维持三倍窗口大小
    maintainTripleWindowSize();
    
    qDebug() << "向前预加载完成: 完整数据行数=" << m_fullData.size() 
             << ", 起始行=" << m_fullDataStartRow;
}

void TableModel::appendPreloadedData(const QVector<QStringList> &data)
{
    if (data.isEmpty()) return;
    
    qDebug() << "向后预加载数据: 数据行数=" << data.size();
    
    // 将新数据添加到后面
    for (const QStringList& row : data) {
        m_fullData.append(row);
    }
    
    // 维持三倍窗口大小
    maintainTripleWindowSize();
    
    qDebug() << "向后预加载完成: 完整数据行数=" << m_fullData.size() 
             << ", 起始行=" << m_fullDataStartRow;
}

bool TableModel::canPrependData(qint64 requestedStartRow) const
{
    // 检查是否可以向前预加载（即起始行号是否可以更小）
    return requestedStartRow < m_fullDataStartRow;
}

bool TableModel::canAppendData(qint64 requestedEndRow) const
{
    // 检查是否可以向后预加载（即结束行号是否可以更大）
    return (m_fullDataStartRow + m_fullData.size() - 1) < requestedEndRow;
}

void TableModel::maintainTripleWindowSize()
{
    // 确保数据窗口大小维持在三倍于可视行数
    if (m_visibleRows <= 0) return;
    
    int targetSize = m_visibleRows * 3;
    if (m_fullData.size() > targetSize) {
        // 如果数据过多，需要裁剪
        int excess = m_fullData.size() - targetSize;
        // 优先保持当前可视窗口的数据，裁剪两端的数据
        // 这里简化处理，直接从前面裁剪
        for (int i = 0; i < excess; ++i) {
            m_fullData.removeFirst();
            m_fullDataStartRow++;
        }
    }
}

void TableModel::setVisibleRows(int visibleRows)
{
    m_visibleRows = visibleRows;
}

void TableModel::setSelectedColumns(const QVector<QString>& selectedColumns)
{
    beginResetModel();
    m_selectedColumnIndexes.clear();
    
    // 根据选中的列名查找对应的索引
    for (const QString& columnName : selectedColumns) {
        for (int i = 0; i < m_headers.size(); ++i) {
            if (m_headers[i] == columnName) {
                m_selectedColumnIndexes.append(i);
                break;
            }
        }
    }
    
    // 对索引进行排序以保持列的原始顺序
    std::sort(m_selectedColumnIndexes.begin(), m_selectedColumnIndexes.end());
    endResetModel();
}

const QVector<int>& TableModel::getSelectedColumnIndexes() const
{
    return m_selectedColumnIndexes;
}