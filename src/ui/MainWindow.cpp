#include "MainWindow.h"

#include "ui_MainWindow.h"

#include "CapturePage.h"
#include "ShowImagePage.h"

namespace palmvein
{

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , capturePage_(nullptr)
    , showImagePage_(nullptr)
{
    ui->setupUi(this);

    /*
     * MainWindow.ui 中应该已经放好了：
     *
     * QStackedWidget
     *
     * objectName:
     * stackedWidget
     */


    // ============================================================
    // 创建 CapturePage
    // ============================================================

    capturePage_ = new CapturePage(this);


    // ============================================================
    // 创建 ShowImagePage
    // ============================================================

    showImagePage_ = new ShowImagePage(this);


    // ============================================================
    // 加入 QStackedWidget
    // ============================================================

    ui->stackedWidget->addWidget(
        capturePage_);

    ui->stackedWidget->addWidget(
        showImagePage_);


    // ============================================================
    // 默认显示 MainPage
    // ============================================================

    ui->stackedWidget->setCurrentIndex(0);


    // ============================================================
    // MainPage 按钮
    // ============================================================

    connect(
        ui->button_capture,
        &QPushButton::clicked,
        this,
        &MainWindow::showCapturePage
    );

    connect(
        ui->button_show_image,
        &QPushButton::clicked,
        this,
        &MainWindow::showShowImagePage
    );


    // ============================================================
    // CapturePage 返回信号
    // ============================================================

    connect(
        capturePage_,
        &CapturePage::backRequested,
        this,
        &MainWindow::showMainPage
    );


    // ============================================================
    // ShowImagePage 返回信号
    // ============================================================

    connect(
        showImagePage_,
        &ShowImagePage::backRequested,
        this,
        &MainWindow::showMainPage
    );
}


MainWindow::~MainWindow()
{
    delete ui;
}


// ================================================================
// 页面切换
// ================================================================

void MainWindow::showMainPage()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void MainWindow::showCapturePage()
{
    ui->stackedWidget->setCurrentWidget(
        capturePage_);
}


void MainWindow::showShowImagePage()
{
    ui->stackedWidget->setCurrentWidget(
        showImagePage_);
}

}