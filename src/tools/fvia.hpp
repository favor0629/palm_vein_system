#pragma once

#include <opencv2/opencv.hpp>


constexpr double T_VAR = 120;        // 这个参数需要后续调整

struct FVIAResult_t
{
    double entropy = 0.0;       // E
    double variance = 0.0;      // V
    double mean_gray = 0.0;     // G_var
    double quality = 0.0;       // Q_FVIA

    bool gray_range_ok = false;
    bool quality_ok = false;
    bool qualified = false;
};

FVIAResult_t evaluateFVIA(const cv::Mat &input_gray, const cv::Rect &roi, const double T);

double calculateMean(const cv::Mat &gray);

