#ifndef LABEL_TOOLS_IMAGE_V2_COMMON_H
#define LABEL_TOOLS_IMAGE_V2_COMMON_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>

struct MyLabelInfo {
	int id;
	cv::Scalar color;
};

typedef std::map<std::string, MyLabelInfo> MyLabel;
typedef std::vector<cv::Point> MyPolygon;

struct MyBoxInfo {
	std::string id;
	cv::Rect rect;
	cv::Point textPt;
	cv::Scalar color;
};

typedef std::map<std::string, MyBoxInfo> MyPanelBox;

struct ImageInfo {
	int id;
	std::string name;
	cv::Mat image;
};

#endif
