#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "csvreader.h"
#include "tablemodel.h"  // 添加包含
#include <QVBoxLayout>
#include <QScrollArea>
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QDateTime>
#include <QScrollBar>
#include <QTimer>
#include <QResizeEvent>  // 添加QResizeEvent头文件
#include <QHeaderView>   // 添加QHeaderView头文件
#include <QWheelEvent>   // 添加鼠标滚轮事件头文件
#include <QKeyEvent>     // 添加键盘事件头文件
#include <QInputDialog>  // 添加输入对话框头文件
#include <QMessageBox>   // 添加消息框头文件

// 定义DEBUG_PRINT宏，如果未定义则设为qDebug()输出调试信息
#ifndef DEBUG_PRINT
#define DEBUG_PRINT(msg) qDebug() << "[DEBUG]" << msg
#endif


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_fileName("")
    , m_csvReader(new CsvReader)
    , m_tableModel(new TableModel(this))  // 初始化数据模型
    , m_workerThread(new QThread)
    , m_delayedLoadTimer(new QTimer(this))
    , m_scrollBarResetTimer(new QTimer(this))
    , m_preloadTimer(new QTimer(this))
    , m_totalRows(0)
    , m_visibleRows(100) // 默认显示100行
    , m_currentStartRow(0) // 初始化当前起始行
    , m_lastScrollPosition(0) // 初始化上次滚动位置
    , m_internalScrollBarChange(false) // 初始化滚动条循环调用标志
    , m_defaultRowHeight(25) // 默认行高25像素
{
    ui->setupUi(this);
    
    // 将数据模型设置到tableView中
    ui->tableView->setModel(m_tableModel);
    
    // 设置固定行高并启用文本换行
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView->verticalHeader()->setDefaultSectionSize(m_defaultRowHeight);
    ui->tableView->setWordWrap(true);
    
    // 禁用QTableView自带的垂直滚动条
    ui->tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 将CsvReader移动到工作线程
    m_csvReader->moveToThread(m_workerThread);
    m_workerThread->start();

    // 连接信号和槽
    connect(this, &MainWindow::initCsvReader, m_csvReader, &CsvReader::init);
    connect(this, &MainWindow::requestRowsData, m_csvReader, &CsvReader::readRows); // 连接请求数据行的信号和槽
    connect(this, &MainWindow::requestPreloadData, m_csvReader, &CsvReader::readRows); // 连接预加载数据请求信号和槽
    
    // 创建一个定时器用于重置滚动条颜色
    m_scrollBarResetTimer = new QTimer(this);
    m_scrollBarResetTimer->setSingleShot(true);
    connect(m_scrollBarResetTimer, &QTimer::timeout, this, &MainWindow::resetScrollBarColor);
    connect(m_csvReader, &CsvReader::initializationDataReady, 
            this, &MainWindow::onInitializationDataReceived);
    connect(m_csvReader, &CsvReader::rowDataReady, 
            this, &MainWindow::onRowsDataReceived); // 连接数据行读取完成的信号和槽

    // 连接滚动条信号和槽
    connect(ui->verticalScrollBar, &QScrollBar::valueChanged,
            this, &MainWindow::onVerticalScrollBarValueChanged);
    // 保存当前起始行为初始值
    m_currentStartRow = ui->verticalScrollBar->value();
    
    // 设置延迟加载定时器
    m_delayedLoadTimer->setSingleShot(true);
    connect(m_delayedLoadTimer, &QTimer::timeout, this, &MainWindow::onDelayedLoad);
    
    // 设置预加载定时器
    m_preloadTimer->setSingleShot(true);
    connect(m_preloadTimer, &QTimer::timeout, this, &MainWindow::onPreloadTimeout);

    //init
    ui->dockWidget->hide();
    ui->tableView->hide();
    
    // 连接输入框的文本变化信号到筛选槽函数
    QLineEdit *lineEdit = ui->dockWidgetContents->findChild<QLineEdit*>("lineEdit_clowmn_name");
    if (lineEdit) {
        connect(lineEdit, &QLineEdit::textChanged, this, &MainWindow::on_lineEdit_clowmn_name_textChanged);
    }
    
    // 初始化状态栏信息
    statusBar()->showMessage(tr("就绪"));
    
    // 初始化书签UI
    setupBookmarkUI();
    
    // 初始化右键菜单
    m_contextMenu = new QMenu(this);
    QAction *addBookmarkAction = new QAction(tr("添加当前行为书签"), this);
    connect(addBookmarkAction, &QAction::triggered, this, &MainWindow::onAddBookmarkTriggered);
    m_contextMenu->addAction(addBookmarkAction);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &MainWindow::showContextMenu);
}

MainWindow::~MainWindow()
{
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_csvReader;
    delete m_workerThread;
    delete ui;
}

void MainWindow::startTiming(const QString &operation)
{
    m_timer.start();
    // 在状态栏显示正在执行的操作
    statusBar()->showMessage(tr("正在执行%1...").arg(operation));
}

void MainWindow::endTiming(const QString &operation)
{
    if (m_timer.isValid()) {
        qint64 elapsed = m_timer.elapsed();
        m_performanceData[operation] = elapsed;
        // 更新状态栏显示耗时信息
        updateStatusBarWithTimingInfo();
    }
}

