#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QStackedWidget;

namespace Ui
{
class MainWindow;
}

namespace palmvein
{

class CapturePage;
class ShowImagePage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 页面切换
    void showMainPage();
    void showCapturePage();
    void showShowImagePage();

private:

    Ui::MainWindow *ui;

    CapturePage *capturePage_;
    ShowImagePage *showImagePage_;
};

}

#endif // MAINWINDOW_H