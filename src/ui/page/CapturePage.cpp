#include "CapturePage.h"

#include "ui_CapturePage.h"

#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QPixmap>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <array>
#include <exception>

#include "../../driver/pwm/SoftwarePwmChannel.hpp"
#include "../../tools/fvia.hpp"

#include "../../../test/debug.hpp"

namespace palmvein
{

namespace 
{
/*
* 本页面将现有模块串成一条链路：
* CameraManager 的最新 BGR 帧 -> Qt 预览显示；同一帧再进入曝光控制，
* 经由“四个 ROI 的平均灰度 -> AdaptiveInfraredController -> 八路 PWM”。
*
* 点击 Capture 后不会阻塞界面或停止预览。captureTimer_ 间隔取多张候选帧，
* 收齐后用 FVIA 比较质量，只保存质量最高的一张。
*/

// 预览刷新周期：33 ms 约为 30 FPS。
constexpr int k_preview_interval_ms = 33;

// 自动曝光控制周期。比预览慢，可降低 PWM 频繁调节造成的振荡
constexpr int k_exposure_update_interval_ms = 100;

// 每次拍照比较 5 帧；相邻候选帧相隔 120 ms。
constexpr int k_burst_frame_count = 5;
constexpr int k_burst_interval_ms = 120;

// 连续取不到相机帧时的最大尝试数，防止界面永久处在拍摄状态。
constexpr int k_burst_max_attempts = 15;

// 大 ROI 为画面中央正方形，边长为图像短边的 70%，之后切成 2 x 2 小 ROI。
constexpr double k_roi_square_ratio = 0.70;

// PWM 输出频率；应结合实际 LED 驱动与相机曝光时间调整。
constexpr std::uint32_t k_lamp_pwm_frequency_hz = 1000;

/*
 * BCM GPIO 配置：此处是需要按实际接线修改的唯一位置。
 * 数组下标是逻辑灯号，而不是 GPIO 编号：0..7 = N, NE, E, SE, S, SW, W, NW。
 */
constexpr std::array<unsigned int, 8> k_lamp_gpios = {5, 6, 12, 13, 16, 19, 20, 21};
// PwmController 内部通道 ID；这里与逻辑灯号一一对应。
constexpr rpi::pwm::AdaptiveInfraredController::ChannelMap k_lamp_channels =
    {0, 1, 2, 3, 4, 5, 6, 7};


/*
 * 权重矩阵 W 的行是 ROI、列是灯。控制器计算：PWM灯j增量 = kp * Σ(W[ROI][j] * ROI误差)。
 * ROI误差 = 目标灰度 - 当前灰度，故 ROI 偏暗时误差为正，周边灯的亮度会增加。
 * ROI 顺序固定为左上、右上、右下、左下：
 * 左上由 N/W/NW 控制，右上由 N/NE/E 控制，右下由 E/SE/S 控制，左下由 S/SW/W 控制。
 * 每盏相邻灯占 1/3 权重。
 */
constexpr rpi::pwm::AdaptiveInfraredController::WeightMatrix k_roi_lamp_weights = {{
    {{1.0 / 3.0, 0.0,       0.0,       0.0,       0.0,       0.0,       1.0 / 3.0, 1.0 / 3.0}},
    {{1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0, 0.0,       0.0,       0.0,       0.0,       0.0}},
    {{0.0,       0.0,       1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0, 0.0,       0.0,       0.0}},
    {{0.0,       0.0,       0.0,       0.0,       1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0, 0.0}}
}};
}



CapturePage::CapturePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CapturePage)
    , cameraManager_(
          std::make_unique<CameraManager>())
    , previewTimer_(new QTimer(this))
    , captureTimer_(new QTimer(this))
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
        DEBUG_ERROR("CapturePage", "Camera initialization failed; capture button disabled");
        ui->label_status->setText("Camera initialization failed.");

        ui->button_capture->setEnabled(false);

        return;
    }

    // ============================================================
    // 设置实时预览定时器
    // ============================================================

    // 相机初始化和启动必须先于预览定时器，避免首次刷新时相机尚未准备好。
    connect(
        previewTimer_,
        &QTimer::timeout,
        this,
        &CapturePage::updatePreview
    );

    // 连拍定时器只在用户点击 Capture 后启动，因此不会影响普通预览刷新。
    captureTimer_->setInterval(k_burst_interval_ms);
    connect(captureTimer_, &QTimer::timeout, this, &CapturePage::captureNextFrame);

    /*
     * 30 FPS
     */
    previewTimer_->start(33);
}


CapturePage::~CapturePage()
{
    // 先停定时器、关灯和相机，再释放 UI，避免异步回调访问已释放对象。
    shutdownCamera();

    delete ui;
}


