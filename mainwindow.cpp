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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_fileName("")
    , m_csvReader(new CsvReader)
    , m_tableModel(new TableModel(this))  // 初始化数据模型
    , m_workerThread(new QThread)
{
    ui->setupUi(this);
    
    // 将数据模型设置到tableView中
    ui->tableView->setModel(m_tableModel);

    // 将CsvReader移动到工作线程
    m_csvReader->moveToThread(m_workerThread);
    m_workerThread->start();

    // 连接信号和槽
    connect(this, &MainWindow::initCsvReader, m_csvReader, &CsvReader::init);
    connect(m_csvReader, &CsvReader::initializationDataReady, 
            this, &MainWindow::onInitializationDataReceived);

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
    
    // TODO: 这里将添加实际的文件读取、解析和显示逻辑
    // 目前只是显示一个消息表示功能已触发
    statusBar()->showMessage(tr("筛选显示功能已触发，选中了%1列").arg(selectedColumns.size()), 3000);
    
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
    
    // 设置表格模型的表头
    m_tableModel->setHeaders(headers);
    
    endTiming(tr("生成筛选面板"));
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