void MainWindow::updateStatusBarWithTimingInfo()
{
    QString message = tr("就绪");
    if (!m_performanceData.isEmpty()) {
        // 显示最近几次操作的耗时信息
        QString timingInfo;
        for (auto it = m_performanceData.begin(); it != m_performanceData.end(); ++it) {
            if (!timingInfo.isEmpty()) timingInfo += ", ";
            timingInfo += QString("%1: %2ms").arg(it.key()).arg(it.value());
        }
        message = timingInfo;
    }
    statusBar()->showMessage(message);
}

void MainWindow::on_action_open_triggered()
{
    startTiming(tr("打开文件对话框"));
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open CSV File"),"",tr("CSV Files (*.csv)"));
    endTiming(tr("打开文件对话框"));

    if(!fileName.isEmpty())
    {
        m_fileName = fileName;
        // 发送文件名给CsvReader进行初始化
        startTiming(tr("文件初始化"));
        emit initCsvReader(fileName);
        // 注意：这里不会立即结束计时，因为是异步操作
        // 实际的计时结束会在onInitializationDataReceived中处理
    }
    else
    {
        QMessageBox::warning(this,"waring","please select csv");
    }
}

void MainWindow::on_action_show_select_triggered()
{
    // 切换筛选面板的显示状态
    if (ui->dockWidget->isVisible()) {
        ui->dockWidget->hide();
    } else {
        ui->dockWidget->show();
    }
}

void MainWindow::on_pushButton_all_clicked()
{
    // 全选所有复选框
    toggleSelectAll(true);
}

void MainWindow::on_pushButton_clear_clicked()
{
    // 清空所有复选框选择
    toggleSelectAll(false);
}

void MainWindow::on_pushButton_filter_clicked()
{
    startTiming(tr("筛选显示"));
    
    // 获取选中的列
    QScrollArea *scrollArea = ui->dockWidgetContents->findChild<QScrollArea*>("scrollArea_select");
    if (!scrollArea) return;
    
    QWidget *contentWidget = scrollArea->widget();
    if (!contentWidget) return;
    
    // 获取QTreeWidget控件
    QTreeWidget *treeWidget = contentWidget->findChild<QTreeWidget*>("columnTreeWidget");
    if (!treeWidget) return;
    
    // 获取所有选中的树项
    QVector<QString> selectedColumns;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            selectedColumns.append(item->text(0));
        }
    }
    
    // 检查是否有选中的列
    if (selectedColumns.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请至少选择一列进行显示"));
        endTiming(tr("筛选显示"));
        return;
    }

    // 显示表格视图
    ui->tableView->show();
    
    // 保存当前筛选的列索引，用于后续比较
    QVector<int> oldSelectedColumns = m_tableModel->getSelectedColumnIndexes();
    
    // 设置新的选中的列
    m_tableModel->setSelectedColumns(selectedColumns);
    
    // 获取新的筛选列索引
    QVector<int> newSelectedColumns = m_tableModel->getSelectedColumnIndexes();
    
    // 找出新增的筛选列（新的列索引在之前未被选中）
    QVector<int> newHighlightedColumns;
    for (int columnIndex : newSelectedColumns) {
        if (!oldSelectedColumns.contains(columnIndex)) {
            newHighlightedColumns.append(columnIndex);
        }
    }
    
    // 如果有新增的筛选列，设置表头高亮为红色
    if (!newHighlightedColumns.isEmpty()) {
        PRINT_DEBUG(QString("发现 %1 个新增筛选列，设置为红色高亮").arg(newHighlightedColumns.size()));
        m_tableModel->setNewHighlightedColumns(newHighlightedColumns);
    }
    
    endTiming(tr("筛选显示"));
}

void MainWindow::on_lineEdit_clowmn_name_textChanged(const QString &text)
{
    filterCheckboxes(text);
}

void MainWindow::filterCheckboxes(const QString &text)
{
    startTiming(tr("筛选列"));
    
    // 获取TabWidget
    QTabWidget *tabWidget = ui->dockWidgetContents->findChild<QTabWidget*>("filterBookmarkTabWidget");
    if (!tabWidget) {
        PRINT_DEBUG("无法获取filterBookmarkTabWidget");
        return;
    }
    
    // 获取筛选Tab页
    QWidget *filterTab = tabWidget->widget(0); // 假设筛选是第一个tab
    if (!filterTab) {
        PRINT_DEBUG("无法获取筛选Tab页");
        return;
    }
    
    // 查找ScrollArea
    QScrollArea *scrollArea = filterTab->findChild<QScrollArea*>("scrollArea_select");
    if (!scrollArea) {
        PRINT_DEBUG("无法获取ScrollArea");
        return;
    }
    
    // 获取滚动区域的内容部件
    QWidget *contentWidget = scrollArea->widget();
    if (!contentWidget) {
        PRINT_DEBUG("无法获取contentWidget");
        return;
    }
    
    // 获取QTreeWidget控件
    QTreeWidget *treeWidget = contentWidget->findChild<QTreeWidget*>("columnTreeWidget");
    if (!treeWidget) {
        PRINT_DEBUG("无法获取columnTreeWidget");
        return;
    }
    
    // 根据输入文本过滤树项
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);
        // 根据输入文本决定是否显示树项
        if (text.isEmpty() || text == "输入列名") {
            // 如果输入框为空或为默认文本，显示所有树项
            item->setHidden(false);
        } else {
            // 根据文本过滤树项
            if (item->text(0).contains(text, Qt::CaseInsensitive)) {
                item->setHidden(false);
            } else {
                item->setHidden(true);
            }
        }
    }
    
    endTiming(tr("筛选列"));
}


