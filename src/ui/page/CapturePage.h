#ifndef CAPTUREPAGE_H
#define CAPTUREPAGE_H

#include <QWidget>
#include <QTimer>

#include <memory>

#include "../../camera/CameraManager.h"

namespace Ui
{
class CapturePage;
}

namespace palmvein
{

class CapturePage : public QWidget
{
    Q_OBJECT

public:
    explicit CapturePage(QWidget *parent = nullptr);
    ~CapturePage();

Q_SIGNALS:

    void backRequested();

private Q_SLOTS:

    // 更新实时预览
    void updatePreview();

    // 点击拍照
    void onCaptureClicked();

    // 点击返回
    void onBackClicked();

private:

    bool initializeCamera();

    void shutdownCamera();

private:

    Ui::CapturePage *ui;

    std::unique_ptr<CameraManager>
        cameraManager_;

    QTimer *previewTimer_;

    int imageNumber_;
};

}

#endif // CAPTUREPAGE_H