#ifndef LABEL_TOOLS_VIDEO_COMMON_H
#define LABEL_TOOLS_VIDEO_COMMON_H

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

/*
struct LabeledObject {
	std::string label;
	int id = -1;
	cv::Point2f center;
	cv::Point2f size;
	cv::Size imageSize;
	cv::Rect displayRect;
	
	LabeledObject(std::string _label, int _id, cv::Point _center, int _w, int _h, cv::Size _imageSize) {
		label = _label;
		id = _id;
		center = cv::Point2f(
			(float)_center.x / (float)_imageSize.width, 
			(float)_center.y / (float)_imageSize.height
		);
		size = cv::Point2f(
			(float)_w / (float)_imageSize.width,
			(float)_h / (float)_imageSize.height
		);
		displayRect = cv::Rect(
			_center.x - int(_w * 0.5),
			_center.y - int(_h * 0.5),
			_w,
			_h
		);
		imageSize = _imageSize;
	}
	
	cv::Rect rectangle(cv::Size _imageSize) {
		if (id == -1) {
			return cv::Rect();
		}
		
		if (imageSize.width == _imageSize.width 
			&& imageSize.height == _imageSize.height) {
			return displayRect;
		} else {
			return cv::Rect(
				int((center.x - (size.width * 0.5)) * _imageSize.width),
				int((center.y - (size.height * 0.5)) * _imageSize.height),
				int(size.width * _imageSize.width),
				int(size.height * _imageSize.height)
			);
		}
	}
};
*/
struct MyBoxInfo {
	std::string id;
	cv::Rect rect;
	cv::Point textPt;
	cv::Scalar color;
};

typedef std::map<std::string, MyBoxInfo> MyPanelBox;

#endif