void MainWindow::toggleSelectAll(bool select)
{
    startTiming(select ? tr("全选") : tr("清空"));
    
    // 获取TabWidget
    QTabWidget *tabWidget = ui->dockWidgetContents->findChild<QTabWidget*>("filterBookmarkTabWidget");
    if (!tabWidget) {
        PRINT_DEBUG("无法获取filterBookmarkTabWidget");
        return;
    }
    
    // 获取筛选Tab页
    QWidget *filterTab = tabWidget->widget(0); // 假设筛选是第一个tab
    if (!filterTab) {
        PRINT_DEBUG("无法获取筛选Tab页");
        return;
    }
    
    // 查找ScrollArea
    QScrollArea *scrollArea = filterTab->findChild<QScrollArea*>("scrollArea_select");
    if (!scrollArea) {
        PRINT_DEBUG("无法获取ScrollArea");
        return;
    }
    
    // 获取滚动区域的内容部件
    QWidget *contentWidget = scrollArea->widget();
    if (!contentWidget) {
        PRINT_DEBUG("无法获取contentWidget");
        return;
    }
    
    // 获取QTreeWidget控件
    QTreeWidget *treeWidget = contentWidget->findChild<QTreeWidget*>("columnTreeWidget");
    if (!treeWidget) {
        PRINT_DEBUG("无法获取columnTreeWidget");
        return;
    }
    
    // 获取所有可见的树项并设置它们的选中状态
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);
        if (item->isHidden()) continue;
        
        item->setCheckState(0, select ? Qt::Checked : Qt::Unchecked);
    }
    
    endTiming(select ? tr("全选") : tr("清空"));
}

void MainWindow::onInitializationDataReceived(const QVector<QString> &headers)
{
    // 结束文件初始化计时
    endTiming(tr("文件初始化"));
    
    qDebug() << "初始化数据接收: 表头数量=" << headers.size() << ", 总行数=" << m_csvReader->getTotalRows();
    
    // 显示CsvReader的性能数据
    const QMap<QString, qint64> &csvReaderPerformanceData = m_csvReader->getPerformanceData();
    if (!csvReaderPerformanceData.isEmpty()) {
        // 将CsvReader的性能数据合并到主窗口的性能数据中
        for (auto it = csvReaderPerformanceData.begin(); it != csvReaderPerformanceData.end(); ++it) {
            m_performanceData["CsvReader-" + it.key()] = it.value();
        }
        updateStatusBarWithTimingInfo();
    }
    
    startTiming(tr("生成筛选面板"));
    
    // 显示dockWidget
    ui->dockWidget->show();

    // 清空之前的数据
    m_tableModel->clear();
    
    // 根据表头生成复选框
    generateColumnCheckboxes(headers);
    
    // 保存表头信息
    m_headers = headers;
    
    // 设置表格模型的表头
    m_tableModel->setHeaders(headers);
    
    // 保存总行数（从CsvReader获取）
    m_totalRows = m_csvReader->getTotalRows();
    
    qDebug() << "总行数设置为:" << m_totalRows << ", 可视行数:" << m_visibleRows;
    
    // 更新滚动条范围
    updateScrollBarRange();

    emit requestRowsData(1, m_visibleRows); // 从第1行开始读取可视行数（跳过表头）
    
    // 初始化TableView的滚动条
    QScrollBar* tableViewScrollBar = ui->tableView->verticalScrollBar();
    if (tableViewScrollBar) {
        tableViewScrollBar->setRange(0, m_totalRows - m_visibleRows);
        tableViewScrollBar->setPageStep(m_visibleRows);
        qDebug() << "初始化TableView滚动条: 范围0-" << (m_totalRows - m_visibleRows) << ", 步长=" << m_visibleRows;
    }
    
    endTiming(tr("生成筛选面板"));
}

void MainWindow::onRowsDataReceived(const struct CsvRowData &rowData, qint64 startRow)
{
    if(startRow != m_currentStartRow+1)
    {
        PreloadedDataReceived(rowData,startRow);
        return;
    }

    // 1. 清除当前显示的数据，但保留表头
    m_tableModel->clearDataOnly(); // 只清空数据部分
    
    startTiming(tr("处理数据行"));
    
    qDebug() << "接收到数据行: startRow=" << startRow << ", 数据行数=" << rowData.rows.size();
    
    // 填充可视窗口的数据
    m_tableModel->setModelData(rowData.rows, startRow);

#ifdef DEBUG_PRINT
    // 打印模型中的数据信息
    qDebug() << "模型更新后行数=" << m_tableModel->rowCount();
    if (m_tableModel->rowCount() > 0) {
        QModelIndex firstIndex = m_tableModel->index(0, 0);
        QVariant firstData = m_tableModel->data(firstIndex, Qt::DisplayRole);
        qDebug() << "第一行第一列数据=" << firstData.toString();
    }
#endif
    // 强制刷新视图
    ui->tableView->viewport()->update();
    
    // 直接滚动到顶部以确保显示正确位置的数据
    if (rowData.rows.size() > 0) {
        QModelIndex firstIndex = m_tableModel->index(0, 0);
        ui->tableView->scrollTo(firstIndex, QAbstractItemView::PositionAtTop);
        qDebug() << "已滚动到第一行数据";
    }
    
    // 更新状态栏显示当前数据位置
    QString positionInfo = QString("显示行 %1-%2 (共%3行)")
        .arg(startRow + 1)
        .arg(startRow + rowData.rows.size())
        .arg(m_totalRows);
    statusBar()->showMessage(positionInfo);
    
    // 显示CsvReader的性能数据
    if (!rowData.performanceData.isEmpty()) {
        // 将CsvReader的性能数据合并到主窗口的性能数据中
        for (auto it = rowData.performanceData.begin(); it != rowData.performanceData.end(); ++it) {
            m_performanceData["CsvReader-" + it.key()] = it.value();
        }
        updateStatusBarWithTimingInfo();
    }
    
    endTiming(tr("处理数据行"));
}

