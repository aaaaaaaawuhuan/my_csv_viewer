#include "tablemodel.h"

TableModel::TableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int TableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    
    return m_data.size();
}

int TableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    
    return m_headers.size();
}

QVariant TableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size() || index.column() >= m_headers.size())
        return QVariant();
    
    if (role == Qt::DisplayRole) {
        return m_data.at(index.row()).at(index.column());
    }
    
    return QVariant();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal && section < m_headers.size()) {
            return m_headers.at(section);
        }
    }
    
    return QAbstractTableModel::headerData(section, orientation, role);
}

void TableModel::setHeaders(const QVector<QString> &headers)
{
    beginResetModel();
    m_headers = headers;
    m_data.clear();
    endResetModel();
}

void TableModel::addRow(const QStringList &row)
{
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.append(row);
    endInsertRows();
}

void TableModel::clear()
{
    beginResetModel();
    m_headers.clear();
    m_data.clear();
    endResetModel();
}