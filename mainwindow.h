#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QCheckBox>
#include <QString>
#include <QThread>
#include <QVector>
#include <QMap>
#include <QTimer>
#include <QTreeWidget>
#include <QMenu>
#include <QContextMenuEvent>
#include <QListWidget>
#include <QTabWidget>
#include "statusmanager.h"

// 调试宏定义，可通过注释掉这行来关闭所有调试信息
#define DEBUG_PRINT true

#define PRINT_DEBUG(msg) if (DEBUG_PRINT) qDebug() << msg;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class CsvReader;
class TableModel;
struct CsvRowData; // 前置声明
QT_END_NAMESPACE

// 添加滚动类型枚举
enum ScrollType {
    SMALL_SCROLL,   // 小范围滚动
    LARGE_SCROLL    // 大范围滚动
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void initCsvReader(const QString &fileName);
    void requestRowsData(qint64 startRow, qint64 rowCount); // 添加请求数据行的信号
    void requestPreloadData(qint64 startRow, qint64 rowCount); // 添加请求预加载数据的信号

private slots:
    void on_action_open_triggered();
    void on_action_show_select_triggered();
    void on_action_goto_row_triggered(); // 添加跳转到行的槽函数
    void on_pushButton_all_clicked();
    void on_pushButton_clear_clicked();
    void on_pushButton_filter_clicked();
    void on_lineEdit_clowmn_name_textChanged(const QString &text);
    void onInitializationDataReceived(const QVector<QString> &headers);
    void onRowsDataReceived(const struct CsvRowData &rowData, qint64 startRow); // 修改参数类型以匹配信号
    void onVerticalScrollBarValueChanged(int value); // 添加滚动条值变化槽函数
    void onDelayedLoad(); // 添加延迟加载槽函数
    void onPreloadTimeout(); // 添加预加载超时槽函数
    void resetScrollBarColor(); // 添加重置滚动条颜色槽函数
    void onBookmarkItemDoubleClicked(QListWidgetItem *item);
    void onAddBookmarkTriggered();
    void onRemoveBookmarkTriggered();

protected:
    void resizeEvent(QResizeEvent *event) override; // 添加窗口大小改变事件处理
    void wheelEvent(QWheelEvent *event) override;   // 添加鼠标滚轮事件处理
    void keyPressEvent(QKeyEvent *event) override;  // 添加键盘事件处理
    void contextMenuEvent(QContextMenuEvent *event) override;
    void showContextMenu(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    QString m_fileName;
    CsvReader *m_csvReader;
    TableModel *m_tableModel;  // 添加数据模型
    QVector<QString> m_headers; // 保存表头信息
    QThread *m_workerThread;
    QTimer *m_delayedLoadTimer; // 延迟加载定时器
    QTimer *m_scrollBarResetTimer; // 滚动条颜色重置定时器
    QTimer *m_preloadTimer; // 预加载定时器
    qint64 m_totalRows; // 文件总行数
    qint64 m_visibleRows; // 可视行数
    qint64 m_currentStartRow; // 当前显示的数据起始行
    qint64 m_lastScrollPosition; // 上次滚动位置
    bool m_internalScrollBarChange; // 防止滚动条信号循环调用
    int m_defaultRowHeight; // 默认行高
    QMap<QString, qint64> m_bookmarks; // 书签映射，键为书签名称，值为行号
    QMenu *m_contextMenu; // 右键菜单
    StatusManager *m_statusManager; // 状态栏管理器
    
    // 添加新函数
    ScrollType detectScrollType(qint64 oldPosition, qint64 newPosition); // 滚动类型识别
    void handleLargeScroll(qint64 targetPosition); // 大范围滚动处理
    void handleSmallScroll(qint64 targetPosition); // 小范围滚动处理
    void preloadData(qint64 centerRow); // 预加载数据
    void generateColumnCheckboxes(const QVector<QString> &headers);
    void toggleSelectAll(bool select);
    void filterCheckboxes(const QString &text);
    void updateScrollBarRange(); // 更新滚动条范围
    void updateVisibleRows(); // 更新可视行数
    int getUniformRowHeight() const; // 获取统一行高
    void PreloadedDataReceived(const struct CsvRowData &rowData, qint64 startRow); // 添加预加载数据函数
    void gotoRow(qint64 row); // 添加跳转到指定行的函数
    void setupBookmarkUI(); // 设置书签UI
    void updateBookmarkList(); // 更新书签列表显示
};

#endif // MAINWINDOW_H