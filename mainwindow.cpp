#include <QFileInfo>
#include <QtCore>
#include <QDir>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <tchar.h>
#include <psapi.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionDeploy_triggered()
{
    doDeploy();
}

void MainWindow::on_actionExit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionRefresh_triggered()
{
    DWORD aProcesses[1024*5] = {0};
    DWORD cbNeeded = 0;
    DWORD cProcesses = 0;
    unsigned int i;

    // Get the list of process identifiers.

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return ;

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the names of the modules for each process.

    for ( i = 0; i < cProcesses; i++ )
    {
        auto hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                     PROCESS_VM_READ,
                                     FALSE, aProcesses[i] );
        if (! hProcess)
            continue;

        char szModName[MAX_PATH] = {0};

        GetModuleFileNameExA( hProcess, 0, szModName,
                              sizeof(szModName) / sizeof(szModName[0]));
        QFileInfo fi(szModName);
        QString text = QString("%1 - %2").arg(fi.baseName()).arg(aProcesses[i]);
        QListWidgetItem* item = new QListWidgetItem(text, ui->listProcess);
        item->setData(Qt::UserRole, QVariant::fromValue(aProcesses[i]));
        ui->listProcess->addItem(item);

        // Release the handle to the process.

        CloseHandle( hProcess );
    }
}

int MainWindow::listModules(DWORD processID, const QString& dir)
{
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ,
                            FALSE, processID );
    if (NULL == hProcess)
        return 1;

    // Get a list of all the modules in this process.

    if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        QDir qt5Dir(".");
        QRegularExpression re("^Qt5.+\\.dll$");
        for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
        {
            char szModName[MAX_PATH] = {0};

            // Get the full path to the module's file.

            if ( GetModuleFileNameExA( hProcess, hMods[i], szModName,
                                       sizeof(szModName) / sizeof(szModName[0])))
            {
                QFileInfo fi(szModName);

                if (qt5Dir.dirName()==".")
                {
                    auto match = re.match(fi.fileName());
                    if (match.hasMatch() && fi.absoluteDir() != QDir(dir))
                    {
                        qt5Dir = fi.absoluteDir();
                        qDebug() << "Qt5 dir:" << qt5Dir << fi.absoluteFilePath();
                        break;
                    }
                }
            }
        }

        QStringList qtPluginsDirectories = {"bearer", "iconengines", "imageformats", "platforms", "styles"};
        auto pathVar = qEnvironmentVariable("PATH");
        auto paths = pathVar.split(";");
        for ( i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
        {
            char szModName[MAX_PATH] = {0};

            // Get the full path to the module's file.

            if ( GetModuleFileNameExA( hProcess, hMods[i], szModName,
                                       sizeof(szModName) / sizeof(szModName[0])))
            {
                QFileInfo fi(szModName);

                bool inPathVar = false;
                for (const auto& p : paths)
                {
                    QDir d(p);
                    if (d == fi.absoluteDir() && d != qt5Dir)
                    {
                        // in path variable, but not in Qt5 dir, skip
                        qDebug() << "skip" << fi.absoluteFilePath();
                        inPathVar = true;
                        break;
                    }
                }

                if (inPathVar)
                {
                    continue;
                }
                QString targetPath = dir;
                auto modDir = fi.absoluteDir().dirName();
                if (qtPluginsDirectories.contains(modDir.toLower())) {
                    // is a plugin directory, should create directory first
                    QDir d(dir + "/" + modDir);
                    if (!d.exists())
                        d.mkpath(d.absolutePath());
                    targetPath = d.absolutePath() + "/" +fi.fileName();
                } else {
                    targetPath = dir + "/" + fi.fileName();
                }
                QFile src(szModName);
                if (!QFile::exists(targetPath))
                {
                    src.copy(targetPath);
                    qDebug() << "copy " << QString(szModName) << "to" << QDir::toNativeSeparators(targetPath);
                }
            }
        }
    }

    // Release the handle to the process.

    CloseHandle( hProcess );

    QMessageBox::information(this, tr("Deploying completed!"), tr("All thirdparty dlls are copied."), QMessageBox::Ok);
    return 0;
}

void MainWindow::doDeploy()
{
    auto items = ui->listProcess->selectedItems();
    if (items.isEmpty())
        return;
    auto item = items[0];
    auto pid = item->data(Qt::UserRole).toLongLong();

    auto hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_READ,
                                 FALSE, pid );
    if (! hProcess)
        return;

    char szModName[MAX_PATH] = {0};

    GetModuleFileNameExA( hProcess, 0, szModName,
                          sizeof(szModName) / sizeof(szModName[0]));
    QFileInfo fi(szModName);

    CloseHandle( hProcess );

    listModules(pid, fi.absolutePath());
}

void MainWindow::on_listProcess_itemActivated(QListWidgetItem *)
{
    doDeploy();
}
