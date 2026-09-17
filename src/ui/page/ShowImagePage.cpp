#include "ShowImagePage.h"

#include "ui_ShowImagePage.h"


#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QPixmap>

namespace palmvein
{

ShowImagePage::ShowImagePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ShowImagePage)
{
    ui->setupUi(this);


    connect(
        ui->button_exit,
        &QPushButton::clicked,
        this,
        &ShowImagePage::onBackClicked
    );


    // ============================================================
    // Browse Image
    // ============================================================

    connect(
        ui->button_browse,
        &QPushButton::clicked,
        this,
        &ShowImagePage::onBrowseClicked
    );
}


ShowImagePage::~ShowImagePage()
{
    delete ui;
}


void ShowImagePage::onBackClicked()
{
    Q_EMIT backRequested();
}



// ================================================================
// 获取图片目录
// ================================================================

QString ShowImagePage::getImageDirectory() const
{
    return QCoreApplication::applicationDirPath()
           + "/../data/images";
}


// ================================================================
// 浏览图片
// ================================================================

void ShowImagePage::onBrowseClicked()
{
    const QString imageDirectory =
        getImageDirectory();


    /*
     * 确保目录存在。
     */
    QDir directory(imageDirectory);

    if (!directory.exists())
    {
        ui->label_show_image->setText(
            "Image directory does not exist.");

        return;
    }


    /*
     * 打开文件选择对话框。
     */
    const QString filePath =
        QFileDialog::getOpenFileName(
            this,
            "Select Image",
            imageDirectory,
            "Images (*.png *.jpg *.jpeg *.bmp)"
        );


    /*
     * 用户点击 Cancel。
     */
    if (filePath.isEmpty())
    {
        return;
    }


    /*
     * 显示选中的图片。
     */
    showImage(filePath);
}


// ================================================================
// 显示图片
// ================================================================

void ShowImagePage::showImage(
    const QString &filePath)
{
    QImage image(filePath);


    if (image.isNull())
    {
        ui->label_show_image->setText(
            "Failed to load image.");

        return;
    }


    /*
     * 根据 QLabel 当前大小缩放图片。
     *
     * KeepAspectRatio：
     * 保持原始宽高比。
     */
    QPixmap pixmap =
        QPixmap::fromImage(image);


    pixmap =
        pixmap.scaled(
            ui->label_show_image->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );


    /*
     * 设置到 QLabel。
     */
    ui->label_show_image->setPixmap(
        pixmap
    );


    /*
     * 显示图片文件名。
     *
     * 如果你不需要这个信息，可以删除。
     */
    const QFileInfo fileInfo(filePath);

    ui->label_show_image->setToolTip(
        fileInfo.fileName()
    );
}


}