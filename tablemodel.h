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

private:
    QVector<QString> m_headers;  // 表头数据
    QVector<QStringList> m_data; // 表格数据
    qint64 m_currentWindowStartRow; // 当前数据窗口的起始行
};

#endif // TABLEMODEL_H