#include "fvia.hpp"

#include <opencv2/opencv.hpp>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "../../test/debug.hpp"

constexpr unsigned int GRAY_NUMBER = 256;
/**
 * @brief 计算图像熵
 *
 * E = -sum(p(k) * log2(p(k)))
 *
 * 输入必须为 CV_8UC1 灰度图像。
 */
double calculateEntropy(const cv::Mat &gray)
{
    if(gray.empty())
    {
        DEBUG_ERROR("FVIA", "Entropy input is empty");
        throw std::invalid_argument("Input image is empty");
    }

    // 判断图片是否为CV_8UC1 灰度图像
    if(gray.type() != CV_8UC1)
    {
        DEBUG_ERROR("FVIA", "Entropy input must be CV_8UC1, actual type=" << gray.type());
        throw std::invalid_argument("Entropy calculation requires CV_8UC1 image.");
    }

    // 统计256个灰度级
    int histogram[GRAY_NUMBER] = {0};

    const int rows = gray.rows;
    const int cols = gray.cols;
    const int total_pixels = rows * cols;

    for(int y = 0; y < rows; ++y)
    {
        const uchar *row = gray.ptr<uchar>(y);

        for(int x = 0; x < cols; ++x)
        {
            ++histogram[row[x]];
        }
    }

    double entropy = 0.0;

    for(int k = 0; k < GRAY_NUMBER; ++k)
    {
        if(histogram[k] == 0)
        {
            continue;
        }

        const double p = static_cast<double>(histogram[k]) / static_cast<double>(total_pixels);

        entropy -= p * std::log2(p);
    }

    return entropy;
}


/**
 * @brief 计算平均灰度
 *
 * G = 1/N * sum(x_i)
 *
 * 
 */
double calculateMean(const cv::Mat &gray)
{
    if (gray.empty())
    {
        DEBUG_ERROR("FVIA", "Mean input is empty");
        throw std::invalid_argument("Input image is empty.");
    }

    if (gray.type() != CV_8UC1)
    {
        DEBUG_ERROR("FVIA", "Mean input must be CV_8UC1, actual type=" << gray.type());
        throw std::invalid_argument("Mean/variance calculation requires CV_8UC1 image.");
    }

    const int rows = gray.rows;
    const int cols = gray.cols;
    const uint64_t total_pixels = static_cast<uint64_t>(rows) * static_cast<uint64_t>(cols);

    uint64_t sum = 0;

    for(int y = 0; y < rows; ++y)
    {
        const uchar *row = gray.ptr<uchar>(y);

        for(int x = 0; x < cols; ++x)
        {
            const uint64_t value = row[x];

            sum += value;
        }
    }


    double mean_gray = static_cast<double>(sum) / static_cast<double>(total_pixels);
    
    return mean_gray;
}

/**
 * @brief 计算平均灰度和灰度方差
 *
 * G = 1/N * sum(x_i)
 *
 * V = 1/N * sum(x_i^2) - G^2
 */
void calculateMeanAndVariance(const cv::Mat &gray, double &mean_gray, double &variance)
{
    if (gray.empty())
    {
        DEBUG_ERROR("FVIA", "Mean/variance input is empty");
        throw std::invalid_argument("Input image is empty.");
    }

    if (gray.type() != CV_8UC1)
    {
        DEBUG_ERROR("FVIA", "Mean/variance input must be CV_8UC1, actual type=" << gray.type());
        throw std::invalid_argument("Mean/variance calculation requires CV_8UC1 image.");
    }

    const int rows = gray.rows;
    const int cols = gray.cols;
    const uint64_t total_pixels = static_cast<uint64_t>(rows) * static_cast<uint64_t>(cols);

    uint64_t sum = 0;
    uint64_t sum_squares = 0;

    for(int y = 0; y < rows; ++y)
    {
        const uchar *row = gray.ptr<uchar>(y);

        for(int x = 0; x < cols; ++x)
        {
            const uint64_t value = row[x];

            sum += value;

            sum_squares += value * value;
        }
    }


    mean_gray = static_cast<double>(sum) / static_cast<double>(total_pixels);
    
    const double mean_square = static_cast<double>(sum_squares) / static_cast<double>(total_pixels);

    variance = mean_square - mean_gray * mean_gray;

    // 避免由于浮点误差出现极小负数
    if(variance < 0.0)
    {
        variance = 0.0;
    }
}