void MainWindow::generateColumnCheckboxes(const QVector<QString> &headers)
{
    startTiming(tr("创建复选框"));
    
    // 查找scrollArea_select控件
    QScrollArea *scrollArea = ui->dockWidgetContents->findChild<QScrollArea*>("scrollArea_select");
    if (!scrollArea) return;
    
    // 获取滚动区域的内容部件
    QWidget *contentWidget = scrollArea->widget();
    if (!contentWidget) return;
    
    // 获取内容部件的布局
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(contentWidget->layout());
    if (!layout) return;
    
    // 删除布局中除spacer外的所有控件
    for (int i = layout->count() - 1; i >= 0; --i) {
        QLayoutItem* item = layout->itemAt(i);
        if (item) {
            QWidget* widget = item->widget();
            // 如果是复选框或其他旧控件，删除它
            if (qobject_cast<QCheckBox*>(widget) || qobject_cast<QTreeWidget*>(widget)) {
                layout->takeAt(i);
                delete widget;
                delete item;
            }
        }
    }
    
    // 创建QTreeWidget来显示列选项
    QTreeWidget *treeWidget = new QTreeWidget(contentWidget);
    treeWidget->setObjectName("columnTreeWidget"); // 设置对象名称
    treeWidget->setHeaderLabel(tr("选择要显示的列"));
    treeWidget->setSelectionMode(QAbstractItemView::NoSelection);
    treeWidget->setFocusPolicy(Qt::NoFocus);
    
    // 为每个表头创建一个树项
    for (const QString &header : headers) {
        QTreeWidgetItem *item = new QTreeWidgetItem(treeWidget);
        item->setText(0, header);
        item->setCheckState(0, Qt::Unchecked); // 默认选中
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    }
    
    // 设置树形控件的一些属性，确保正确填充空间
    treeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    treeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 插入到布局中倒数第二个位置（在spacer之前）
    layout->insertWidget(layout->count() - 1, treeWidget);
    
    // 强制更新布局
    treeWidget->updateGeometry();
    contentWidget->updateGeometry();
    scrollArea->update();
    
    endTiming(tr("创建复选框"));
}

void MainWindow::onVerticalScrollBarValueChanged(int value)
{
    qDebug() << "滚动条值变化: value=" << value << ", 当前起始行=" << m_currentStartRow;
    int currentValue = ui->verticalScrollBar->value();
    // 检查是否需要加载新数据
    ScrollType scrollType = detectScrollType(m_lastScrollPosition, currentValue);

    if (scrollType == LARGE_SCROLL)
    {
        // 当滚动条值变化时，启动延迟加载定时器
        m_delayedLoadTimer->start(200); // 200ms延迟
    }
    else
    {
        handleSmallScroll(currentValue);
    }

    m_lastScrollPosition = currentValue;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // 更新可视行数
    updateVisibleRows();
    
    // 更新滚动条范围
    updateScrollBarRange();
    
    // 重新加载当前可视区域数据以适应新大小
    handleLargeScroll(ui->verticalScrollBar->value());
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    // 计算滚动行数
    int delta = event->angleDelta().y();
    int scrollRows = -delta / 120; // 通常每120个单位滚动1行
    
    int oldValue = ui->verticalScrollBar->value();
    // 计算新的滚动条值并立即应用限制
    int newScrollBarValue = qBound(0, ui->verticalScrollBar->value() + scrollRows, 
                                   ui->verticalScrollBar->maximum());
    
    qDebug() << "鼠标滚轮事件: delta=" << delta << ", scrollRows=" << scrollRows 
             << ", oldValue=" << oldValue << ", newValue=" << newScrollBarValue;
    
    // 更新滚动条值（会触发onVerticalScrollBarValueChanged）
    m_internalScrollBarChange = true;
    ui->verticalScrollBar->setValue(newScrollBarValue);
    
    // 接受事件
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int currentScrollBarValue = ui->verticalScrollBar->value();
    int newScrollBarValue = currentScrollBarValue;
    
    qDebug() << "键盘事件: key=" << event->key() << ", currentValue=" << currentScrollBarValue;

    switch (event->key()) {
    case Qt::Key_Up:
        newScrollBarValue = qMax(0, currentScrollBarValue - 1);
        qDebug() << "处理向上键: newValue=" << newScrollBarValue;
        break;
    case Qt::Key_Down:
        newScrollBarValue = qMin(ui->verticalScrollBar->maximum(), 
                                currentScrollBarValue + 1);
        qDebug() << "处理向下键: newValue=" << newScrollBarValue;
        break;
    case Qt::Key_PageUp:
        newScrollBarValue = qMax(0, currentScrollBarValue - m_visibleRows);
        qDebug() << "处理PageUp键: newValue=" << newScrollBarValue << ", m_visibleRows=" << m_visibleRows;
        break;
    case Qt::Key_PageDown:
        newScrollBarValue = qMin(ui->verticalScrollBar->maximum(), 
                                currentScrollBarValue + m_visibleRows);
        qDebug() << "处理PageDown键: newValue=" << newScrollBarValue << ", m_visibleRows=" << m_visibleRows;
        break;
    case Qt::Key_Home:
        newScrollBarValue = 0;
        qDebug() << "处理Home键: newValue=" << newScrollBarValue;
        break;
    case Qt::Key_End:
        newScrollBarValue = ui->verticalScrollBar->maximum();
        qDebug() << "处理End键: newValue=" << newScrollBarValue;
        break;
    default:
        // 其他按键不处理，调用父类处理
        qDebug() << "未处理的按键: key=" << event->key();
        QMainWindow::keyPressEvent(event);
        return;
    }
    
    // 更新滚动条值
    if (newScrollBarValue != currentScrollBarValue) {
        qDebug() << "更新滚动条值: " << currentScrollBarValue << " -> " << newScrollBarValue;
        m_internalScrollBarChange = true;
        ui->verticalScrollBar->setValue(newScrollBarValue);
    }
    
    // 接受事件
    event->accept();
}

