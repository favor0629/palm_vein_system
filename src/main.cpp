

#include <opencv2/opencv.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "camera/CameraManager.h"


/**
 * qt相关
 */
#include <QApplication>
#include "ui/MainWindow.h"

/**
 * 测试摄像头的基本功能
 */
void test_camera()
{
     palmvein::CameraManager camera;

    // ============================================================
    // 1. 初始化摄像头
    // ============================================================

    if (!camera.initialize())
    {
        std::cerr
            << "[Test] Camera initialization failed."
            << std::endl;

        return ;
    }

    // ============================================================
    // 2. 启动实时预览
    // ============================================================

    if (!camera.startPreview())
    {
        std::cerr
            << "[Test] Failed to start camera preview."
            << std::endl;

        camera.shutdown();

        return ;
    }

    std::cout
        << "[Test] Camera preview started."
        << std::endl;

    std::cout
        << "[Test] Press 'q' or ESC to exit."
        << std::endl;


    // ============================================================
    // 3. 自动拍照相关变量
    // ============================================================

    int imageNumber = 0;

    // 程序启动之后，3 秒后第一次拍照
    auto lastCaptureTime =
        std::chrono::steady_clock::now();

    const auto captureInterval =
        std::chrono::seconds(10);


    // ============================================================
    // 4. 实时预览循环
    // ============================================================

    cv::namedWindow(
    "Camera Preview",
    cv::WINDOW_NORMAL
    );

    cv::resizeWindow(
        "Camera Preview",
        800,
        450
    );

    while (true)
    {
        cv::Mat frame;

        /*
         * 从 CameraManager 获取当前最新的一帧。
         *
         * getLatestFrame() 内部应该返回 clone 后的 Mat，
         * 因此这里可以安全地使用。
         */
        if (camera.getLatestFrame(frame))
        {
            if (!frame.empty())
            {
                cv::imshow(
                    "Camera Preview",
                    frame);
            }
        }


        // ========================================================
        // 5. 检查是否到了拍照时间
        // ========================================================

        auto currentTime =
            std::chrono::steady_clock::now();

        if (currentTime - lastCaptureTime >=
            captureInterval)
        {
            std::string filePath =
                "data/images/image" +
                std::to_string(imageNumber) +
                ".png";


            std::cout
                << "[Test] Capturing: "
                << filePath
                << std::endl;


            if (camera.captureImage(filePath))
            {
                std::cout
                    << "[Test] Saved successfully: "
                    << filePath
                    << std::endl;

                ++imageNumber;
            }
            else
            {
                std::cerr
                    << "[Test] Failed to capture image: "
                    << filePath
                    << std::endl;
            }

            /*
             * 无论这次拍照成功还是失败，
             * 下一次计时都从现在开始。
             */
            lastCaptureTime = currentTime;
        }


        // ========================================================
        // 6. OpenCV 键盘事件
        // ========================================================

        int key = cv::waitKey(1);

        /*
         * q / Q / ESC 都可以退出
         */
        if (key == 'q' ||
            key == 'Q' ||
            key == 27)
        {
            break;
        }
    }


    // ============================================================
    // 7. 停止预览并释放摄像头
    // ============================================================

    std::cout
        << "[Test] Stopping camera..."
        << std::endl;

    camera.stopPreview();

    camera.shutdown();

    cv::destroyAllWindows();

    std::cout
        << "[Test] Camera test finished."
        << std::endl;
}



int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    palmvein::MainWindow window;
    window.show();

    return application.exec();
}