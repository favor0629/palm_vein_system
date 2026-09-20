#ifndef __IMAGEPROCESSOR_H
#define __IMAGEPROCESSOR_H

#include <opencv2/opencv.hpp>

#include <array>
#include <string>
#include <vector>

class ImageProcessor
{
public:
    ImageProcessor() = default;
    ~ImageProcessor() = default;

    // 加载图片
    bool load(const std::string& filepath);

    // 保存图片
    bool save(const std::string &filepath, const cv::Mat &image) const;

    // 将图片转为单通道灰度图
    cv::Mat toGray(const cv::Mat &image) const;

    // 根据opencv矩形区域裁剪ROI
    cv::Mat cropROI(const cv::Mat &image, const cv::Rect &roi) const;


    // 按比例将图片划分成四个矩形
    //
    // xRatio：X方向分割比例
    // yRatio：Y方向分割比例
    //
    // 例如：
    // xRatio = 0.5
    // yRatio = 0.5
    //
    // 表示分别在 X、Y 方向的 50% 位置进行分割。
    std::array<cv::Mat, 4> split4(const cv::Mat& image, double xRatio, double yRatio) const;

    // 获取当天加载的图片
    const cv::Mat& image() const;

    // 判断是否已经加载图片
    bool isLoaded() const;

private:
    cv::Mat image_;

};



#endif