void MainWindow::updateVisibleRows()
{
    // 获取TableView的可视区域高度
    int viewportHeight = ui->tableView->viewport()->height();
    int tableViewHeight = ui->tableView->height();
    
    // 使用统一的行高
    int uniformRowHeight = getUniformRowHeight();
    
    // 计算可视行数
    int visibleRows = viewportHeight / uniformRowHeight;
    
    // 确保至少显示1行，最多不超过一个合理的上限
    m_visibleRows = qBound(1, visibleRows, 200); // 最多显示200行
    
    // 同步到TableModel
    m_tableModel->setVisibleRows(m_visibleRows);
    
    qDebug() << "更新可视行数: viewportHeight=" << viewportHeight 
             << ", tableViewHeight=" << tableViewHeight
             << ", uniformRowHeight=" << uniformRowHeight 
             << ", visibleRows=" << visibleRows
             << ", m_visibleRows=" << m_visibleRows;
}

int MainWindow::getUniformRowHeight() const
{
    // 使用固定的默认行高
    return m_defaultRowHeight;
}

void MainWindow::onDelayedLoad()
{
    // 恢复滚动条正常样式
    ui->verticalScrollBar->setPalette(QPalette());
    
    // 延迟加载数据
    int currentValue = ui->verticalScrollBar->value();
    
    qDebug() << "延迟加载触发: currentValue=" << currentValue << ", m_currentStartRow=" << m_currentStartRow;
    handleLargeScroll(currentValue);
}

void MainWindow::onPreloadTimeout()
{
    int currentValue = ui->verticalScrollBar->value();
    qDebug() << "预加载触发: currentValue=" << currentValue;
    
    // 执行预加载
    preloadData(currentValue + 1);
}

// 预加载数据函数
void MainWindow::preloadData(qint64 centerRow)
{
    // 计算预加载范围：前后各m_visibleRows行
    // 防止读取表头
    qint64 preStartRow = qMax(1LL, centerRow - m_visibleRows);
    qint64 postEndRow = qMin(m_totalRows - 1, centerRow + 2 * m_visibleRows - 1);
    
    qDebug() << "预加载数据: 中心行=" << centerRow 
             << ", 前置范围=[" << preStartRow << "," << (centerRow-1) << "]"
             << ", 后置范围=[" << (centerRow + m_visibleRows) << "," << postEndRow << "]";
    
    // 请求预加载前置数据
    if (preStartRow < centerRow) {
        qint64 rowCount = centerRow - preStartRow;
        emit requestPreloadData(preStartRow, rowCount);
        qDebug() << "请求前置预加载数据: 起始行=" << (preStartRow) << ", 行数=" << rowCount;
    }
    
    // 请求预加载后置数据
    qint64 postStartRow = centerRow + m_visibleRows;
    if (postStartRow <= postEndRow) {
        qint64 rowCount = postEndRow - postStartRow;
        emit requestPreloadData(postStartRow, rowCount);
        qDebug() << "请求后置预加载数据: 起始行=" << (postStartRow) << ", 行数=" << rowCount;
    }
}

// 滚动类型识别
ScrollType MainWindow::detectScrollType(qint64 oldPosition, qint64 newPosition)
{
    // 根据滚动距离判断是大范围还是小范围滚动
    qint64 distance = qAbs(newPosition - oldPosition);
    
    // 如果滚动距离超过可视区域的一半，则认为是大范围滚动
    // 否则为小范围滚动
    if (distance > m_visibleRows / 2) {
        return LARGE_SCROLL;
    } else {
        return SMALL_SCROLL;
    }
}

