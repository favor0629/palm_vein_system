#ifndef CAPTUREPAGE_H
#define CAPTUREPAGE_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>


#include <array>
#include <vector>
#include <memory>

#include "../../camera/CameraManager.h"
#include "../../ProcessingImage/ImageProcessor.hpp"
#include "../../driver/pwm/PwmController.hpp"
#include "../../lamp/InfraredLampController.hpp"
#include "../../lamp/AdaptiveInfraredController.hpp"

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

    // 每次 previewTimer_ 超时调用：显示最新相机帧，并使用同一帧驱动自动曝光。
    void updatePreview();

    // 用户点击 Capture：开始非阻塞连拍，而不是立即保存一张图。
    void onCaptureClicked();

    // 点击返回
    void onBackClicked();

    // 一轮连拍中的单次取帧步骤；由 captureTimer_ 驱动，避免阻塞 Qt 事件循环。
    void captureNextFrame();





private:

    // 初始化并启动相机；随后尽力初始化红外灯，灯失败不会导致相机预览失败
    bool initializeCamera();

    // 停止两个定时器、关闭灯光 PWM，并释放相机。
    void shutdownCamera();

    // 建立 8 路软件 PWM 和自适应灯光控制器。
    bool initializeLampController();

    // 从当前 BGR 帧计算 4 块 ROI 的平均灰度，并更新 8 路 PWM。
    void updateExposure(const cv::Mat &frame);

    // 返回中央大正方形 ROI，具体四分和裁剪由 ImageProcessor 完成。
    cv::Rect exposureLargeRoi(const cv::Size &imageSize) const;

    // 对 burstFrames_ 逐帧计算 FVIA，选择并保存质量最好的图像。
    void finishBurstCapture();
private:

    Ui::CapturePage *ui;

    std::unique_ptr<CameraManager> cameraManager_;
    ImageProcessor imageProcessor_;         // 图片处理

    // 约每 33 ms 刷新预览和检查自动曝光。
    QTimer *previewTimer_;

    // 只在连拍时运行，用于按固定间隔收集候选图像。
    QTimer *captureTimer_;

    // PWM 对象层级。后两个对象引用前一层，故成员顺序不可随意颠倒。
    std::unique_ptr<rpi::pwm::PwmController> pwmController_;
    std::unique_ptr<rpi::pwm::InfraredLampController> lampController_;
    std::unique_ptr<rpi::pwm::AdaptiveInfraredController> adaptiveLampController_;

    // 限制自动曝光控制的频率，避免每个预览帧都更新 PWM。
    QElapsedTimer exposureUpdateTimer_;
    // 当前一轮连拍已获得、尚未进行 FVIA 选优的 BGR 图像。
    std::vector<cv::Mat> burstFrames_;
    
    // 本轮已经尝试获取帧的次数，用于检测断流超时。
    int burstAttempts_ = 0;

    // GPIO/PWM 是否成功启动；失败时仍可手动拍摄。
    bool lampControlAvailable_ = false;
    // 自适应控制器最近一次判定的曝光稳定状态。
    bool exposureStable_ = false;
    // 防止同一时间启动两轮连拍。
    bool burstInProgress_ = false;

    // 输出文件编号，依次生成 image0.png、image1.png 等。
    int imageNumber_;
};

}

#endif // CAPTUREPAGE_H