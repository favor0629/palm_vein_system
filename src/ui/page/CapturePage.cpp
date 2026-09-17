#include "CapturePage.h"

#include "ui_CapturePage.h"

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QPixmap>

#include <opencv2/core.hpp>

namespace palmvein
{

CapturePage::CapturePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CapturePage)
    , cameraManager_(
          std::make_unique<CameraManager>())
    , previewTimer_(new QTimer(this))
    , imageNumber_(0)
{
    ui->setupUi(this);

    connect(
        ui->button_exit,
        &QPushButton::clicked,
        this,
        &CapturePage::onBackClicked
    );

    connect(
        ui->button_capture,
        &QPushButton::clicked,
        this,
        &CapturePage::onCaptureClicked
    );

    // ============================================================
    // 初始化摄像头
    // ============================================================

    if (!initializeCamera())
    {
        ui->label_status->setText(
            "Camera initialization failed.");

        ui->button_capture->setEnabled(false);

        return;
    }

    // ============================================================
    // 设置实时预览定时器
    // ============================================================

    connect(
        previewTimer_,
        &QTimer::timeout,
        this,
        &CapturePage::updatePreview
    );


    /*
     * 30 FPS
     */
    previewTimer_->start(33);
}


CapturePage::~CapturePage()
{
    shutdownCamera();

    delete ui;
}


// ================================================================
// 初始化摄像头
// ================================================================

bool CapturePage::initializeCamera()
{
    if (!cameraManager_->initialize())
    {
        return false;
    }

    if (!cameraManager_->startPreview())
    {
        return false;
    }

    ui->label_status->setText(
        "Camera Ready");

    return true;
}


// ================================================================
// 实时预览
// ================================================================

void CapturePage::updatePreview()
{
    if (!cameraManager_)
    {
        return;
    }

    cv::Mat frame;

    if (!cameraManager_->getLatestFrame(frame))
    {
        return;
    }

    if (frame.empty())
    {
        return;
    }


    /*
     * 当前 CameraManager 输出 BGR888。
     *
     * Qt 6 可以使用 Format_BGR888。
     */

    QImage image(
        frame.data,
        frame.cols,
        frame.rows,
        static_cast<int>(frame.step),
        QImage::Format_BGR888
    );


    /*
     * QImage 必须复制一份。
     *
     * 因为 frame 是当前函数的局部对象。
     */
    QImage imageCopy = image.copy();


    /*
     * 显示到 QLabel。
     *
     * cameraLabel 是 Qt Designer 中的 objectName。
     */

    ui->label_show->setPixmap(
        QPixmap::fromImage(imageCopy)
            .scaled(
                ui->label_show->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            )
    );
}


// ================================================================
// 拍照
// ================================================================

void CapturePage::onCaptureClicked()
{
    if (!cameraManager_)
    {
        return;
    }


    /*
     * 图片保存目录
     *
     * <程序目录>/../data/images
     */
    const QString imageDirectory =
        QCoreApplication::applicationDirPath()
        + "/../data/images";


    QDir directory(imageDirectory);


    /*
     * 如果目录不存在就创建。
     */
    if (!directory.exists())
    {
        if (!directory.mkpath("."))
        {
            ui->label_status->setText(
                "Failed to create image directory.");

            return;
        }
    }


    /*
     * image0.png
     * image1.png
     * image2.png
     */
    const QString filePath =
        directory.filePath(
            QString("image%1.png")
                .arg(imageNumber_)
        );


    /*
     * 调用 CameraManager 保存当前帧。
     */
    const bool success =
        cameraManager_->captureImage(
            filePath.toStdString()
        );


    if (success)
    {
        ui->label_status->setText(
            QString("Saved: %1")
                .arg(filePath)
        );

        ++imageNumber_;
    }
    else
    {
        ui->label_status->setText(
            "Capture failed.");
    }
}


// ================================================================
// 返回 MainPage
// ================================================================

void CapturePage::onBackClicked()
{
    Q_EMIT backRequested();
}


// ================================================================
// 关闭摄像头
// ================================================================

void CapturePage::shutdownCamera()
{
    if (previewTimer_)
    {
        previewTimer_->stop();
    }

    if (cameraManager_)
    {
        cameraManager_->shutdown();
    }
}

}