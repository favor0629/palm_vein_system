#ifndef PALM_VEIN_CAMERA_MANAGER_H
#define PALM_VEIN_CAMERA_MANAGER_H

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <atomic>


#include <opencv2/core.hpp>
#include <libcamera/libcamera.h>

namespace palmvein
{
    class CameraManager
    {
        public:
            CameraManager();
            ~CameraManager();

            // 初始化摄像头系统
            bool initialize();

            // 彻底释放摄像头资源
            void shutdown();

            // 拍摄一张图片并保存为文件
            bool captureImage(const std::string &filePath);

            // 判断摄像头是否已经初始化
            bool isInitialized() const;

            bool isPreviewRunning() const;

            // 释放摄像头资源
            void release();

            // 启动实时预览
            bool startPreview();

            // 停止实时预览
            void stopPreview();

            // 获取当前最新的一帧
            bool getLatestFrame(cv::Mat &image);

        private:
            // =========================
            // libcamera 回调
            // =========================
            void requestComplete(libcamera::Request *request);

            // 从FrameBuffer读取图像并转换为OpenCV Mat
            bool frameBufferToMat(libcamera::FrameBuffer *buffer, cv::Mat& image);

            // 从DMA-BUF映射内存
            void* mapPlane(const libcamera::FrameBuffer::Plane &plane, size_t &mappedLength, size_t &offsetInMapping);

                // 解除DMA-BUF映射
            void unmapPlane(void* mappedMemory, size_t mappedLength);
        
        private:
            // libcamera
            std::unique_ptr<libcamera::CameraManager> cameraManager_;
            std::shared_ptr<libcamera::Camera> camera_;

            std::unique_ptr<libcamera::CameraConfiguration> configuration_;
            libcamera::Stream *stream_ = nullptr;

            std::unique_ptr<libcamera::FrameBufferAllocator> allocator_;
            //std::unique_ptr<libcamera::Request> request_;
            std::vector<std::unique_ptr<libcamera::Request>> requests_;

            // 图像参数
            unsigned int width_ = 1280;
            unsigned int height_ = 720;
            unsigned int stride_ = 0;

            
            libcamera::PixelFormat pixelFormat_;

            // 状态
            bool initialized_ = false;
            bool captureCompleted_ = false; // 采集流程是否结束
            bool captureSuccess_ = false;   // 采集是否成功

            /*
            * 摄像头刚启动时丢掉前几帧。
            *
            * 这是应用层策略，不是 libcamera 要求。
            */
            unsigned int warmupFramesRemaining_ = 5;

            /*
            * 使用 atomic 是因为：
            *
            * Qt 线程可能调用 stopPreview()
            *
            * libcamera 回调线程同时会调用 requestComplete()
            */
            std::atomic<bool> previewRunning_{false};

            // 保存的图像
            cv::Mat capturedImage_;

            cv::Mat latestFrame_; // 最新图像

            // 同步
            // std::mutex mutex_;
            // std::condition_variable condition_;

            mutable std::mutex mutex_;

    };
}   // namespace palmvein


#endif //PALM_VEIN_CAMERA_MANAGER_H