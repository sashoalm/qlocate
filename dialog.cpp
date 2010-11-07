#include "dialog.h"
#include "ui_dialog.h"
#include <QProcess>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QDir>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QTimer>
#include <QMenu>
#include <QFileIconProvider>
#include <QDebug>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    // initialize misc. variables
    ui->setupUi(this);
    reallyQuit = false;
    locate = NULL;
    originalLabelPalette = ui->labelStatus->palette();
    iconProvider = new QFileIconProvider;

    // initialize the auto-search timer
    // there is no find/search button in our app
    // application starts searching automatically
    // a fixed time interval after last key typed by user
    QTimer* timer = new QTimer(this);
    timer->setInterval(500);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onFind()));
    connect(ui->lineEdit, SIGNAL(textEdited(QString)), timer, SLOT(start()));

    // initialize the tray icon
    // the application resides in the tray and when
    // user clicks on the tray icon the dialog is shown
    // this is to speed things up (no process loading and
    // initialization, so the app is more responsive)
    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(toggleDialogVisible(QSystemTrayIcon::ActivationReason)));
    trayIcon->setVisible(true);
    trayIcon->setIcon(QIcon(":/images/edit-find.svg"));
    QMenu* trayIconContextMenu = new QMenu;
    trayIconContextMenu->addAction("Update Database", this, SLOT(startUpdateDB()));
    trayIconContextMenu->addAction("Quit", this, SLOT(quit()));
    trayIcon->setContextMenu(trayIconContextMenu);

    // initialize list widget context menu
    // the user can right click on a found file to pop up the context menu
    // the context menu can be used to open the selected file (this is "Open File")
    // or to open the folder in which the selected file is (this is "Open Folder")
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    listWidgetContextMenu = new QMenu(this);
    listWidgetContextMenu->addAction("Open File", this, SLOT(openFile()));
    listWidgetContextMenu->addAction("Open Folder", this, SLOT(openFolder()));

    // initialize the checkboxes for various options
    oldCaseSensitive = false;
    oldUseRegExp = false;
    oldSearchOnlyHome = true;
    oldShowFullPath = false;
    connect(ui->checkBoxCaseSensitive, SIGNAL(toggled(bool)), this, SLOT(onFind()));
    connect(ui->checkBoxRegExp, SIGNAL(toggled(bool)), this, SLOT(onFind()));
    connect(ui->checkBoxSearchOnlyHome, SIGNAL(toggled(bool)), this, SLOT(onFind()));
    connect(ui->checkBoxShowFullPath, SIGNAL(toggled(bool)), this, SLOT(onFind()));
    connect(ui->listWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openFile()));
}

Dialog::~Dialog()
{
    delete iconProvider;
    delete ui;
}

void Dialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void Dialog::onFind()
{
    if (oldFindString == ui->lineEdit->text() &&
        oldShowFullPath == ui->checkBoxShowFullPath->isChecked() &&
        oldUseRegExp == ui->checkBoxRegExp->isChecked() &&
        oldCaseSensitive  == ui->checkBoxCaseSensitive->isChecked() &&
        oldSearchOnlyHome == ui->checkBoxSearchOnlyHome->isChecked())
    {
        return;
    }

    oldShowFullPath = ui->checkBoxShowFullPath->isChecked();
    oldUseRegExp = ui->checkBoxRegExp->isChecked();
    oldCaseSensitive  = ui->checkBoxCaseSensitive->isChecked();
    oldSearchOnlyHome = ui->checkBoxSearchOnlyHome->isChecked();
    oldFindString = ui->lineEdit->text();
    if (locate)
    {
        locate->terminate();
        locate->waitForFinished();
    }
    ui->listWidget->clear();
    
    ui->labelStatus->setPalette(originalLabelPalette);

    if (ui->lineEdit->text().isEmpty())
    {
        ui->labelStatus->setText(tr("Ready."));
        return;
    }

    lastPartialLine.clear();
    ui->labelStatus->setText(tr("Searching..."));

    locate = new QProcess(this);
    connect(locate, SIGNAL(readyReadStandardOutput()), this, SLOT(readLocateOutput()));
    connect(locate, SIGNAL(finished(int)), this, SLOT(locateFinished(int)));

    // the arguments to pass to locate
    QStringList args;
    args << "--existing" << "--basename";
    if (!ui->checkBoxCaseSensitive->isChecked())
        args << "--ignore-case";
    if (ui->checkBoxRegExp->isChecked())
        args << "--regexp";
    args << ui->lineEdit->text();

    locate->start("locate", args);
}