/**
 * @brief FVIA 中的灰度归一化函数 F_N(G, T)
 *
 * 论文公式：
 *
 * F_N(G,T) =
 *
 *   -pi/2 * ((T-G)/T)^2 - pi/6, G <= T
 *
 *    pi/2 * ((G-T)/(256-T))^2 + pi/6, G > T
 */
double calculateFN(const double mean_gray,const double T = T_VAR)
{
    if(T < 0.0 || T >= 256.0)
    {
        DEBUG_ERROR("FVIA", "Invalid T value: " << T);
        throw std::invalid_argument("T must satisfy 0 < T < 256");
    }

    double fn = 0.0;

    if(mean_gray <= T)
    {
        const double normalized = (T - mean_gray) / T;

        fn = -M_PI / 2.0 * normalized * normalized - M_PI / 6.0;
    }
    else
    {
        const double normalized = (mean_gray - T) / (256.0 - T);

        fn = M_PI / 2.0 * normalized * normalized + M_PI / 6.0;
    }

    return fn;
}


/**
 * @brief 
 *
 * Q =
 *      exp(E) + V
 * ----------------------------
 * log(|tanh(FN)| + 1)
 */
double calculateFVIAQuality(const double entropy,
                            const double variance,
                            const double mean_gray,
                            const double T = T_VAR)
{
    const double fn = calculateFN(mean_gray, T);

    const double denominator = std::log(std::fabs(std::tanh(fn)) + 1.0);


    // 理论上论文通过 pi/6 避免 denominator 为 0，
    // 这里仍然做保护。
    if(denominator <= 1e-12)
    {
        DEBUG_ERROR("FVIA", "Quality denominator is too small");
        throw std::runtime_error("FVIA denominator is too small");
    }

    const double numerator = std::exp(entropy) + variance;

    return numerator / denominator;
}



/**
 * @brief 对单幅掌静脉图像执行 FVIA 质量评价
 *
 * 注意：
 * Q_th、G_min、G_max 是你的掌静脉系统需要
 * 通过实验开发集自行确定的参数。
 */
FVIAResult_t evaluateFVIA(const cv::Mat& input_gray, const cv::Rect& roi, const double T = T_VAR)
{
    if(input_gray.empty())
    {
        DEBUG_ERROR("FVIA", "Evaluation input is empty");
        throw std::invalid_argument("Input image is empty.");
    }

    if (input_gray.type() != CV_8UC1)
    {
        DEBUG_ERROR("FVIA", "Evaluation input must be CV_8UC1, actual type=" << input_gray.type());
        throw std::invalid_argument("Input image must be CV_8UC1.");
    }

    // 检查 ROI
    if (roi.x < 0 || roi.y < 0 ||
        roi.x + roi.width > input_gray.cols ||
        roi.y + roi.height > input_gray.rows ||
        roi.width <= 0 || roi.height <= 0)
    {
        DEBUG_ERROR("FVIA", "Evaluation ROI is outside image boundaries");
        throw std::invalid_argument("ROI is outside image boundaries.");
    }

    // 提取 ROI
    cv::Mat roi_image = input_gray(roi);

    FVIAResult_t result;

    // 计算 E、G、V
    result.entropy = calculateEntropy(roi_image);

    calculateMeanAndVariance(roi_image, result.mean_gray, result.variance);

    // 计算 FVIA 综合质量分数
    result.quality = calculateFVIAQuality(result.entropy, result.variance, result.mean_gray, T);

    /**
     * 暂时不进行gray_range_ok;quality_ok;qualified 的判断
     */
    result.gray_range_ok = true;
    result.qualified = true;
    result.quality_ok = true;
    // // 单幅图像的曝光范围判断
    // result.grayRangeOK =
    //     (result.meanGray >= G_min &&
    //      result.meanGray <= G_max);

    // // FVIA 综合质量判断
    // result.qualityOK =
    //     (result.quality >= Q_th);

    // // 最终判定
    // result.qualified =
    //     result.grayRangeOK &&
    //     result.qualityOK;

    return result;
}