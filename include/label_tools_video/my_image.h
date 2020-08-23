#ifndef LABEL_TOOLS_VIDEO_MY_IMAGE_H
#define LABEL_TOOLS_VIDEO_MY_IMAGE_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>

#include "common.h"

class LabeledObject {
public:
	LabeledObject(std::string label, int class_id, cv::Point center, int w, int h, cv::Size imageSize);
	~LabeledObject();
	cv::Rect rectangle(cv::Size imageSize);
	cv::Rect rectangle();
	std::string getInfo();
	std::string getLabel();
	std::string getLabelInfoStr();
private:
	int class_id_ = -1;
	std::string label_;
	cv::Point2f center_;
	cv::Point2f size_;
	cv::Size imageSize_;
	cv::Rect displayRect_;
};

class MyImage {
public:
	MyImage();
	~MyImage();
	void init(cv::Mat image, std::string id);
	void addObject(LabeledObject object, std::string id);
	void deleteObjectById(std::string id);
	void deleteLastObject();
	void draw(cv::Mat &image, MyLabel classnames, bool showId = false);
	std::string getImageId() {
		return frame_id_;
	}
	std::string getInfo();
	bool isOk() {
		return (frame_id_ != "") && (!image_.empty());
	}
	bool hasObjects() {
		return (objects_.size() > 0);
	}
	cv::Size getImageSize() {
		return image_.size();
	}
	cv::Mat getImage() {
		return image_;
	}
	std::map<std::string, LabeledObject> getObjects() {
		return objects_;
	}
private:
	void drawTextWithBackground(cv::Mat &image, int top, int left, std::string text, int fontFace, double fontScale, int fontThickness, cv::Scalar textColor, cv::Scalar bgColor);
	
	std::string frame_id_ = "";
	cv::Mat image_;
	std::map<std::string, LabeledObject> objects_;
};

#endif