// 大范围滚动处理
void MainWindow::handleLargeScroll(qint64 targetPosition)
{
    qint64 startRow = targetPosition;
    qint64 rowCount = m_visibleRows;
    
    qDebug() << "大范围滚动处理: 起始行=" << startRow + 1 << ", 行数=" << rowCount;
    
    // 3. 请求数据加载
    emit requestRowsData(startRow + 1, rowCount); // +1是因为跳过表头
     m_preloadTimer->start(500);     // 500ms 延迟
    // 4. 更新当前起始行
    m_currentStartRow = targetPosition;
}

// 小范围滚动处理
void MainWindow::handleSmallScroll(qint64 targetPosition)
{
    // 小范围滚动时，调整TableModel中的可视窗口
    qint64 relativePosition = targetPosition - m_lastScrollPosition;
    
    qDebug() << "小范围滚动处理: targetPosition=" << targetPosition 
             << ", relativePosition=" << relativePosition
             <<", visiableStart=" << m_tableModel->getVisiableStartRow()
             <<", fulldatasize -1="<<m_tableModel->getFullDataSize()-1;

    //if()上下方向移动预加载

    if((m_tableModel->getVisiableStartRow() + relativePosition < 1) || (m_tableModel->getVisiableStartRow() + m_visibleRows + relativePosition > m_tableModel->getFullDataSize()-1))
    {
        //待处理为直接预加载
        qDebug() << "小范围滚动超出范围，降级到大范围滚动处理";
        handleLargeScroll(targetPosition);
    }
    else
    {
        m_tableModel->adjustVisibleWindow(relativePosition);
    }
}

/**
 * @brief 重置滚动条颜色
 */
void MainWindow::resetScrollBarColor()
{
    // 恢复滚动条默认颜色
    QPalette pal = ui->verticalScrollBar->palette();
    pal.setColor(QPalette::Active, QPalette::Highlight, 
                 style()->standardPalette().color(QPalette::Active, QPalette::Highlight));
    ui->verticalScrollBar->setPalette(pal);
}

//考虑将滚动条从1开始
void MainWindow::updateScrollBarRange()
{    
    // 更新滚动条范围
    m_visibleRows = qMin(m_totalRows,ui->frame->height()/getUniformRowHeight())-2;
    ui->verticalScrollBar->setRange(0, m_totalRows - m_visibleRows);
    ui->verticalScrollBar->setPageStep(m_visibleRows);
    qDebug() << "更新滚动条范围: 0-" << (m_totalRows - m_visibleRows) << ", 步长=" << m_visibleRows;
}

void MainWindow::PreloadedDataReceived(const struct CsvRowData &rowData, qint64 startRow)
{
    qDebug() << "接收到预加载数据: startRow=" << startRow << ", 数据行数=" << rowData.rows.size();
    
    // 根据预加载数据的位置决定是向前还是向后整合数据
    qint64 currentDataStartRow = m_tableModel->getFullDataStartRow();
    qint64 preloadedDataEndRow = startRow + rowData.rows.size() - 1;
    qint64 currentDataEndRow = currentDataStartRow + m_tableModel->getFullDataSize() - 1;
    
    if (preloadedDataEndRow < currentDataStartRow) {
        // 预加载的是前方数据
        if (m_tableModel->canPrependData(startRow)) {
            m_tableModel->prependPreloadedData(rowData.rows);
        }
    } else if (startRow > currentDataEndRow) {
        // 预加载的是后方数据
        if (m_tableModel->canAppendData(preloadedDataEndRow)) {
            m_tableModel->appendPreloadedData(rowData.rows);
        }
    }
    
    // 显示CsvReader的性能数据
    if (!rowData.performanceData.isEmpty()) {
        // 将CsvReader的性能数据合并到主窗口的性能数据中
        for (auto it = rowData.performanceData.begin(); it != rowData.performanceData.end(); ++it) {
            m_performanceData["预加载-" + it.key()] = it.value();
        }
        updateStatusBarWithTimingInfo();
    }
}


void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    // 调用showContextMenu方法显示上下文菜单
    showContextMenu(event->pos());
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    // 将视图坐标转换为全局坐标
    QPoint globalPos = ui->tableView->viewport()->mapToGlobal(pos);
    
    // 获取右键点击的行
    QModelIndex index = ui->tableView->indexAt(pos);
    if (index.isValid()) {
        // 显示菜单
        m_contextMenu->exec(globalPos);
    }
}

void MainWindow::on_action_goto_row_triggered()
{
    if (m_totalRows <= 0) {
        QMessageBox::information(this, tr("提示"), tr("请先打开一个CSV文件"));
        return;
    }
    
    bool ok;
    qint64 row = QInputDialog::getInt(this, tr("跳转到行"), 
                                     tr("请输入行号 (1-%1):").arg(m_totalRows),
                                     1, 1, static_cast<int>(m_totalRows), 1, &ok);
    if (ok) {
        gotoRow(row);
    }
}

