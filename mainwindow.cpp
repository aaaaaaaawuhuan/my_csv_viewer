#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "csvreader.h"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QWidget>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_fileName("")
    , m_csvReader(new CsvReader)
    , m_workerThread(new QThread)
{
    ui->setupUi(this);

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
}

MainWindow::~MainWindow()
{
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_csvReader;
    delete m_workerThread;
    delete ui;
}

void MainWindow::on_action_open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open CSV File"),"",tr("CSV Files (*.csv)"));

    if(!fileName.isEmpty())
    {
        m_fileName = fileName;
        // 发送文件名给CsvReader进行初始化
        emit initCsvReader(fileName);
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

void MainWindow::on_lineEdit_clowmn_name_textChanged(const QString &text)
{
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
    
    // 获取所有复选框及其在布局中的索引
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
}


void MainWindow::toggleSelectAll(bool select)
{
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
}

void MainWindow::onInitializationDataReceived(const QVector<QString> &headers)
{
    // 显示dockWidget
    ui->dockWidget->show();
    
    // 根据表头生成复选框
    generateColumnCheckboxes(headers);
}

void MainWindow::generateColumnCheckboxes(const QVector<QString> &headers)
{
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
}