// ================================================================
// 初始化摄像头
// ================================================================

bool CapturePage::initializeCamera()
{
    // initialize() 负责 libcamera 的配置；startPreview() 才开始持续产生 latestFrame。
    if (!cameraManager_->initialize())
    {
        DEBUG_ERROR("CapturePage", "CameraManager::initialize() returned false");
        return false;
    }

    if (!cameraManager_->startPreview())
    {
        DEBUG_ERROR("CapturePage", "CameraManager::startPreview() returned false");
        return false;
    }

    // ui->label_status->setText(
    //     "Camera Ready");

    // pwm 是可选的，gpio打不开时仍保留相机和手动拍摄
    lampControlAvailable_ = initializeLampController();
    if(!lampControlAvailable_)
    {
        DEBUG_WARN("CapturePage", "Infrared PWM unavailable; manual capture mode enabled");
        // 相机可独立工作；GPIO 不可用时仍允许手动采集，便于桌面调试。
        ui->label_status->setText("Camera ready. Infrared PWM is unavailable");
    }
    else
    {
        ui->label_status->setText("Camera ready. Adjusting infrared exposure");
    }
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

    // CameraManager 在内部锁保护下 clone 最新帧；返回后 frame 可独立安全使用。
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


    // 预览显示和曝光控制共用同一帧，避免额外向相机请求图像。
    updateExposure(frame);
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
    if (!cameraManager_ || burstInProgress_)
    {
        DEBUG_WARN("CapturePage", "Capture request ignored: camera unavailable or burst already running");
        return;
    }

    // 正常硬件模式下，只有灯光连续稳定后才能拍摄，：pwm不可用时允许手动拍摄
    if(lampControlAvailable_ && !exposureStable_)
    {
        DEBUG_WARN("CapturePage", "Capture request rejected because exposure is not stable");
        ui->label_status->setText("Exposure is still adjusting. Please wait for the capture-ready prompt.");
        return ;
    }

    // 初始化本轮连拍状态，禁用按钮可阻止用户重入冰启动第二个定时器
    burstFrames_.clear();
    burstAttempts_  = 0;
    burstInProgress_ = true;

    ui->button_capture->setEnabled(false);
    ui->label_status->setText(QString("capturing %1 frames for FVIA selection..").arg(k_burst_frame_count));

    // 立即取第一帧降低操作延迟：余下帧由QTimer异步取得，GUI始终可以响应
    captureNextFrame();
    if(burstInProgress_)
    {
        captureTimer_->start();
    }

    // /*
    //  * 图片保存目录
    //  *
    //  * <程序目录>/../data/images
    //  */
    // const QString imageDirectory =
    //     QCoreApplication::applicationDirPath()
    //     + "/../data/images";
    // QDir directory(imageDirectory);
    // /*
    //  * 如果目录不存在就创建。
    //  */
    // if (!directory.exists())
    // {
    //     if (!directory.mkpath("."))
    //     {
    //         ui->label_status->setText(
    //             "Failed to create image directory.");
    //         return;
    //     }
    // }
    // /*
    //  * image0.png
    //  * image1.png
    //  * image2.png
    //  */
    // const QString filePath =
    //     directory.filePath(
    //         QString("image%1.png")
    //             .arg(imageNumber_)
    //     );
    // /*
    //  * 调用 CameraManager 保存当前帧。
    //  */
    // const bool success =
    //     cameraManager_->captureImage(
    //         filePath.toStdString()
    //     );
    // if (success)
    // {
    //     ui->label_status->setText(
    //         QString("Saved: %1")
    //             .arg(filePath)
    //     );
    //     ++imageNumber_;
    // }
    // else
    // {
    //     ui->label_status->setText(
    //         "Capture failed.");
    // }
}


void CapturePage::captureNextFrame()
{
    // 不调用 CameraManager::captureImage()，因为我们先在内存中比较多帧，再只保存最佳帧
    ++burstAttempts_;

    cv::Mat frame;

    if(cameraManager_ && cameraManager_->getLatestFrame(frame) && !frame.empty())
    {
        burstFrames_.push_back(std::move(frame));
    }
    else
    {
        DEBUG_WARN("CapturePage", "No valid camera frame for burst attempt " << burstAttempts_);
    }

    // 拿到足够的候选帧之后，停止timer，进入评分和保存阶段
    if(static_cast<int>(burstFrames_.size()) >= k_burst_frame_count)
    {
        captureTimer_->stop();
        finishBurstCapture();
    }
    // 相机断流会导致取帧一直失败，此处恢复界面，避免永久“拍摄中”。
    else if(burstAttempts_ >= k_burst_max_attempts)
    {
        captureTimer_->stop();
        burstInProgress_ = false;
        ui->button_capture->setEnabled(true);
        ui->label_status->setText("Capture failed: timed out waiting for camera frames.");
        DEBUG_ERROR("CapturePage", "Burst capture timed out after " << burstAttempts_ << " attempts");
    }


}