void MainWindow::gotoRow(qint64 row)
{
    // 行号从1开始，转换为从0开始的索引并考虑表头
    qint64 targetRow = row - 1; // 减1得到0基索引
    
    // 检查目标行是否有效
    if (targetRow < 0 || targetRow >= m_totalRows) {
        QMessageBox::warning(this, tr("错误"), tr("行号超出范围"));
        return;
    }
    
    PRINT_DEBUG(QString("跳转到行: %1 (0基索引: %2)").arg(row).arg(targetRow));
    
    // 设置滚动条位置为目标行
    m_internalScrollBarChange = true; // 防止触发重新加载
    ui->verticalScrollBar->setValue(static_cast<int>(targetRow));
    
    // 直接处理大范围滚动以加载目标行数据
    handleLargeScroll(targetRow);
}

void MainWindow::setupBookmarkUI()
{
    // 获取筛选面板的内容部件
    QWidget *dockWidgetContents = ui->dockWidgetContents;
    if (!dockWidgetContents) {
        PRINT_DEBUG("无法获取dockWidgetContents");
        return;
    }
    
    // 清除当前所有布局和控件
    QLayout *oldLayout = dockWidgetContents->layout();
    if (oldLayout) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete oldLayout;
    }
    
    // 创建新的主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(dockWidgetContents);
    dockWidgetContents->setLayout(mainLayout);
    
    // 创建TabWidget作为侧边栏的主要容器
    QTabWidget *tabWidget = new QTabWidget(dockWidgetContents);
    tabWidget->setObjectName("filterBookmarkTabWidget");
    tabWidget->setTabPosition(QTabWidget::North); // 设置标签位置在顶部
    tabWidget->setDocumentMode(true); // 启用文档模式，使标签栏更紧凑
    
    // 创建筛选面板Tab页
    QWidget *filterTab = new QWidget();
    QVBoxLayout *filterLayout = new QVBoxLayout(filterTab);
    filterLayout->setContentsMargins(5, 5, 5, 5); // 设置内边距
    
    // 重新创建筛选面板控件，确保干净的UI
    QFrame *frame_button = new QFrame(filterTab);
    frame_button->setMaximumSize(QSize(16777215, 100));
    frame_button->setFrameShape(QFrame::Box);
    frame_button->setFrameShadow(QFrame::Raised);
    QGridLayout *gridLayout = new QGridLayout(frame_button);
    
    QPushButton *pushButton_all = new QPushButton(tr("全选"), frame_button);
    gridLayout->addWidget(pushButton_all, 0, 0);
    connect(pushButton_all, &QPushButton::clicked, this, &MainWindow::on_pushButton_all_clicked);
    
    QPushButton *pushButton_clear = new QPushButton(tr("清空"), frame_button);
    gridLayout->addWidget(pushButton_clear, 0, 1);
    connect(pushButton_clear, &QPushButton::clicked, this, &MainWindow::on_pushButton_clear_clicked);
    
    QPushButton *pushButton_filter = new QPushButton(tr("筛选"), frame_button);
    gridLayout->addWidget(pushButton_filter, 1, 0, 1, 2);
    connect(pushButton_filter, &QPushButton::clicked, this, &MainWindow::on_pushButton_filter_clicked);
    
    // 添加列名输入框
    QLineEdit *lineEdit_clowmn_name = new QLineEdit(frame_button);
    lineEdit_clowmn_name->setObjectName("lineEdit_clowmn_name");
    lineEdit_clowmn_name->setPlaceholderText(tr("输入列名"));
    gridLayout->addWidget(lineEdit_clowmn_name, 2, 0, 1, 2);
    connect(lineEdit_clowmn_name, &QLineEdit::textChanged, this, &MainWindow::on_lineEdit_clowmn_name_textChanged);

    filterLayout->addWidget(frame_button);
    
    // 添加列选择区域
    QScrollArea *scrollArea = new QScrollArea(filterTab);
    scrollArea->setObjectName("scrollArea_select"); // 设置对象名称
    scrollArea->setWidgetResizable(true);
    QWidget *scrollAreaWidgetContents = new QWidget();
    scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
    QVBoxLayout *verticalLayout_2 = new QVBoxLayout(scrollAreaWidgetContents);
    verticalLayout_2->setContentsMargins(5, 5, 5, 5);
    
    // 添加标签
    QLabel *label = new QLabel(tr("选择要显示的列"), scrollAreaWidgetContents);
    verticalLayout_2->addWidget(label);
    
    // 创建QTreeWidget来显示列选项
    QTreeWidget *treeWidget = new QTreeWidget(scrollAreaWidgetContents);
    treeWidget->setObjectName("columnTreeWidget"); // 设置对象名称
    treeWidget->setHeaderLabel(tr("选择要显示的列"));
    treeWidget->setSelectionMode(QAbstractItemView::NoSelection);
    treeWidget->setFocusPolicy(Qt::NoFocus);
    treeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    treeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalLayout_2->addWidget(treeWidget);
    
    scrollArea->setWidget(scrollAreaWidgetContents);
    filterLayout->addWidget(scrollArea);
    
    tabWidget->addTab(filterTab, tr("筛选"));
    
    // 创建书签Tab页
    QWidget *bookmarkTab = new QWidget();
    QVBoxLayout *bookmarkLayout = new QVBoxLayout(bookmarkTab);
    bookmarkLayout->setContentsMargins(5, 5, 5, 5); // 设置内边距
    
    // 添加标题
    QLabel *bookmarkLabel = new QLabel(tr("书签列表"), bookmarkTab);
    QFont font = bookmarkLabel->font();
    font.setBold(true);
    bookmarkLabel->setFont(font);
    bookmarkLayout->addWidget(bookmarkLabel);
    
    // 创建书签列表
    QListWidget *bookmarkListWidget = new QListWidget(bookmarkTab);
    bookmarkListWidget->setObjectName("bookmarkListWidget");
    bookmarkListWidget->setAlternatingRowColors(true); // 启用交替行颜色
    connect(bookmarkListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onBookmarkItemDoubleClicked);
    bookmarkLayout->addWidget(bookmarkListWidget, 1); // 占据剩余空间
    
    // 创建书签操作按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 5, 0, 0); // 设置上边距
    
    QPushButton *addButton = new QPushButton(tr("添加当前行为书签"), bookmarkTab);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddBookmarkTriggered);
    buttonLayout->addWidget(addButton);
    
    QPushButton *removeButton = new QPushButton(tr("移除选中书签"), bookmarkTab);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::onRemoveBookmarkTriggered);
    buttonLayout->addWidget(removeButton);
    
    bookmarkLayout->addLayout(buttonLayout);
    
    tabWidget->addTab(bookmarkTab, tr("书签"));
    
    // 将TabWidget添加到主布局
    mainLayout->addWidget(tabWidget);
    
    PRINT_DEBUG("书签UI设置完成：侧边栏已改造为包含筛选和书签两个Tab页的结构");
}

