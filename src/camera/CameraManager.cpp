#include "CameraManager.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <libcamera/formats.h>

#include "../../../test/debug.hpp"


namespace palmvein
{


CameraManager::CameraManager()
{

}


CameraManager::~CameraManager()
{
    shutdown();
}

/**
 * 初始化摄像头系统
 *   程序启动
        │
        ▼
    创建 CameraManager
        │
        ▼
    启动 CameraManager
        │
        ▼
    枚举系统中的 Camera
        │
        ▼
    选择一个 Camera
        │
        ▼
    Acquire Camera
        │
        ▼
    生成 CameraConfiguration
        │
        ▼
    修改配置
        │
        ├── 分辨率：1280 × 720
        ├── PixelFormat：RGB888
        └── BufferCount：4
        │
        ▼
    validate()
        │
        ▼
    configure()
        │
        ▼
    获取 Stream
        │
        ▼
    创建 FrameBufferAllocator
        │
        ▼
    为 Stream 分配 FrameBuffer
        │
        ▼
    注册 requestCompleted 回调
        │
        ▼
    initialized_ = true
        │
        ▼
    初始化完成


    CameraManager
    │
    ├── Camera
    │      │
    │      ├── CameraConfiguration
    │      │        │
    │      │        └── Stream
    │      │                │
    │      │                └── FrameBuffer
    │      │
    │      └── Request
    │
    └── 其他 Camera
 */
bool CameraManager::initialize()
{
    if (initialized_)
    {
        DEBUG_WARN("Camera", "initialize() called while camera is already initialized");
        return true;
    }

    std::cout << "[Camera] Initializing libcamera..." << std::endl;
    
    // 1. 创建一个 CameraManager 对象,负责管理整个 libcamera 摄像头系统
    cameraManager_ = std::make_unique<libcamera::CameraManager>();

    // 2. 启动 CameraManager
    int ret = cameraManager_->start();

    if (ret < 0)
    {
        DEBUG_ERROR("Camera", "CameraManager::start failed, ret=" << ret);
        std::cerr << "[Camera] Failed to start CameraManager, ret = " << ret << std::endl;

        cameraManager_.reset();
        return false;
    }

    // 3.枚举摄像头
    const auto& cameras = cameraManager_->cameras();

    if (cameras.empty())
    {
        DEBUG_ERROR("Camera", "No camera detected");
        std::cerr << "[Camera] No camera detected." << std::endl;

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    // 输出摄像头数量
    std::cout << "[Camera] Camera count: " << cameras.size() << std::endl;

    // 4.选择 Camera
    // 当前项目只有一个OV5647，因此先使用第一个摄像头
    camera_ = cameras.front();      //准备操作的那一台物理摄像头

    // 摄像头获取失败
    if (!camera_)
    {
        DEBUG_ERROR("Camera", "Camera list returned a null camera");
        std::cerr << "[Camera] Failed to get camera." << std::endl;

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    // 输出摄像头ID
    std::cout << "[Camera] Camera ID: " << camera_->id() << std::endl;

    // 5. Acquire Camera
    /**
     * 向 libcamera 请求独占使用这台摄像头, 因为摄像头可能同时被多
     * 个程序或者组件请求使用, 使用完成之后要调用 release() 释放摄像头, 
     * 否则其他程序无法使用摄像头
     */
    ret = camera_->acquire();

    if (ret < 0)
    {
        DEBUG_ERROR("Camera", "Failed to acquire camera, ret=" << ret);
        std::cerr << "[Camera] Failed to acquire camera, ret = " << ret << std::endl;

        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    // 6. 生成摄像头配置
    /*
     * StillCapture表示我们希望获得用于静态图片拍摄的stream。
     * 使用这台摄像头进行静态图像拍摄
     * streamRole枚举类型定义了摄像头的不同使用场景, 例如视频录制、预览等
     * stream:摄像头向应用程序输出图像数据的一条数据通道
     * Camera
        │
        ├──────────→ Preview Stream
        │              640×480
        │
        ├──────────→ Video Stream
        │              1920×1080
        │
        └──────────→ StillCapture Stream
                        1280×720
     */
    configuration_ = camera_->generateConfiguration({libcamera::StreamRole::Viewfinder});

    if (!configuration_)
    {
        std::cerr << "[Camera] Failed to generate configuration." << std::endl;

        camera_->release();
        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    // 输出摄像头支持的分辨率和像素格式
    if (configuration_->size() == 0)
    {
        std::cerr << "[Camera] Empty camera configuration." << std::endl;

        configuration_.reset();

        camera_->release();
        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    // 获取第一个 Stream 的配置
    auto& streamConfig = configuration_->at(0);

    /*
    * 请求输出RGB888。
    *  R | G | B
     *  --+---+--
     *  8   8   8 bit
    * libcamera 的 RGB888 数据在转换为 OpenCV 图像时，
    * 需要显式转换为 OpenCV 使用的 BGR 顺序。
     */
    streamConfig.pixelFormat = libcamera::formats::RGB888;  // 摄像头输出 RGB888 格式的图像

    /**
     * 设置的是“希望使用”的配置，不一定是最终实际配置，因为硬件可能不支持
     */
    streamConfig.size = {width_, height_};

    /*
     * 设置少量缓冲区用于请求。为这个 Stream 准备 4 个 FrameBuffer
     * libcamera会在内部维护一个缓冲区队列，确保摄像头采集数据时有足够的缓冲区可用。
     * 因为摄像头是持续产生数据的。摄像头可以使用其中一个 Buffer 的同时，
     * 应用程序处理另一个 Buffer。这实际上就是一种缓冲队列 / pipeline buffering
     */
    streamConfig.bufferCount = 4;

    /*
     * 7.
     * validate()会根据硬件真实能力调整配置。
     * 作用：检查你提出的摄像头配置是否符合硬件
     * 和 libcamera pipeline 的实际能力，并在必要时调整配置
     * 
    * RGB888 是否真的能由你的摄像头 pipeline 直接提供，
     * 需要由 validate() 和实际平台支持来决定
     */
    auto validation = configuration_->validate();

    if (validation == libcamera::CameraConfiguration::Invalid)
    {
        std::cerr << "[Camera] Invalid camera configuration." << std::endl;

        configuration_.reset();

        camera_->release();
        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    if (validation == libcamera::CameraConfiguration::Adjusted)
    {
        std::cout << "[Camera] Camera configuration was adjusted." << std::endl;
    }

    /**
     * 8.
     * 重新获取最终的 stream 配置，确保我们使用的是 libcamera 调整后的配置
     * 例如，libcamera可能会调整分辨率、像素格式等，以确保与硬件兼容
     * 这一步是必要的，因为我们需要知道最终的配置参数，以便正确处理摄像头输出的数据
     */
    streamConfig = configuration_->at(0);

    /**
     * 获取stride是因为实际FrameBuffr的每行字节数可能大于width * bytesPerPixel。
     * 例如，摄像头可能会在每行数据后添加一些填充字节，以满足硬件对内存对齐的要求。
     * 因此，在处理图像数据时，不能简单地假设每行的字节数等于 width * bytesPerPixel，
     * 而是应该使用stride来正确地访问每行数据。后面把 FrameBuffer 转成 cv::Mat 时，
     * 必须正确处理 stride
     * eg.
     * |------ 1280个像素 ------| padding 
     * |3840 bytes             | 256B   |
     */
    width_ = streamConfig.size.width;
    height_ = streamConfig.size.height;
    stride_ = streamConfig.stride;
    pixelFormat_ = streamConfig.pixelFormat;

    std::cout << "[Camera] Final configuration:" << std::endl;

    std::cout
        << "          Size       = "
        << width_
        << " x "
        << height_
        << std::endl;

    std::cout
        << "          PixelFormat = "
        << pixelFormat_.toString()
        << std::endl;

    std::cout
        << "          Stride      = "
        << stride_
        << std::endl;

    std::cout
        << "          FrameSize   = "
        << streamConfig.frameSize
        << std::endl;

    /*
     * 应用最终配置。
     * 前面generateConfiguration()只是生成一个配置对象，validate()只是检查和调整配置，
     * 真正的配置应用是在configure()中完成的。
     */
    ret = camera_->configure(configuration_.get());

    if (ret < 0)
    {
        std::cerr << "[Camera] Failed to configure camera, ret = " << ret << std::endl;

        configuration_.reset();

        camera_->release();
        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    // configure() 可能调整像素格式、尺寸或 stride，后续必须使用最终配置。
    const auto &finalStreamConfig = configuration_->at(0);
    width_ = finalStreamConfig.size.width;
    height_ = finalStreamConfig.size.height;
    stride_ = finalStreamConfig.stride;
    pixelFormat_ = finalStreamConfig.pixelFormat;

    std::cout << "[Camera] Configured pixel format: "
              << pixelFormat_.toString()
              << ", stride: " << stride_
              << std::endl;

    if (pixelFormat_ != libcamera::formats::RGB888 &&
        pixelFormat_ != libcamera::formats::BGR888)
    {
        std::cerr << "[Camera] Unsupported configured pixel format: "
                  << pixelFormat_.toString() << std::endl;

        configuration_.reset();
        camera_->release();
        camera_.reset();
        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    /*
     * 9. 获取stream
     * configure()之后，再读取一次stream配置。摄像头图像数据的输出通道
     */
    stream_ = configuration_->at(0).stream();

    if (!stream_)
    {
        std::cerr << "[Camera] Failed to obtain stream." << std::endl;

        camera_->release();
        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    /*
     * 10. 为stream申请FrameBuffer
     * 为stream申请FrameBuffer。负责为指定的 Stream 分配 FrameBuffer
     * FrameBuffer：摄像头产生的图像数据最终必须放到内存中
     */
    allocator_ = std::make_unique<libcamera::FrameBufferAllocator>(camera_);

    /**
     * 11. 分配FrameBuffer
     * streamConfig.bufferCount = 4;表示我希望有 4 个 Buffer，
     * allocator_->allocate(stream_)会尝试为这个 Stream 分配 4 个 FrameBuffer。
     */
    ret = allocator_->allocate(stream_);    //给这个 Stream 实际分配 FrameBuffer。

    if (ret < 0)
    {
        std::cerr << "[Camera] Failed to allocate buffers, ret = " << ret << std::endl;

        allocator_.reset();
        configuration_.reset();

        camera_->release();
        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    // 输出实际分配的FrameBuffer数量
    const auto& buffers = allocator_->buffers(stream_);

    if (buffers.empty())
    {
        std::cerr << "[Camera] No frame buffers allocated." << std::endl;

        allocator_.reset();
        configuration_.reset();

        camera_->release();
        camera_.reset();

        cameraManager_->stop();
        cameraManager_.reset();

        return false;
    }

    std::cout << "[Camera] Allocated " << buffers.size() << " buffers." << std::endl;

    /*
     * 连接request完成信号。
     *
     * 当摄像头完成一帧数据采集之后，
     * libcamera会调用requestComplete()。
     */
    /**
     * 12. 注册 requestCompleted 回调
     * libcamera 是异步采集模型
     */
    camera_->requestCompleted.connect(this, &CameraManager::requestComplete);

    initialized_ = true;

    std::cout << "[Camera] Initialization successful." << std::endl;

    return true;
}

// /**
//  * 作用：拍摄一张图片并保存为文件
//  * 
//  * 创建一次采集请求 → 给请求绑定 FrameBuffer 
//  * → 启动 Camera → 把 Request 放入队列 
//  * → 等待异步回调完成 → 从 FrameBuffer 获取图像
//  * → 转换为 cv::Mat → 保存 JPEG。
//  * captureImage()
//         │
//         ├── 创建 Request
//         │
//         ├── Request 绑定 FrameBuffer
//         │
//         ├── 清理采集状态
//         │
//         ├── Camera.start()
//         │
//         ├── queueRequest()
//         │
//         ▼
//     摄像头异步采集
//         │
//         ▼
//     requestCompleted
//         │
//         ▼
//     requestComplete()
//         │
//         ├── FrameBuffer → cv::Mat
//         ├── warmup
//         └── condition_.notify_one()
//         │
//         ▼
//     captureImage() 被唤醒
//         │
//         ├── Camera.stop()
//         ├── 检查 capturedImage_
//         └── cv::imwrite()
//  */
// bool CameraManager::captureImage(const std::string& filePath)
// {
//     /**
//      * 第一步：检查是否初始化
//      */
//     if (!initialized_)
//     {
//         std::cerr<< "[Camera] Camera has not been initialized."<< std::endl;

//         return false;
//     }

//     /**
//      * 第二步：检查核心资源
//      *  对象	    作用
//         camera_	    控制具体摄像头
//         stream_	    图像数据输出通道
//         allocator_	管理 FrameBuffer
//      */
//     if (!camera_ || !stream_ || !allocator_)
//     {
//         std::cerr << "[Camera] Camera resources are invalid." << std::endl;

//         return false;
//     }

//     /*
//      * 第三步：创建一个capture request。
//      * Request是libcamera的核心概念，表示一次采集请求。
//      * createRequest()并没有开始拍摄，只是创建一个请求对象，
//      * 之后需要通过queueRequest()提交给摄像头。
//      */
//     request_ = camera_->createRequest();

//     if (!request_)
//     {
//         std::cerr << "[Camera] Failed to create request." << std::endl;

//         return false;
//     }

//     // 获取初始化阶段申请的 FrameBuffer。
//     const auto& buffers = allocator_->buffers(stream_);

//     if (buffers.empty())
//     {
//         std::cerr << "[Camera] No buffers available." << std::endl;

//         request_.reset();

//         return false;
//     }

//     /*
//      * 这里使用第一个buffer。
//      * 需要优化：如果有多个buffer，可以轮询使用，避免每次都使用同一个buffer。
//      */
//     libcamera::FrameBuffer *buffer = buffers[0].get();

//     /**
//      * 告诉这个 Request：这个 Stream 获取的图像数据，要放进这个 FrameBuffer
//      * 一个request至少需要绑定一个stream和对应的buffer，才能提交给摄像头进行采集。
//      * 绑定之后，摄像头采集到的图像数据就会写入这个buffer中，之后可以从buffer中读取图像数据。
//      * 绑定的过程是通过addBuffer()实现的。
//      */
//     int ret = request_->addBuffer(stream_, buffer);

//     if (ret < 0)
//     {
//         std::cerr<< "[Camera] Failed to add buffer to request, ret = "<< ret<< std::endl;

//         request_.reset();

//         return false;
//     }

//     /*
//      * 第四步：清理上一次采集状态
//      * 这里是为了准备一次新的拍摄任务
//      * 如果不清理，可能会导致上一次采集的状态影响当前采集，造成错误的结果。
//      * warmupFramesRemaining_ = 5;意味着在正式采集图像之前，
//      * 摄像头会先采集5帧图像作为预热帧，这些帧不会被保存，只是为了让摄像头稳定下来，
//      * 确保最终采集的图像质量。这样做的目的是为了避免摄像头在刚启动时可能存在的曝光、
//      * 白平衡等参数不稳定的问题，确保最终采集的图像质量。
//      * warmupFramesRemaining_ 并不是 libcamera 要求的，而是应用层策略。
//      */
//     {
//         std::lock_guard<std::mutex> lock(mutex_);

//         captureCompleted_ = false;
//         captureSuccess_ = false;
//         warmupFramesRemaining_ = 5;
//         capturedImage_.release();
//     }

//     /*
//      * 第五步：启动 Camera
//      */
//     ret = camera_->start();  // camera开始运行

//     if (ret < 0)
//     {
//         std::cerr<< "[Camera] Failed to start camera, ret = "<< ret<< std::endl;

//         request_.reset();

//         return false;
//     }

//     std::cout<< "[Camera] Capturing..."<< std::endl;

//     /*
//      * 第六步：将request提交给摄像头。
//      * 把这个 Request 提交给摄像头，让摄像头按照这个 Request 执行一次采集。
//      * Camera
//         │
//         │ queueRequest()
//         ▼
//         Request进入Camera处理队列
//         │
//         ▼
//         摄像头采集
//         │
//         ▼
//         图像写入FrameBuffer
//      */
//     ret = camera_->queueRequest(request_.get());

//     if (ret < 0)
//     {
//         std::cerr << "[Camera] Failed to queue request, ret = " << ret << std::endl;

//         camera_->stop();

//         request_.reset();

//         return false;
//     }

//     /*
//      * 第七步：等待request完成。
//      *
//      * 这里不会忙等待，而是由condition_variable阻塞线程。
//      * queueRequest()是异步操作，调用函数之后，摄像头会在后台进行采集，
//      * 采集完成后会触发requestCompleted信号，
//      */
//     {
//         std::unique_lock<std::mutex> lock(mutex_);

//         /**
//          * 等待request完成的条件是captureCompleted_为true，
//          * 这个变量会在requestComplete()中被设置为true，表示采集已经完成。
//          * condition_.wait_for()会阻塞当前线程，直到满足条件或者超时。
//          * 超时时间设置为10秒，如果10秒内采集没有完成，就会返回false，表示采集失败。
//          * 这里使用lambda表达式作为条件，可以直接访问类的成员变量captureCompleted_。
//          * 这种方式比使用传统的条件变量更简洁，也更安全，因为可以避免忘记解锁mutex的问题。
//          * 这里使用wait_for()而不是wait()，是为了防止摄像头采集过程中出现异常情况导致线程永久阻塞，
//          * 设置超时可以让程序在一定时间内没有采集完成时，能够及时返回错误，进行错误处理。
//          */
//         const bool completed = condition_.wait_for(lock, std::chrono::seconds(10),
//                 [this]()
//                 {
//                     return captureCompleted_;
//                 });

//         if (!completed)
//         {
//             std::cerr<< "[Camera] Capture timeout." << std::endl;

//             camera_->stop();
//             request_.reset();

//             return false;
//         }
//     }

//     /*
//      * 第八步：停止摄像头。
//      * 拍照是按需启动、采集一帧、停止。
//      */
//     camera_->stop();

//     /**
//      * 第九步：检查采集是否成功
//      */
//     bool success = false;

//     {
//         std::lock_guard<std::mutex> lock(mutex_);

//         success = captureSuccess_;
//     }

//     /*
//      * 检查图像是否真的成功获取。
//      */
//     if (!success || capturedImage_.empty())
//     {
//         std::cerr << "[Camera] Failed to capture image." << std::endl;

//         request_.reset();

//         return false;
//     }

//     /*
//      * 第十步：保存为JPG。
//      * 这里使用OpenCV的imwrite()函数将cv::Mat保存为JPEG文件。
//      * 这个函数会根据文件扩展名自动选择保存格式，这里是.jpg
//      * 这里没有设置JPEG质量参数，使用OpenCV的默认值，通常是95。
//      * 如果需要更高的质量，可以使用imwrite()的第三个参数设置
//      */
//     if (!cv::imwrite(filePath, capturedImage_))
//     {
//         std::cerr << "[Camera] Failed to save image: " << filePath << std::endl;

//         request_.reset();

//         return false;
//     }

//     std::cout << "[Camera] Image saved successfully: " << filePath << std::endl;

//     request_.reset();

//     return true;
// }




// /**
//  * libcamera请求完成后的回调
//  */
// void CameraManager::requestComplete(libcamera::Request *request)
// {
//     /**
//      * 第一步：检查 Request 是否为空
//      */
//     if (!request)
//     {
//         return;
//     }

//     std::cout << "[Camera] Request completed." << std::endl;

//     /*
//      * 第二步：判断 Request 是否真正完成 
//      * Request可能因为camera stop而取消。不能认为只要进入 requestComplete()，
//      * 就代表图像采集成功。
//      */
//     if (request->status() != libcamera::Request::RequestComplete)
//     {
//         std::cerr << "[Camera] Request was not completed successfully." << std::endl;

//         {
//             std::lock_guard<std::mutex> lock(mutex_);

//             captureSuccess_ = false;
//             captureCompleted_ = true;   //这次 Request 已经结束了，但是失败了。
//         }

//         condition_.notify_one();

//         return;
//     }

//     /**
//      * warmupFramesRemaining_ > 0 表示摄像头还在预热阶段，这些帧不会被保存，
//      * 只是为了让摄像头稳定下来。
//      * 异步循环
//      *      ┌───────────────────┐
//             │                   │
//             ▼                   │
//         queueRequest()          │
//             │                   │
//             ▼                   │
//             Camera              │
//             │                   │
//             ▼                   │
//         Request完成             │
//             │                   │
//             ▼                   │
//         requestComplete()       │
//             │                   │
//             ▼                   │
//         warmup > 0 ?            │
//             │                   │
//             是                  │
//             │                   │
//             ▼                   │
//             reuse()             │
//             │                   │
//             └───────────────────┘
//      */
//     if (warmupFramesRemaining_ > 0)
//     {
//         --warmupFramesRemaining_;
//         /**
//          * 复用 Request，Request 执行完成之后，这个 Request 已经处于：完成状态
//          * 如果想再次提交它，需要让它重新进入可复用状态。
//          * request->reuse(...)把已经完成的 Request 重置，使它可以再次用于下一次采集
//          * ReuseBuffers表示Request 重新使用之前已经绑定的 Buffer。
//          */
//         request->reuse(libcamera::Request::ReuseBuffers);

//         if (camera_->queueRequest(request) < 0)
//         {
//             std::lock_guard<std::mutex> lock(mutex_);
//             captureSuccess_ = false;
//             captureCompleted_ = true;
//             condition_.notify_one();
//         }

//         return;
//     }

//     // 现在开始真正处理这一帧图像。
//     const auto& buffers = request->buffers();

//     if (buffers.empty())
//     {
//         std::cerr << "[Camera] Completed request contains no buffer." << std::endl;

//         {
//             std::lock_guard<std::mutex> lock(mutex_);

//             captureSuccess_ = false;
//             captureCompleted_ = true;
//         }

//         condition_.notify_one();

//         return;
//     }

//     /*
//      * 当前系统只配置了一个stream，
//      * 因此读取map中的第一个buffer。
//      * request->buffers()返回的是一个map，key是Stream，value是对应的FrameBuffer。
//      * 这里使用buffers.begin()->second获取第一个Stream对应的FrameBuffer。
//      * 如果系统配置了多个Stream，需要根据Stream选择对应的FrameBuffer。
//      */
//     libcamera::FrameBuffer *buffer = buffers.begin()->second;

//     /**
//      * 第三步：从FrameBuffer读取图像并转换为OpenCV Mat
//      * 这里调用frameBufferToMat()函数，将摄像头采集到的图像数据从FrameBuffer中读取出来，并转换为OpenCV的Mat格式，方便后续处理和保存。
//      * 这里完成：
//      *  libcamera
//             ↓
//         FrameBuffer
//             ↓
//         frameBufferToMat()
//             ↓
//         cv::Mat
//      */
//     cv::Mat image;

//     const bool success = frameBufferToMat(buffer, image);

//     {
//         std::lock_guard<std::mutex> lock(mutex_);

//         if (success)
//         {
//             capturedImage_ = image;
//             captureSuccess_ = true;
//         }
//         else
//         {
//             capturedImage_.release();
//             captureSuccess_ = false;
//         }

//         captureCompleted_ = true;
//     }

//     condition_.notify_one();
// }



/**
 * 从FrameBuffer读取图像并转换为OpenCV Mat
 * 负责把 libcamera 提供的 FrameBuffer 中的 DMA-BUF 数据，转换成 OpenCV 可以直接处理的 cv::Mat
 * buffer：摄像头已经采集好的图像数据
 * image：输出参数，转换后的 OpenCV Mat
 *  Camera
    │
    ▼
    Request 完成
    │
    ▼
    FrameBuffer
    │
    │  DMA-BUF fd
    ▼
    mmap()
    │
    ▼
    CPU 可访问的内存地址
    │
    │  按 stride 逐行读取
    ▼
    cv::Mat(CV_8UC3)
    │
    ▼
    capturedImage_
    │
    ▼
    cv::imwrite()
    │
    ▼
    JPEG 文件
 */
bool CameraManager::frameBufferToMat(libcamera::FrameBuffer *buffer, cv::Mat& image)
{
    if (!buffer)
    {
        return false;
    }

    /*
     * 当前版本的实现明确要求：
     *
    * RGB888
     * 单平面
     *
     * 这样可以直接转换成CV_8UC3。
     */
    const bool isRgb = pixelFormat_ == libcamera::formats::RGB888;

    if (!isRgb && pixelFormat_ != libcamera::formats::BGR888)
    {
        std::cerr << "[Camera] Unsupported pixel format: " << pixelFormat_.toString() << std::endl;

        return false;
    }

    /**
     * 这里开始进入 libcamera FrameBuffer 的内部结构，
     * 一个 FrameBuffer 不一定只有一块内存。它可能由多个 Plane 组成
     * eg.
     * FrameBuffer
        │
        ├── Plane 0
        ├── Plane 1
        └── Plane 2
     */
    const auto planes = buffer->planes();   //获取这个 FrameBuffer 中的所有 Plane。

    if (planes.empty())
    {
        std::cerr << "[Camera] FrameBuffer contains no planes." << std::endl;

        return false;
    }

    /*
    * RGB888应该是一个plane。
     *
     * 如果实际pipeline提供多plane格式，
     * 这个基础版本不会继续处理。
     */
    if (planes.size() != 1)
    {
        std::cerr << "[Camera] Expected 1 plane, got " << planes.size() << std::endl;

        return false;
    }

    /**
     * 当前只处理第一个plane。
     * Plane
        │
        ├── fd       → 这块 DMA-BUF 对应的文件描述符
        ├── offset   → 数据在 buffer 中的偏移
        └── length   → 这块区域的长度
       fd：DMA-BUF的文件描述符，表示这块内存区域在内核中的标识符。通过这个fd，用户空间可以访问这块内存。
       offset：数据在整个 FrameBuffer 中的偏移量，表示这块 Plane 的数据在整个 FrameBuffer 中的位置。
       length：这块 Plane 的数据长度，表示这块 Plane 占用的内存大小。这个长度可能大于实际图像数据的大小，因为可能包含一些填充字节。
     */
    const auto& plane = planes[0];

    if (!plane.fd.isValid())
    {
        std::cerr << "[Camera] Invalid DMA-BUF fd." << std::endl;
        
        return false;
    }

    /*
     * 获取系统page size。Linux 的虚拟内存是以 page 为基本管理单位。通常是4k
     * mmap要求offset通常是page-aligned。
     */
    const long pageSize = sysconf(_SC_PAGESIZE);

    if (pageSize <= 0)
    {
        std::cerr << "[Camera] Failed to get page size." << std::endl;

        return false;
    }

    /*
     * mmap要求offset通常是page-aligned。
     *
     * 因此需要将真正的offset向下对齐。
     * 摄像头数据存在于DMA-BUF，但是openCv Mat需要访问CPU可访问的内存，
     * 因此需要使用mmap将DMA-BUF映射到用户空间。
     */
    // plane.offset 向下对齐到 page boundary。
    const std::size_t alignedOffset = plane.offset - (plane.offset % static_cast<unsigned int>(pageSize));

    const std::size_t offsetInMapping = plane.offset - alignedOffset;

    const std::size_t mapLength = static_cast<std::size_t>(plane.length) + offsetInMapping;

    /**
     *      参数	        含义
        nullptr	        让 Linux 自动选择虚拟地址
        mapLength	    要映射多少字节
        PROT_READ	    CPU 可以读取
        PROT_WRITE	    CPU 可以写入
        MAP_SHARED	    映射与底层对象共享
        plane.fd.get()	DMA-BUF fd
        alignedOffset	映射起始位置
     */
    void *mapped = mmap(
            nullptr,
            mapLength,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            plane.fd.get(),
            alignedOffset);

    if (mapped == MAP_FAILED)
    {
        std::cerr << "[Camera] mmap failed: " << std::strerror(errno) << std::endl;

        return false;
    }

    auto *data = static_cast<std::uint8_t*>(mapped);

    data += offsetInMapping;    //真正指向 Plane 数据的起始位置

    /*
     * OpenCV Mat使用自己的内存，
     * 然后逐行从DMA buffer复制过去。
     *
     * 不能直接长期让Mat引用摄像头buffer，
     * 因为camera停止/下一次request以后
     * buffer会被复用。
     */
    // OpenCV 自己管理的一块新的内存
    cv::Mat result(static_cast<int>(height_), static_cast<int>(width_), CV_8UC3);

    const std::size_t bytesPerPixel = 3;  // RGB888 每个像素占用 3 个字节

    // rowBytes：一行真正的有效图像数据有多少字节
    const std::size_t rowBytes = static_cast<std::size_t>(width_) * bytesPerPixel;

    if (stride_ < rowBytes ||
        plane.length < (static_cast<std::size_t>(height_) - 1) * stride_ + rowBytes)
    {
        std::cerr << "[Camera] Invalid frame layout: stride=" << stride_
                  << ", plane length=" << plane.length << std::endl;

        munmap(mapped, mapLength);
        return false;
    }

    // stride_:从这一行起始地址到下一行起始地址之间有多少字节
    for (unsigned int y = 0; y < height_; ++y)
    {
        const auto *src = data + static_cast<std::size_t>(y) * stride_;

        auto *dst = result.ptr<std::uint8_t>(static_cast<int>(y));

        std::memcpy(dst, src, rowBytes);
    }

    // 解除映射
    munmap(mapped, mapLength);

    /**
     * cv::Mat 本身是一个带引用计数的图像对象，std::move(result)表示把 result 
     * 的资源交给 image，尽量避免不必要的数据复制。
     * 不是把图像数据再复制一遍。它是把 result 转换为右值，从而允许移动赋值。
     */
    // OpenCV 和 cv::imwrite 使用 BGR；只有 RGB888 需要交换通道。
    // if (isRgb)
    // {
    //     cv::cvtColor(result, image, cv::COLOR_RGB2BGR);
    // }
    // else
    // {
    //     image = std::move(result);
    // }
    image = std::move(result);
    return true;
}

void* CameraManager::mapPlane(const libcamera::FrameBuffer::Plane&, size_t&, size_t&)
{
    /*
     * 当前版本不需要通过这个接口单独暴露映射结果。
     *
     * 真正映射工作在frameBufferToMat()中完成。
     */
    return nullptr;
}

void CameraManager::unmapPlane(void*, size_t)
{
    /*
     * 当前版本不使用。
     */
}

bool CameraManager::isInitialized() const
{
    return initialized_;
}

// void CameraManager::release()
// {
//     if (!initialized_ && !cameraManager_)
//     {
//         return;
//     }

//     std::cout << "[Camera] Releasing camera..." << std::endl;

//     /*
//      * 停止正在运行的摄像头。
//      */
//     if (camera_)
//     {
//         camera_->stop();
//     }

//     /*
//      * request必须在camera停止之后销毁。
//      */
//     request_.reset();

//     /*
//      * allocator负责释放FrameBuffer。
//      */
//     allocator_.reset();

//     configuration_.reset();

//     if (camera_)
//     {
//         camera_->release();
//         camera_.reset();
//     }

//     if (cameraManager_)
//     {
//         cameraManager_->stop();
//         cameraManager_.reset();
//     }

//     stream_ = nullptr;

//     initialized_ = false;

//     std::cout << "[Camera] Camera released." << std::endl;
// }

bool CameraManager::startPreview()
{
    if(!initialized_)
    {
    DEBUG_ERROR("Camera", "Cannot start preview before initialize()");
        std::cerr << "[Camera] Camera has not been initialized." << std::endl;

        return false;
    }

    if(!camera_ || !stream_ || !allocator_)
    {
        DEBUG_ERROR("Camera", "Preview resources are invalid");
        std::cerr<< "[Camera] Camera resources are invalid." << std::endl;

        return false;
    }

    if(previewRunning_.load())
    {
        std::cout << "[Camera] Preview is already running." << std::endl;

        return true;
    }

    /*
     * 获取初始化阶段已经分配好的 FrameBuffer。
     */
    const auto &buffers = allocator_->buffers(stream_);

    if(buffers.empty())
    {
        DEBUG_ERROR("Camera", "No frame buffers available for preview");
        std::cerr<< "[Camera] No frame buffers available." << std::endl;

        return false;   
    }

    std::cout << "[Camera] Preparing " << buffers.size() << " requests..." << std::endl;

    /**
     * 清理旧Request
     */

    requests_.clear();

    /*
     * 为每一个 FrameBuffer 创建一个 Request。
     */
    for(const auto &buffer : buffers)
    {
        std::unique_ptr<libcamera::Request> request = camera_->createRequest();

        if(!request)
        {
            DEBUG_ERROR("Camera", "Failed to create preview request");
            std::cerr << "[Camera] Failed to create request." << std::endl;

            requests_.clear();
            return false;
        }

        libcamera::FrameBuffer *frameBufer = buffer.get();

        /*
         * 将 Stream 和 FrameBuffer 绑定到 Request。
         */
        int ret = request->addBuffer(stream_, frameBufer);

        if(ret < 0)
        {
            DEBUG_ERROR("Camera", "Failed to bind buffer to preview request, ret=" << ret);
            std::cerr << "[Camera] Failed to add buffer to request, ret = " << ret << std::endl;

            requests_.clear();

            return false;
        }

        requests_.push_back(std::move(request));
    }

    /*
     * 清理上一轮图像。
     */
    {
        std::lock_guard<std::mutex> lock(mutex_);

        latestFrame_.release();

        warmupFramesRemaining_ = 5;
    }

    /*
     * 设置预览状态。
     *
     * 必须在 queueRequest() 之前设置。
     */
    previewRunning_.store(true);


    /*
     * 启动 Camera。
     */
    int ret = camera_->start();
    if (ret < 0)
    {
        DEBUG_ERROR("Camera", "Failed to start camera preview, ret=" << ret);
        std::cerr<< "[Camera] Failed to start camera, ret = " << ret << std::endl;

        previewRunning_.store(false);

        requests_.clear();

        return false;
    }

    /*
     * 把所有 Request 放入 Camera 队列。
     */
    for(const auto &request : requests_)
    {
        ret = camera_->queueRequest(request.get());
        if (ret < 0)
        {
            DEBUG_ERROR("Camera", "Failed to queue preview request, ret=" << ret);
            std::cerr << "[Camera] Failed to queue request, ret = " << ret << std::endl;

            previewRunning_.store(false);

            camera_->stop();

            requests_.clear();

            return false;
        }
    }
    std::cout << "[Camera] Preview started." << std::endl;

    return true;
}


void CameraManager::requestComplete(libcamera::Request *request)
{
    if (!request)
    {
        DEBUG_ERROR("Camera", "requestComplete received a null request");
        return;
    }


    /*
     * 如果预览已经停止，
     * 不再处理新的 Frame。
     */
    if (!previewRunning_.load())
    {
        return;
    }


    /*
     * Request 可能是：
     *
     * RequestComplete
     * RequestCancelled
     *
     * 如果不是正常完成，就不处理图像。
     */
    if (request->status() != libcamera::Request::RequestComplete)
    {
        DEBUG_WARN("Camera", "A camera request was cancelled or incomplete");
        std::cerr << "[Camera] Request was not completed normally." << std::endl;

        return;
    }


    /*
     * 获取这个 Request 对应的 FrameBuffer。
     */
    const auto &buffers = request->buffers();

    if (buffers.empty())
    {
        DEBUG_ERROR("Camera", "Completed request contains no buffer");
        std::cerr << "[Camera] Completed request contains no buffer." << std::endl;

        return;
    }


    /*
     * 当前项目只有一个 Stream。
     *
     * 因此取第一个 Stream 对应的 FrameBuffer。
     */
    libcamera::FrameBuffer *buffer = buffers.begin()->second;


    /*
     * =========================================================
     * Warm-up
     * =========================================================
     */

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (warmupFramesRemaining_ > 0)
        {
            --warmupFramesRemaining_;

            /*
             * 这一帧直接丢弃。
             *
             * 不保存到 latestFrame_。
             */
        }
        else
        {
            /*
             * 正式处理这一帧。
             *
             * 注意：
             * frameBufferToMat() 会复制数据到独立的 cv::Mat。
             */
            cv::Mat image;

            const bool success = frameBufferToMat(buffer, image);

            if (success)
            {
                /*
                 * image 已经是独立内存。
                 *
                 * FrameBuffer 后续可以继续复用。
                 */
                latestFrame_ = std::move(image);
            }
            else
            {
                DEBUG_ERROR("Camera", "Failed to convert frame buffer to cv::Mat");
                std::cerr << "[Camera] Failed to convert FrameBuffer to Mat." << std::endl;
            }
        }
    }


    /*
     * =========================================================
     * Request 重新使用
     * =========================================================
     *
     * Request 完成以后不能直接再次 queue。
     *
     * 必须先 reuse。
     */

    request->reuse(libcamera::Request::ReuseBuffers);


    /*
     * 如果用户已经点击停止预览，
     * 就不要重新排队。
     */
    if (!previewRunning_.load())
    {
        return;
    }


    /*
     * 再次进入 Camera 队列。
     *
     * 这就是实时预览能够一直运行的关键。
     */
    int ret = camera_->queueRequest(request);

    if (ret < 0)
    {
        DEBUG_ERROR("Camera", "Failed to requeue preview request, ret=" << ret);
        std::cerr << "[Camera] Failed to requeue request, ret = " << ret << std::endl;

        previewRunning_.store(false);
    }
}


bool CameraManager::getLatestFrame(cv::Mat &image)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!previewRunning_.load())
    {
        DEBUG_WARN("Camera", "captureImage called while preview is stopped");
        return false;
    }

    if (latestFrame_.empty())
    {
        return false;
    }


    /*
     * 必须 clone。
     *
     * 因为函数返回后 mutex 会释放，
     * requestComplete() 可能马上更新 latestFrame_。
     *
     * 所以 Qt 得到一份独立的数据。
     */
    image = latestFrame_.clone();

    return true;
}


bool CameraManager::captureImage(const std::string &filePath)
{
    if (!initialized_)
    {
        std::cerr << "[Camera] Camera has not been initialized." << std::endl;

        return false;
    }


    if (!previewRunning_.load())
    {
        std::cerr << "[Camera] Preview is not running." << std::endl;

        return false;
    }


    cv::Mat image;


    /*
     * 先取得当前最新画面的副本。
     */
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (latestFrame_.empty())
        {
            DEBUG_WARN("Camera", "captureImage called before the first valid frame arrived");
            std::cerr << "[Camera] No frame available." << std::endl;

            return false;
        }

        image = latestFrame_.clone();
    }


    /*
     * mutex 已经释放。
     *
     * imwrite() 不会阻塞 Camera callback。
     */
    if (!cv::imwrite(filePath, image))
    {
        DEBUG_ERROR("Camera", "Failed to save image: " << filePath);
        std::cerr << "[Camera] Failed to save image: " << filePath << std::endl;

        return false;
    }


    std::cout << "[Camera] Image saved successfully: " << filePath << std::endl;

    return true;
}



void CameraManager::stopPreview()
{
    if (!previewRunning_.load())
    {
        return;
    }


    /*
     * 先告诉 callback：
     *
     * 不要再 queueRequest()
     */
    previewRunning_.store(false);


    std::cout << "[Camera] Stopping preview..." << std::endl;


    /*
     * 停止 Camera。
     */
    if (camera_)
    {
        camera_->stop();
    }


    /*
     * Camera stop 之后，
     * 再释放 Request。
     */
    requests_.clear();


    {
        std::lock_guard<std::mutex> lock(mutex_);

        latestFrame_.release();
    }


    std::cout << "[Camera] Preview stopped." << std::endl;
}


bool CameraManager::isPreviewRunning() const
{
    return previewRunning_.load();
}

void CameraManager::shutdown()
{
    stopPreview();

    release();
}


void CameraManager::release()
{
    std::cout << "[Camera] Releasing camera..." << std::endl;


    /*
     * 如果预览还在运行，先停止。
     */
    if (previewRunning_.load())
    {
        stopPreview();
    }


    /*
     * Request 必须在 Camera 停止以后释放。
     */
    requests_.clear();


    /*
     * 释放 FrameBuffer。
     */
    allocator_.reset();


    /*
     * 释放配置。
     */
    configuration_.reset();


    /*
     * 释放 Camera。
     */
    if (camera_)
    {
        camera_->release();
        camera_.reset();
    }


    /*
     * 停止 CameraManager。
     */
    if (cameraManager_)
    {
        cameraManager_->stop();
        cameraManager_.reset();
    }


    stream_ = nullptr;


    {
        std::lock_guard<std::mutex> lock(mutex_);

        latestFrame_.release();
    }


    initialized_ = false;
    previewRunning_.store(false);


    std::cout << "[Camera] Camera released." << std::endl;
}

}