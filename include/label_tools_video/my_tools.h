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
	
	void setVideoFiles(std::map<std::string, std::string> files);
	void setClassNames(MyLabel classnames);
	void setSavedImageType(std::string subfix);
	void setLabeledImagesDir(std::string path);
	
	void mouseClick(cv::Point pt);
	void mouseMove(cv::Point pt);
	void mouseRelease();
	
	void loop();
	void saveLabeledImages(std::map<std::string, std::vector<MyImage>> data);
	void printHelp();
private:
	
	int readVideo(cv::VideoCapture &cap, std::vector<MyImage> &allObjects, cv::Rect panelImgInGlobal, std::string video_id);
	void renderAnnotationBox(cv::Mat &image, MyPanelBox &panelBox, double scale);
	void initAnnotationBoxes(MyPanelBox &annotationBoxes, int width, int height);
	void setPolygon(MyPolygon &polygon, cv::Rect rect);
	void drawPolygon(cv::Mat &image, MyPolygon polygon);
	cv::Rect getBoundingBox(MyPolygon polygon, int currentPtIndex);
	LabeledObject getLabeledObject(std::string label, int id, MyPolygon polygon, cv::Size imageSize);
	void updateObjectsForVideo(std::vector<MyImage> &array, MyImage data);
	void checkPolygonBorderCollision(cv::Rect &rect, cv::Size imageSize, cv::Point &mousePt);
	
	std::string main_winname_, setting_winname_;
	std::string image_subfix_;
	
	std::map<std::string, std::string> video_files_;
	MyLabel classnames_;
	
	enum Mode{FREE_RECT, FIXED_RECT};
	
	int current_polygon_mode_;
	MyPolygon current_polygon_;
	cv::Point prev_clicked_pt_;
	int click_polygon_status_;
	int display_scale_;
	int frame_no_;
	
	cv::Mat info_image_;
	std::map<std::string, cv::Scalar> color_legend_;
	MyPanelBox currentPanelBox_;
	std::string selectedLabel_;
	
	cv::Rect sourceImageRect_;
	std::map<std::string, MyImage> allLabeledImages_;
	
	bool quit_all_, show_object_id_;
	
	cv::Rect panelRectInGlobal_;
	
	MyImage currentImageObjects_;
	std::string labeled_images_dir_;
};

#endif
