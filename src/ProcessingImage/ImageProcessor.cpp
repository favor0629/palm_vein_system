#include "ImageProcessor.hpp"

#include <iostream>
#include <stdexcept>

#include "../../../test/debug.hpp"

/**
 * 默认在划分roi的时候，采用50%， 50%的格式
 */
constexpr double X_RATIO = 0.5;
constexpr double Y_RATIO = 0.5;

bool ImageProcessor::load(const std::string &filepath)
{
    image_ = cv::imread(filepath, cv::IMREAD_UNCHANGED);

    if(image_.empty())
    {
        DEBUG_ERROR("ImageProcessor", "Failed to load image: " << filepath);
        std::cerr << "Failed to load image:" << filepath << std::endl;
        return false;
    }

    return true;
}
bool ImageProcessor::save(const std::string &filepath, const cv::Mat &image) const
{
    if(image.empty())
    {
        DEBUG_WARN("ImageProcessor", "save() received an empty image");
        std::cerr << "Cannot save an empty image." << std::endl;
        return false;
    }

    try
    {
        return cv::imwrite(filepath, image);
    }
    catch(const cv::Exception &e)
    {
        DEBUG_ERROR("ImageProcessor", "OpenCV failed to save image: " << e.what());
        std::cerr << "OpenCV exception while saving image: " << e.what() << std::endl;

        return false;
    }
}
cv::Mat ImageProcessor::toGray(const cv::Mat &image) const
{
    if(image.empty())
    {
        DEBUG_WARN("ImageProcessor", "toGray() received an empty image");
        return {};
    }

    // 已经是单通道
    if(image.channels() == 1)
    {
        return image.clone();
    }

    cv::Mat gray;
    
    /**
     * 进行不同通道的转换
     */
    if(image.channels() == 3)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }
    else if(image.channels() == 4)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    }
    else
    {
        DEBUG_ERROR("ImageProcessor", "Unsupported channel count: " << image.channels());
        throw std::invalid_argument("Unsupported image channel count.");
    }

    return gray;
}

cv::Mat ImageProcessor::cropROI(const cv::Mat &image, const cv::Rect &roi) const
{
    if(image.empty())
    {
        DEBUG_WARN("ImageProcessor", "cropROI() received an empty image");
        return {};
    }

    // 检查roi是否在图片范围内
    cv::Rect iamge_rect(0, 0, image.cols, image.rows);

    if((iamge_rect & roi) != roi)
    {
        DEBUG_ERROR("ImageProcessor", "ROI is outside image: x=" << roi.x
            << ", y=" << roi.y << ", width=" << roi.width
            << ", height=" << roi.height);
        throw std::out_of_range("ROI is outside the image boundary");
    }

    // image(roi) 本质上只是一个 Mat header，
    // 和原图共享数据。
    //
    // clone() 后才得到真正独立的图像数据。
    return image(roi).clone();
}

std::array<cv::Mat, 4> ImageProcessor::split4(const cv::Mat &image, 
                                              double x_ratio = X_RATIO, 
                                              double y_ratio = Y_RATIO) const
{
    if(image.empty())
    {
        DEBUG_WARN("ImageProcessor", "split4() received an empty image");
        return {};
    }

    if(x_ratio <= 0.0 || x_ratio >= 1.0)
    {
        DEBUG_ERROR("ImageProcessor", "Invalid x ratio: " << x_ratio);
        throw std::invalid_argument("x ratio must be between 0 and 1.");
    }

    if(y_ratio <= 0.0 || y_ratio >= 1.0)
    {
        DEBUG_ERROR("ImageProcessor", "Invalid y ratio: " << y_ratio);
        throw std::invalid_argument("y ratio must be between 0 and 1.");
    }

    // 开始划分
    const int split_x = static_cast<int>(image.cols * x_ratio);
    const int split_y = static_cast<int>(image.rows * y_ratio);

    const cv::Rect top_left(0, 0, split_x, split_y);
    const cv::Rect top_right(split_x, 0, image.cols - split_x, split_y);
    const cv::Rect bottom_left(0, split_y, split_x, image.rows -split_y);
    const cv::Rect botton_right(split_x, split_y, image.cols -split_x, image.rows - split_y);

    return 
    {
        image(top_left).clone(),
        image(top_right).clone(),
        image(botton_right).clone(),
        image(bottom_left).clone(),
    };
}


const cv::Mat& ImageProcessor::image() const
{
    return image_;
}


bool ImageProcessor::isLoaded() const
{
    return !image_.empty();
}