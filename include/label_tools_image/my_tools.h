#ifndef LABEL_TOOLS_VIDEO_MY_TOOLS_H
#define LABEL_TOOLS_VIDEO_MY_TOOLS_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <boost/thread.hpp>

#include "common.h"
#include "my_image.h"

static void onMouse(int event, int x, int y, int flags, void *param) ;

class MyTools {
public:
	MyTools(std::string main_winname);
	~MyTools();
	
	void setClassNames(MyLabel classnames);
	
	void mouseClick(cv::Point pt);
	void mouseMove(cv::Point pt);
	void mouseRelease();
	
	void loop(std::vector<MyImage> &images);
	void printHelp();
private:
	
  int editImage(cv::Mat &image, MyImage &objects, cv::Rect panelImgInGlobal, int totalImageNum);
	void renderAnnotationBox(cv::Mat &image, MyPanelBox &panelBox, double scale);
	void initAnnotationBoxes(MyPanelBox &annotationBoxes, int width, int height);
	void setPolygon(MyPolygon &polygon, cv::Rect rect);
	void drawPolygon(cv::Mat &image, MyPolygon polygon);
	cv::Rect getBoundingBox(MyPolygon polygon, int currentPtIndex);
	LabeledObject getLabeledObject(std::string label, int id, MyPolygon polygon, cv::Size imageSize);
	void checkPolygonBorderCollision(cv::Rect &rect, cv::Size imageSize, cv::Point &mousePt);
	
	std::string main_winname_;
	
	MyLabel classnames_;
	
	enum Mode{FREE_RECT, FIXED_RECT};
	
	int current_polygon_mode_;
	MyPolygon current_polygon_;
	cv::Point prev_clicked_pt_;
	int click_polygon_status_;
	
	std::map<std::string, cv::Scalar> color_legend_;
	MyPanelBox currentPanelBox_;
	std::string selectedLabel_;
	
	cv::Rect sourceImageRect_;	
	bool quit_all_, show_object_id_;
	cv::Size currentImageSize_;
};

#endif
