#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QVariantList>

namespace Ui {
    class MainWindow;
}

class QProcess;
class QFileIconProvider;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void startSearching();
    void stopSearching();
    void toggleVisible(QSystemTrayIcon::ActivationReason);
    void toggleVisible();
    void readLocateOutput();
    void quit();
    void openFile();
    void showFile();
    void startUpdateDB();
    void showContextMenu(QPoint p);
    void animateEllipsis();
    void toggleFullPaths();

private:
    bool event(QEvent *event);
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *event);
    QString currentFilename();
    void openFile(QString filename);
    void showFile(QString filename);
    void restoreSettings();
    void saveSettings();
    void setLabelText(const QString& text);
    void setRedLabelText(const QString& text);
    virtual void keyPressEvent(QKeyEvent *event);

private:
    Ui::MainWindow *ui;
    QProcess* locate;
    bool reallyQuit;
    QVariantList lastState;
    QMenu* listWidgetContextMenu;
    QPalette originalLabelPalette;
    QFileIconProvider* iconProvider;
    QTimer* animateEllipsisTimer;
    int nextEllipsisCount;
    QTimer* readLocateOutputTimer;
    QString homePath;
    bool isSearching;
    bool isLabelTextRed;
};

#endif // MAINWINDOW_H
