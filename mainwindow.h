#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Windows.h>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionDeploy_triggered();

    void on_actionExit_triggered();

    void on_actionRefresh_triggered();

    void on_listProcess_itemActivated(QListWidgetItem *);

private:
    Ui::MainWindow *ui;

    int listModules(DWORD processID , const QString &dir);
    void doDeploy();
};
#endif // MAINWINDOW_H