void CapturePage::finishBurstCapture()
{
    // QTimer 已被停止，本轮可重新允许用户点击拍摄
    burstInProgress_ = false;

    ui->button_capture->setEnabled(true);

    if(burstFrames_.empty())
    {
        DEBUG_ERROR("CapturePage", "Cannot evaluate burst: no frames were collected");
        ui->label_status->setText("Capture failed: no frame received");
        return ;
    }

    /*
     * 图片保存目录
     *
     * <程序目录>/../data/images
     */
    const QString image_directory = QCoreApplication::applicationDirPath() + "/../data/images";

    QDir directory(image_directory);

    /*
     * 如果目录不存在就创建。
     */
    if(!directory.exists())
    {
        if(!directory.mkpath("."))
        {
            DEBUG_ERROR("CapturePage", "Failed to create image directory: " << image_directory.toStdString());
            ui->label_status->setText("Failed to create image directory.");
            return;
        }
    }

    /*
     * image0.png
     * image1.png
     * image2.png
     */
    const QString filepath = directory.filePath(QString("image%1.png").arg(imageNumber_));

    // 同一轮所有帧尺寸相同，用第一帧生成roi即可
    //const cv::Rect largeRoi = exposureLargeRoi(burstFrames_.front().size());
    const cv::Rect large_roi(0, 0, burstFrames_.front().cols, burstFrames_.front().rows);
    double best_quality = -1.0;
    std::size_t best_index = 0;

    try
    {
        for(std::size_t index = 0; index < burstFrames_.size(); ++index)
        {
            const cv::Mat gray = imageProcessor_.toGray(burstFrames_[index]);
            const cv::Mat largeRoiImage = imageProcessor_.cropROI(gray, large_roi);
            //const auto subRois = imageProcessor_.split4(largeRoiImage, 0.5, 0.5);

            /**
             * 计算 FVIA 质量分数，选择最高的帧保存。
             */
            double frame_quality = evaluateFVIA(
                largeRoiImage,
                cv::Rect(0, 0, largeRoiImage.cols, largeRoiImage.rows),
                T_VAR
            ).quality;

            if(frame_quality > best_quality)
            {
                best_quality = frame_quality;
                best_index = index;
            }

            // /**
            //  *  用四个子 ROI 的平均 FVIA 作为一帧的质量分数，避免单一区域主导结果。
            //  * 四个子 ROI 分别计算 FVIA，再平均，避免某一个局部区域独占分数。
            //  */
            // // double frameQuality = 0.0;
            // // for (const cv::Mat &subRoi : subRois)
            // // {
            // //     frameQuality += evaluateFVIA(
            // //         subRoi,
            // //         cv::Rect(0, 0, subRoi.cols, subRoi.rows),
            // //         T_VAR
            // //     ).quality;
            // // }
            // // frameQuality /= static_cast<double>(subRois.size());

            // // if (frameQuality > best_quality)
            // // {
            // //     best_quality = frameQuality;
            // //     best_index = index;
            // // }
        }
    }
    catch (const std::exception &error)
    {
        DEBUG_ERROR("CapturePage", "FVIA evaluation failed: " << error.what());
        ui->label_status->setText(QString("FVIA evaluation failed: %1").arg(error.what()));
        return;
    }

    // 只写入FVIA平均分最高的图
    const bool success = cv::imwrite(filepath.toStdString(), burstFrames_[best_index]);

    if(success)
    {
        ui->label_status->setText(
            QString("Saved best frame: %1 (FVIA %2)")
                .arg(filepath)
                .arg(best_quality, 0, 'f', 2)
        );
        ++imageNumber_;
    }
    else
    {
        DEBUG_ERROR("CapturePage", "Failed to save best frame: " << filepath.toStdString());
        ui->label_status->setText("Capture failed.");
    }
}


