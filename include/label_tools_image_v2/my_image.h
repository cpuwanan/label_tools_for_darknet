#ifndef LABEL_TOOLS_VIDEO_MY_IMAGE_H
#define LABEL_TOOLS_VIDEO_MY_IMAGE_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>

#include "common.h"

class LabeledObject {
public:
	LabeledObject(std::string name, int class_id, double x, double y, double w, double h);
	~LabeledObject();
	cv::Rect rectangle(int width, int height);
	std::string name() { return classname_; }
	std::string getInfo();
	MyPolygon polygon(int width, int height);
private:
	int class_id_ = -1;
	std::string classname_;
	double x_, y_, w_, h_; // scaled from 0 ~ 1
};

class MyImage {
public:
	MyImage();
	~MyImage();
	std::string name() { return name_; }
	int imageIndex() { return image_index_; }
	void init(std::string id, int image_index);
	void addObject(LabeledObject object, std::string id);
	void deleteObjectById(std::string id);
	void deleteLastObject();
	void draw(cv::Mat &image, MyLabel classnames, bool showId, int selectedIndex);
	void clearAllObjects();
	bool hasObjects() { return (objects_.size() > 0); }
	std::map<std::string, LabeledObject> getObjects() { return objects_; }
private:
	void drawTextWithBackground(cv::Mat &image, int top, int left, std::string text, int fontFace, double fontScale, int fontThickness, cv::Scalar textColor, cv::Scalar bgColor);
	
	int image_index_;
	std::string name_ = "";
	std::map<std::string, LabeledObject> objects_;
};

#endif
