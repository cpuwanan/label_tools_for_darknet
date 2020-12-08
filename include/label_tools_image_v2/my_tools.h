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
	MyTools(std::string main_winname, std::string setting_winname);
	~MyTools();
	
	void setImageFilenames(std::vector<std::string> files, std::string input_dir, std::string output_dir);
	void setClassNames(MyLabel classnames);
	
	void mouseClick(cv::Point pt);
	void mouseMove(cv::Point pt);
	void mouseRelease();
	
	void run();
	void printHelp();
private:

	void saveToFile(std::map<std::string, MyImage> data);
	
	int editImage(ImageInfo input, MyImage &drawing, cv::Rect panel_rect);
	void renderAnnotationBox(cv::Mat &image, MyPanelBox &panelBox, double scale, std::string image_label);
	void initAnnotationBoxes(MyPanelBox &annotationBoxes, int width, int height);
	void setPolygon(MyPolygon &polygon, cv::Rect rect);
	void drawPolygon(cv::Mat &image, MyPolygon polygon);
	cv::Rect getBoundingBox(MyPolygon polygon, int currentPtIndex);
	void checkPolygonBorderCollision(cv::Rect &rect, cv::Size imageSize, cv::Point &mousePt);
	
	std::string main_winname_, setting_winname_;
	
	std::vector<ImageInfo> image_list_;
	MyLabel classnames_;
	
	enum Mode{FREE_RECT, FIXED_RECT};
	
	int current_polygon_mode_;
	MyPolygon current_polygon_;
	cv::Point prev_clicked_pt_;
	int click_polygon_status_;
	int display_scale_;
	
	cv::Mat info_image_;
	std::map<std::string, cv::Scalar> color_legend_;
	MyPanelBox currentPanelBox_;
	std::string selectedLabel_;
	
	cv::Rect sourceImageRect_;
	std::map<std::string, MyImage> allLabeledImages_;
	
	bool quit_all_, show_object_id_;
	
	cv::Rect panelRectInGlobal_;
	
	MyImage current_drawing_;
	std::string input_dir_, output_dir_;
};

#endif