bool CapturePage::initializeLampController()
{
    try
    {
        /*
         * 三层对象的关系：
         * PwmController 管理 GPIO/PWM 通道；InfraredLampController 提供“亮度百分比”接口；
         * AdaptiveInfraredController 根据 ROI 误差计算下一次亮度。
         * 后两层保存下层对象的引用，成员声明顺序保证析构时引用不会悬空。
         */
        pwmController_ = std::make_unique<rpi::pwm::PwmController>();

        const auto gpio_chip = pwmController_->gpioChip();

        // 将每一盏逻辑灯关联到一个独立的软件pwm通道
        for(std::size_t index = 0; index < k_lamp_gpios.size(); ++index)
        {
            auto channel = std::make_unique<rpi::pwm::SoftwarePwmChannel>(
                gpio_chip, k_lamp_gpios[index], k_lamp_pwm_frequency_hz, 30.0
            );

            if(!pwmController_->addChannel(k_lamp_channels[index], std::move(channel)))
            {
                DEBUG_ERROR("CapturePage", "Failed to register PWM channel " << k_lamp_channels[index]
                    << " for GPIO " << k_lamp_gpios[index]);
                // 失败则返回
                return false;
            }
        }

        lampController_ = std::make_unique<rpi::pwm::InfraredLampController>(*pwmController_);
        adaptiveLampController_ = std::make_unique<rpi::pwm::AdaptiveInfraredController>(
            *lampController_, k_roi_lamp_weights, k_lamp_channels
        );
        // start() 启动八路 PWM，并应用 AdaptiveInfraredController 配置中的初始亮度。
        return adaptiveLampController_->start();        
    }
    catch(const std::exception& e)
    {
        DEBUG_ERROR("CapturePage", "Infrared PWM initialization exception: " << e.what());
        // 常见原因：没有 /dev/gpiochip0、权限不足或某 GPIO 已被其他进程占用。
        // 清理半初始化对象，调用方会自动退化到手动拍摄模式。
        adaptiveLampController_.reset();
        lampController_.reset();
        pwmController_.reset();
        return false;
    }
}

cv::Rect CapturePage::exposureLargeRoi(const cv::Size &image_size) const
{
    const int side = std::max(2, static_cast<int>(std::min(image_size.width, image_size.height) * k_roi_square_ratio));
    const int evenSide = side - (side % 2);
    const int x = (image_size.width - evenSide) / 2;
    const int y = (image_size.height - evenSide) / 2;

    return cv::Rect(x, y, evenSide, evenSide);
}

void CapturePage::updateExposure(const cv::Mat &frame)
{
    // 连拍期间不再调节 pwm， 保证五张候选帧处在同一照明条件下
    if(!lampController_ || !adaptiveLampController_ || burstInProgress_)
    {
        return ;
    }

    // 预览约 30 FPS；控制环限制为 10 Hz，避免每帧调光造成亮度震荡。
    if(exposureUpdateTimer_.isValid() && exposureUpdateTimer_.elapsed() < k_exposure_update_interval_ms)
    {
        return;
    }

    exposureUpdateTimer_.restart();

    // 曝光控制仅使用平均灰度
    try
    {
        const cv::Mat gray = imageProcessor_.toGray(frame);
        const cv::Rect largeRoi = exposureLargeRoi(gray.size());
        const cv::Mat largeRoiImage = imageProcessor_.cropROI(gray, largeRoi);
        const auto subRois = imageProcessor_.split4(largeRoiImage, 0.5, 0.5);
        rpi::pwm::AdaptiveInfraredController::RoiValues means{};

        for (std::size_t index = 0; index < subRois.size(); ++index)
        {
            means[index] = calculateMean(subRois[index]);
        }

        exposureStable_ = adaptiveLampController_->update(means);
    }
    catch (const std::exception& error)
    {
        exposureStable_ = false;
        DEBUG_ERROR("CapturePage", "Exposure processing failed: " << error.what());
        ui->label_status->setText(QString("Exposure processing failed: %1").arg(error.what()));
        return;
    }

    /*
     * update() 内部会低通滤波 ROI 灰度，计算“目标 - 当前”的误差，按权重分配给 8 灯，
     * 再判断全部 ROI 是否进入死区、PWM 是否几乎不变且已持续足够帧数。
     * 满足所有条件才返回 true，表示此时建议拍照。
     */
    if (exposureStable_)
    {
        ui->label_status->setText("Exposure is stable. Capture is recommended now.");
    }
    else
    {
        ui->label_status->setText("Adjusting infrared exposure...");
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
    // 先停止 Qt timer，避免资源释放过程中 timer 又触发 updatePreview/captureNextFrame。
    if (previewTimer_)
    {
        previewTimer_->stop();
    }

    if (captureTimer_)
    {
        captureTimer_->stop();
    }

    // stop() 会关闭八路 PWM，红外灯恢复为熄灭状态。
    if (adaptiveLampController_)
    {
        adaptiveLampController_->stop();
    }

    if (cameraManager_)
    {
        cameraManager_->shutdown();
    }
}

}