void MainWindow::updateBookmarkList()
{
    // 获取书签列表控件
    QTabWidget *tabWidget = ui->dockWidgetContents->findChild<QTabWidget*>("filterBookmarkTabWidget");
    if (!tabWidget) {
        PRINT_DEBUG("无法获取tabWidget");
        return;
    }
    
    QListWidget *bookmarkListWidget = tabWidget->findChild<QListWidget*>("bookmarkListWidget");
    if (!bookmarkListWidget) {
        PRINT_DEBUG("无法获取bookmarkListWidget");
        return;
    }
    
    // 清空书签列表
    bookmarkListWidget->clear();
    
    // 重新填充书签列表
    for (auto it = m_bookmarks.constBegin(); it != m_bookmarks.constEnd(); ++it) {
        QString bookmarkName = it.key();
        qint64 rowNumber = it.value();
        
        QString displayText = QString("%1 (行 %2)").arg(bookmarkName).arg(rowNumber);
        QListWidgetItem *item = new QListWidgetItem(displayText, bookmarkListWidget);
        item->setData(Qt::UserRole, rowNumber); // 存储行号数据
    }
    
    PRINT_DEBUG(QString("书签列表已更新，共 %1 个书签").arg(m_bookmarks.size()));
}

void MainWindow::onAddBookmarkTriggered()
{
    if (m_totalRows <= 0) {
        QMessageBox::information(this, tr("提示"), tr("请先打开一个CSV文件"));
        return;
    }
    
    // 获取当前选中的行或滚动条位置对应的行
    int currentRow = ui->verticalScrollBar->value() + 1; // 转换为1基行号
    
    // 弹出对话框让用户输入书签名称
    bool ok;
    QString bookmarkName = QInputDialog::getText(this, tr("添加书签"),
                                               tr("请输入书签名称:"),
                                               QLineEdit::Normal,
                                               QString(tr("行 %1")).arg(currentRow),
                                               &ok);
    
    if (ok && !bookmarkName.isEmpty()) {
        // 添加书签到映射中
        m_bookmarks[bookmarkName] = currentRow;
        
        PRINT_DEBUG(QString("添加书签: %1 -> 行 %2").arg(bookmarkName).arg(currentRow));
        
        // 更新书签列表
        updateBookmarkList();
    }
}

void MainWindow::onBookmarkItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    
    // 获取存储的行号
    qint64 rowNumber = item->data(Qt::UserRole).toLongLong();
    
    PRINT_DEBUG(QString("双击书签，跳转到行: %1").arg(rowNumber));
    
    // 跳转到对应行
    gotoRow(rowNumber);
}

void MainWindow::onRemoveBookmarkTriggered()
{
    // 获取书签列表控件
    QTabWidget *tabWidget = ui->dockWidgetContents->findChild<QTabWidget*>("filterBookmarkTabWidget");
    if (!tabWidget) {
        PRINT_DEBUG("无法获取tabWidget");
        return;
    }
    
    QListWidget *bookmarkListWidget = tabWidget->findChild<QListWidget*>("bookmarkListWidget");
    if (!bookmarkListWidget) {
        PRINT_DEBUG("无法获取bookmarkListWidget");
        return;
    }
    
    // 获取选中的项
    QListWidgetItem *selectedItem = bookmarkListWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::information(this, tr("提示"), tr("请先选择要移除的书签"));
        return;
    }
    
    // 获取书签名称（去掉行号信息）
    QString itemText = selectedItem->text();
    int pos = itemText.indexOf(" (");
    QString bookmarkName = (pos > 0) ? itemText.left(pos) : itemText;
    
    // 从映射中移除书签
    if (m_bookmarks.contains(bookmarkName)) {
        m_bookmarks.remove(bookmarkName);
        PRINT_DEBUG(QString("移除书签: %1").arg(bookmarkName));
        
        // 从列表中移除项
        delete selectedItem;
        
        // 更新书签列表
        updateBookmarkList();
    }
}