void Dialog::toggleDialogVisible(QSystemTrayIcon::ActivationReason reason)
{
    if (QSystemTrayIcon::Trigger == reason)
    {
        if (!isVisible())
        {
            ui->lineEdit->selectAll();
            ui->lineEdit->setFocus();
        }
        setVisible(!isVisible());
    }
}

void Dialog::closeEvent(QCloseEvent *event)
{
    if (!reallyQuit)
    {
        hide();
        event->ignore();
    }
}

void Dialog::readLocateOutput()
{
    int lastCount = ui->listWidget->count();

    lastPartialLine += QString::fromUtf8(locate->readAllStandardOutput());
    QStringList list = lastPartialLine.split('\n');
    lastPartialLine = list.back();
    list.pop_back();
    if (ui->checkBoxSearchOnlyHome->isChecked())
        list = list.filter(QRegExp(QString("^%1/").arg(QRegExp::escape(QDir::homePath()))));

    foreach (const QString& filename, list)
    {
        QListWidgetItem* item = new QListWidgetItem;
        item->setIcon(iconProvider->icon(QFileInfo(filename)));
        if (ui->checkBoxShowFullPath->isChecked())
        {
            item->setData(Qt::DisplayRole, filename);
        }
        else
        {
            item->setData(Qt::DisplayRole, filename.mid(filename.lastIndexOf('/')+1));
            item->setData(Qt::ToolTipRole, filename);
        }
        ui->listWidget->addItem(item);
    }

    int count = ui->listWidget->count();
    if (count != lastCount)
    {
        if (count == 1)
            ui->labelStatus->setText(tr("Searching (1 file found)..."));
        else
            ui->labelStatus->setText(QString(tr("Searching (%1 files found)...").arg(ui->listWidget->count())));
    }
}

void Dialog::quit()
{
    reallyQuit = true;
    close();
}

void Dialog::openFile()
{
    int role = ui->checkBoxShowFullPath->isChecked() ? Qt::DisplayRole : Qt::ToolTipRole;
    if (ui->listWidget->currentItem() && ui->listWidget->currentItem()->isSelected())
        QProcess::startDetached("/usr/bin/xdg-open", QStringList(ui->listWidget->currentIndex().data(role).toString()));
}

void Dialog::openFolder()
{
    int role = ui->checkBoxShowFullPath->isChecked() ? Qt::DisplayRole : Qt::ToolTipRole;
    if (ui->listWidget->currentItem() && ui->listWidget->currentItem()->isSelected())
        QProcess::startDetached("/usr/bin/xdg-open", QStringList(ui->listWidget->currentIndex().data(role).toString().remove(QRegExp("/[^/]+$"))));
}

void Dialog::startUpdateDB()
{
    QProcess::startDetached("gksudo", QStringList("updatedb"));
}

void Dialog::showContextMenu(QPoint p)
{
    listWidgetContextMenu->exec(ui->listWidget->mapToGlobal(p));
}

void Dialog::locateFinished(int /*exitCode*/)
{
    int count = ui->listWidget->count();
    if (count > 1)
    {
        ui->labelStatus->setText(QString(tr("%1 files found.").arg(ui->listWidget->count())));
    }
    else if (count == 1)
    {
        ui->labelStatus->setText(tr("1 file found."));
    }
    else
    {
        QPalette palette = originalLabelPalette;
        palette.setColor(ui->labelStatus->foregroundRole(), Qt::red);
        ui->labelStatus->setPalette(palette);
        ui->labelStatus->setText(tr("Nothing found."));
    }

    locate->deleteLater();
    locate = NULL;
}
