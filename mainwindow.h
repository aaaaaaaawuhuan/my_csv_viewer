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
#include <QElapsedTimer>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class CsvReader;
class TableModel;
QT_END_NAMESPACE

class MainWindow : public QMainWindow

{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void initCsvReader(const QString &fileName);

private slots:
    void on_action_open_triggered();
    void on_action_show_select_triggered();
    void on_pushButton_all_clicked();
    void on_pushButton_clear_clicked();
    void on_pushButton_filter_clicked();
    void on_lineEdit_clowmn_name_textChanged(const QString &text);
    void onInitializationDataReceived(const QVector<QString> &headers);

private:
    Ui::MainWindow *ui;
    QString m_fileName;
    CsvReader *m_csvReader;
    TableModel *m_tableModel;  // 添加数据模型
    QThread *m_workerThread;
    QElapsedTimer m_timer;  // 用于计时的计时器
    QMap<QString, qint64> m_performanceData;  // 存储性能数据
    void generateColumnCheckboxes(const QVector<QString> &headers);
    void toggleSelectAll(bool select);
    void filterCheckboxes(const QString &text);
    void startTiming(const QString &operation);
    void endTiming(const QString &operation);
    void updateStatusBarWithTimingInfo();
};

#endif // MAINWINDOW_H