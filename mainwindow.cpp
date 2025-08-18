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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_fileName("")
    , m_csvReader(new CsvReader)
    , m_tableModel(new TableModel(this))  // 初始化数据模型
    , m_workerThread(new QThread)
    , m_delayedLoadTimer(new QTimer(this))
    , m_totalRows(0)
    , m_visibleRows(100) // 默认显示100行
    , m_currentStartRow(0) // 初始化当前起始行
{
    ui->setupUi(this);
    
    // 将数据模型设置到tableView中
    ui->tableView->setModel(m_tableModel);
    
    // 禁用QTableView自带的垂直滚动条
    ui->tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 将CsvReader移动到工作线程
    m_csvReader->moveToThread(m_workerThread);
    m_workerThread->start();

    // 连接信号和槽
    connect(this, &MainWindow::initCsvReader, m_csvReader, &CsvReader::init);
    connect(this, &MainWindow::requestRowsData, m_csvReader, &CsvReader::readRows); // 连接请求数据行的信号和槽
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
    
    // 获取所有选中的复选框
    QList<QCheckBox*> checkBoxList = contentWidget->findChildren<QCheckBox*>();
    QVector<QString> selectedColumns;
    for (QCheckBox *checkBox : checkBoxList) {
        if (checkBox->isChecked()) {
            selectedColumns.append(checkBox->text());
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
    
    // 清空之前的数据
    m_tableModel->clear();
    
    // 重新设置表头
    m_tableModel->setHeaders(m_headers);
    
    // 请求读取数据行（这里我们先读取前100行作为示例）
    emit requestRowsData(1, m_visibleRows); // 从第1行开始读取可视行数（跳过表头）
    
    endTiming(tr("筛选显示"));
}

void MainWindow::on_lineEdit_clowmn_name_textChanged(const QString &text)
{
    filterCheckboxes(text);
}

void MainWindow::filterCheckboxes(const QString &text)
{
    startTiming(tr("筛选列"));
    
    // 根据输入的文本过滤复选框
    QScrollArea *scrollArea = ui->dockWidgetContents->findChild<QScrollArea*>("scrollArea_select");
    if (!scrollArea) return;
    
    QWidget *contentWidget = scrollArea->widget();
    if (!contentWidget) return;

    // 获取当前布局
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(contentWidget->layout());
    if (!layout) return;

    // 创建临时列表存储所有复选框及其原始索引
    QList<QCheckBox*> checkBoxList = contentWidget->findChildren<QCheckBox*>();
    QList<QPair<QCheckBox*, int>> checkBoxWithIndex;
    
    // 获取所有复选框及其在布局中的索引（排除spacer）
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem* item = layout->itemAt(i);
        if (item) {
            QWidget* widget = item->widget();
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget);
            if (checkBox) {
                checkBoxWithIndex.append(qMakePair(checkBox, i));
            }
        }
    }
    
    // 根据输入文本过滤复选框
    for (auto pair : checkBoxWithIndex) {
        QCheckBox* checkBox = pair.first;
        // 根据输入文本决定是否显示复选框
        if (text.isEmpty() || text == "输入列名") {
            // 如果输入框为空或为默认文本，显示所有复选框
            checkBox->show();
        } else {
            // 根据文本过滤复选框
            if (checkBox->text().contains(text, Qt::CaseInsensitive)) {
                checkBox->show();
            } else {
                checkBox->hide();
            }
        }
    }
    
    // 强制重新计算布局
    layout->invalidate();
    contentWidget->updateGeometry();
    scrollArea->update();
    
    endTiming(tr("筛选列"));
}


void MainWindow::toggleSelectAll(bool select)
{
    startTiming(select ? tr("全选") : tr("清空"));
    
    // 查找scrollArea_select控件
    QScrollArea *scrollArea = ui->dockWidgetContents->findChild<QScrollArea*>("scrollArea_select");
    if (!scrollArea) return;
    
    // 获取滚动区域的内容部件
    QWidget *contentWidget = scrollArea->widget();
    if (!contentWidget) return;
    
    // 获取所有可见的复选框并设置它们的选中状态
    QList<QCheckBox*> checkBoxList = contentWidget->findChildren<QCheckBox*>();
    for (QCheckBox *checkBox : checkBoxList) {
        if (checkBox->isVisible()) {
            checkBox->setChecked(select);
        }
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
    
    // 根据表头生成复选框
    generateColumnCheckboxes(headers);
    
    // 保存表头信息
    m_headers = headers;
    
    // 设置表格模型的表头
    m_tableModel->setHeaders(headers);
    
    // 保存总行数（从CsvReader获取）
    m_totalRows = m_csvReader->getTotalRows();
    
    qDebug() << "总行数设置为:" << m_totalRows;
    
    // 更新滚动条范围
    updateScrollBarRange();
    
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
    Q_UNUSED(startRow);
    
    startTiming(tr("处理数据行"));
    
    qDebug() << "接收到数据行: startRow=" << startRow << ", 数据行数=" << rowData.rows.size();
    
    // 使用数据窗口方式更新表格模型中的数据
    m_tableModel->setDataWindow(rowData.rows, startRow);
    
    // 打印模型中的数据信息
    qDebug() << "模型更新后行数=" << m_tableModel->rowCount();
    if (m_tableModel->rowCount() > 0) {
        QModelIndex firstIndex = m_tableModel->index(0, 0);
        QVariant firstData = m_tableModel->data(firstIndex, Qt::DisplayRole);
        qDebug() << "第一行第一列数据=" << firstData.toString();
    }
    
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
            // 如果是复选框，删除它
            if (qobject_cast<QCheckBox*>(widget)) {
                layout->takeAt(i);
                delete widget;
                delete item;
            }
        }
    }
    
    // 为每个表头创建一个复选框，并插入到布局中（在spacer之前）
    for (const QString &header : headers) {
        QCheckBox *checkBox = new QCheckBox(header, contentWidget);
        checkBox->setChecked(true); // 默认选中
        // 插入到布局中倒数第二个位置（在spacer之前）
        layout->insertWidget(layout->count() - 1, checkBox);
    }
    
    endTiming(tr("创建复选框"));
}

void MainWindow::onVerticalScrollBarValueChanged(int value)
{
    qDebug() << "滚动条值变化: value=" << value << ", 当前起始行=" << m_currentStartRow;
    
    // 当滚动条值变化时，启动延迟加载定时器
    m_delayedLoadTimer->start(200); // 200ms延迟
    Q_UNUSED(value);
}

void MainWindow::onDelayedLoad()
{
    // 延迟加载数据
    int currentValue = ui->verticalScrollBar->value();
    
    qDebug() << "延迟加载触发: currentValue=" << currentValue << ", m_currentStartRow=" << m_currentStartRow;
    
    // 检查是否需要加载新数据
    if (currentValue != m_currentStartRow) {
        m_currentStartRow = currentValue;
        qDebug() << "请求数据: 起始行=" << m_currentStartRow + 1 << ", 行数=" << m_visibleRows;
        // 请求读取数据行，从当前滚动位置开始
        emit requestRowsData(m_currentStartRow + 1, m_visibleRows); // +1是因为跳过表头
    } else {
        qDebug() << "无需加载新数据，起始行未变化";
    }
}

void MainWindow::updateScrollBarRange()
{
    // 更新滚动条范围
    ui->verticalScrollBar->setRange(0, m_totalRows - m_visibleRows);
    ui->verticalScrollBar->setPageStep(m_visibleRows);
    qDebug() << "更新滚动条范围: 0-" << (m_totalRows - m_visibleRows) << ", 步长=" << m_visibleRows;
}