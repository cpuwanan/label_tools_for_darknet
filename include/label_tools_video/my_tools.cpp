#include "my_tools.h"
#include "utils.h"
#include <chrono>
#include <unistd.h>
#include <ctime>
#include <unistd.h>
#include <fstream>

const int POLYGON_CENTER = 100;

static void onMouse(int event, int x, int y, int flags, void *param) {
	MyTools *pThis = (MyTools*) param;
	if (event == cv::EVENT_LBUTTONDOWN) { 
		pThis->mouseClick(cv::Point(x, y));
		return;
	} else if (event == cv::EVENT_MOUSEMOVE) {
		pThis->mouseMove(cv::Point(x, y));
		return;
	} else if (event == cv::EVENT_LBUTTONUP) {
		pThis->mouseRelease();
		return;
	}
}

MyTools::MyTools(std::string main_winname, std::string setting_winname)
	: main_winname_(main_winname), setting_winname_(setting_winname)
	, renderThread_(NULL)
	, image_subfix_(".jpg")
	, labeled_images_dir_("")
{
	click_polygon_status_ = -1;
	display_scale_ = 50;
	prev_clicked_pt_ = cv::Point(0, 0);
	current_polygon_mode_ = Mode::FREE_RECT;
	image_subfix_ = ".png";
	frame_no_ = 5;
	allLabeledImages_.clear();
	
	cv::namedWindow(main_winname_, 1);
	cv::namedWindow(setting_winname_, 1);
	cv::createTrackbar("Display scale", main_winname_, &display_scale_, 100, 0);
	cv::setMouseCallback(main_winname_, onMouse, this);
}

MyTools::~MyTools()
{
	if (renderThread_ != NULL) {
		renderThread_->join();
	}
}

void MyTools::setClassNames(MyLabel classnames)
{
	classnames_ = classnames;
	std::cout << " ** Set " << int(classnames_.size()) << " class names" << std::endl;
}

void MyTools::setVideoFiles(std::map<std::string, std::string> files)
{
	video_files_ = files;
	std::cout << " ** Set " << int(video_files_.size()) << " videos as input" << std::endl;
}

void MyTools::setSavedImageType(std::string subfix)
{
	image_subfix_ = subfix;
}

void MyTools::setLabeledImagesDir(std::string path)
{
	labeled_images_dir_ = path;
}

void MyTools::mouseClick(cv::Point pt)
{
	bool isPolygonSelection = true;
	
	if (sourceImageRect_.width > 0 && sourceImageRect_.height > 0) {
		bool in_region_x = pt.x >= sourceImageRect_.x && pt.x < sourceImageRect_.x + sourceImageRect_.width;
		bool in_region_y = pt.y >= sourceImageRect_.y && pt.y < sourceImageRect_.y + sourceImageRect_.height;
		isPolygonSelection = (in_region_x && in_region_y);
	}
	
	if (isPolygonSelection) {
		int nearbyPtIndex = -1;
		cv::Rect rect = this->getBoundingBox(current_polygon_, -1);

		if (current_polygon_mode_ == Mode::FREE_RECT || current_polygon_mode_ == Mode::FIXED_RECT) {
			double distThreshold = 20.0;
			
			// In case of very small rect
			if (rect.width < distThreshold || rect.height < distThreshold) {
				int minLength = std::min(rect.width, rect.height);
				distThreshold = int(minLength * 0.3);
			}
			
			double minDist = 100.0;
			for (int i=0; i<int(current_polygon_.size()); i++) {
				double dist = sqrt(
					pow(pt.x - current_polygon_[i].x, 2.0) + pow(pt.y - current_polygon_[i].y, 2.0)
				);
				if (dist < distThreshold && dist < minDist) {
					minDist = dist;
					nearbyPtIndex = i;
				}
			}
		}
		
		if (nearbyPtIndex != -1) {
			current_polygon_[nearbyPtIndex] = pt;
			click_polygon_status_ = nearbyPtIndex;
		} else {
			// Click somewhere inside the polygon
			bool in_range_x = pt.x > rect.x && pt.x < rect.x + rect.width;
			bool in_range_y = pt.y > rect.y && pt.y < rect.y + rect.height;
			click_polygon_status_ = (in_range_x && in_range_y) ? POLYGON_CENTER : click_polygon_status_;
		}
		
		prev_clicked_pt_ = pt;
	} else {
		if (currentPanelBox_.size() > 0) {
			MyPanelBox::iterator it;
			std::string selected = "";
			for(it = currentPanelBox_.begin(); it != currentPanelBox_.end(); it++) {
				cv::Rect rect = it->second.rect;
				// For vconcat only
				cv::Point subpt(pt.x - sourceImageRect_.x, pt.y - sourceImageRect_.height);
				bool in_region_x = subpt.x > rect.x && subpt.x < rect.x + rect.width;
				bool in_region_y = subpt.y > rect.y && subpt.y < rect.y + rect.height;
				if (in_region_x && in_region_y) {
					selected = it->first;
				}
			}
			if (selected != "") {
				selectedLabel_ = selected;
			}
		}
	}
}

void MyTools::mouseMove(cv::Point pt)
{	
	if (current_polygon_.size() == 0) {
		return;
	}

	bool movePolygon = false;
	
	// Click around the polygon vertices
	if (click_polygon_status_ >= 0 && click_polygon_status_ < int(current_polygon_.size())) {
		switch(current_polygon_mode_) {
			case Mode::FREE_RECT: {
				// Drag vertices to form a free shape
				current_polygon_[click_polygon_status_] = pt;
				cv::Rect rect = this->getBoundingBox(current_polygon_, click_polygon_status_);
				this->setPolygon(current_polygon_, rect);
				break;
			}
			case Mode::FIXED_RECT: {
				// Remain the same shape
				movePolygon = true;
				break;
			}
		}
	} else if (click_polygon_status_ == POLYGON_CENTER) {
		movePolygon = true;
	}
	
	if (movePolygon) {
		int dx = pt.x - prev_clicked_pt_.x;
		int dy = pt.y - prev_clicked_pt_.y;
		for (int i=0; i<int(current_polygon_.size()); i++) {
			current_polygon_[i].x += dx;
			current_polygon_[i].y += dy;
		}
	}
	
	cv::Rect rect = this->getBoundingBox(current_polygon_, -1);
	this->checkPolygonBorderCollision(rect, currentImageObjects_.getImageSize(), pt);
	this->setPolygon(current_polygon_, rect);
	
	prev_clicked_pt_ = pt;
}

void MyTools::mouseRelease()
{
	click_polygon_status_ = -1;
}

void MyTools::checkPolygonBorderCollision(cv::Rect &rect, cv::Size imageSize, cv::Point &mousePt)
{
	if (mousePt.x < 0) 										{ mousePt.x = 0; }
	else if (mousePt.x > imageSize.width) { mousePt.x = imageSize.width; }

	if (mousePt.y < 0) 										{ mousePt.y = 0; }
	else if (mousePt.y > imageSize.height) { mousePt.y = imageSize.height; }
	
	if (rect.x < 0)	{ rect.x = 0; }	
	if (rect.y < 0)	{ rect.y = 0; }
	
	int defaultLength = 20;
	
	if ((rect.x + rect.width) > imageSize.width) { 
		rect.x = imageSize.width - defaultLength; 
		rect.width = defaultLength;
	}
	if ((rect.y + rect.height) > imageSize.height) { 
		rect.y = imageSize.height - defaultLength; 
		rect.height = defaultLength;
	}	
}

void MyTools::initAnnotationBoxes(MyPanelBox& annotationBoxes, int width, int height)
{
	annotationBoxes.clear();
	
	if (classnames_.size() == 0) {
		std::cout << utils::colorText(TextType::DANGER_B, " ** No classname was defined");
		return;
	}
	
	MyLabel::iterator it1;
	int xPos = 10;
	int yPos = 30;
	double textWidthScale = 1.5;
	
	int fontType = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = 1.0;
	int fontThickness = 1;
	
	int index = 0;
	for (it1 = classnames_.begin(); it1 != classnames_.end(); it1++, index++) {
		int baseLine = 0;
		cv::Size textSize = cv::getTextSize(it1->first, fontType, fontScale, fontThickness, &baseLine);
		
		textSize.width *= textWidthScale;
		
		if (xPos + textSize.width > width) {
			xPos = 10;
			yPos += 40;
		}
		
		int xTextPos = xPos;
		if (textWidthScale > 1.0) {
			xTextPos += int((textWidthScale - 1.0) * 0.3 * textSize.width);
		}
		
		MyBoxInfo box;
		box.color = it1->second.color;
		box.id = it1->first;
		box.rect = cv::Rect(xPos, yPos - textSize.height, textSize.width, textSize.height + baseLine);
		box.textPt = cv::Point(xTextPos, yPos);
		annotationBoxes.insert(std::pair<std::string, MyBoxInfo>(it1->first, box));
	}
}

void MyTools::renderAnnotationBox(cv::Mat& image, MyPanelBox &panelBox, double scale)
{	
	if (classnames_.size() == 0) {
		std::cout << utils::colorText(TextType::DANGER_B, " ** No classname was defined");
		return;
	}
	
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = 1.0;
	int fontThickness = 1;
	
	if (scale >= 0.0 || panelBox.size() == 0) {
		// std::cout << " ** [" << cv::getTickCount() << "] " <<  utils::colorText(TextType::WARNING_B, cv::format("Readjust rendered panel: (%d x %d)", image.cols, image.rows)) << std::endl;
		
		panelBox.clear();
	
		int xPos = 10;
		int yPos = 30;
		double textWidthScale = 1.5;
		
		MyLabel::iterator it;

		int index = 0;
		for (it = classnames_.begin(); it != classnames_.end(); it++, index++) {
			int baseLine = 0;
			cv::Size textSize = cv::getTextSize(it->first, fontFace, fontScale, fontThickness, &baseLine);
			
			textSize.width *= textWidthScale;
			
			if (xPos + 10 + textSize.width > image.cols) {
				xPos = 10;
				yPos += 40;
			}
			
			int xTextPos = xPos;
			if (textWidthScale > 1.0) {
				xTextPos += int((textWidthScale - 1.0) * 0.3 * textSize.width);
			}
					
			MyBoxInfo box;
			box.color = it->second.color;
			box.id = it->first;
			box.rect = cv::Rect(xPos, yPos - textSize.height, textSize.width, textSize.height + baseLine);
			box.textPt = cv::Point(xTextPos, yPos);
			panelBox.insert(std::pair<std::string, MyBoxInfo>(it->first, box));
			xPos += 10 + textSize.width;
		}
	}
	
	if (panelBox.size() == 0) {
		std::cout << "[" << cv::getTickCount() << "] " <<  utils::colorText(TextType::WARNING_B, "Box panel is empty") << std::endl;
		return;
	}
	
	cv::line(image, cv::Point(0, 0), cv::Point(image.cols, 0), cv::Scalar(255, 255, 255), 2);
	
	MyPanelBox::iterator it;
	int index = 0;
	for (it = panelBox.begin(); it != panelBox.end(); it++, index++) {
		bool isSelected = it->first == selectedLabel_;
		cv::Scalar textColor = isSelected ? cv::Scalar::all(255) : it->second.color;
		int rectThickness = isSelected ? -1 : 1;
		cv::rectangle(image, it->second.rect, it->second.color, rectThickness);
		cv::putText(image, it->first, it->second.textPt, fontFace, fontScale, textColor, fontThickness);
	}
	
}

void MyTools::loop()
{
	quit_all_ = false;
	if (video_files_.size() == 0) {
		std::cout << utils::colorText(TextType::DANGER_B, " ** No available videos") << std::endl;
		return;
	}
		
	// Map of 'video_id' (i.e., 000010) and 'full_path_filename'
	std::map<std::string, std::string>::iterator imv = video_files_.begin();
	int panelImgHeight = 100;
	
	this->setPolygon(current_polygon_, cv::Rect(rand() % 100, rand() % 100, 100, 100));
	
	renderThread_ = new boost::thread(boost::bind(&MyTools::renderPolygonThread, this));
	
	std::map<std::string, std::vector<MyImage>> objectsFromMultipleImages;
	
	while (!quit_all_) {
		if (imv == video_files_.end()) {
			imv = video_files_.begin();
		}
		
		std::cout << "\n[Main] Current video: " << utils::colorText(TextType::SUCCESS_B, imv->second) << std::endl;
		
		cv::VideoCapture cap(imv->second);
		if (cap.isOpened()) {
			info_image_.create(300, 500, CV_8UC3);
			info_image_.setTo(cv::Scalar::all(0));

			int frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);
			int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
			int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
			std::vector<std::string> texts;
			texts.push_back("Id: " + imv->first);
			texts.push_back("File: " + imv->second);
			texts.push_back(cv::format("Size: %d x %d, Frames: %d", width, height, frame_count));
			
			for (int i=0; i<texts.size(); i++) {
				// cv::putText(info_image_, texts[i], cv::Point(10, 30 + i * 30), cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(255, 255, 255));
				std::cout << "   -- " << texts[i] << std::endl;
			}
			// cv::imshow(setting_winname_, info_image_);
			
			bool isNewLabelForThisVideo = true;
			std::vector<MyImage> allObjects;			
			std::map<std::string, std::vector<MyImage>>::iterator it = objectsFromMultipleImages.find(imv->first);
			if (it != objectsFromMultipleImages.end()) {
				allObjects = it->second;
				isNewLabelForThisVideo = false;
				std::cout << "[Main] This video (id: " << imv->first << ") has " << allObjects.size() << " images registered" << std::endl;
			}
			
			// Do reading the video
			int res = this->readVideo(
				cap, 
				allObjects,
				cv::Rect(0, height, width, panelImgHeight),
				imv->first
			);
			
			/*
			for (int i=0; i<allObjects.size(); i++) {
				auto data = allObjects[i];
				cv::Mat showImg = data.getImage().clone();
				cv::resize(showImg, showImg, cv::Size(showImg.cols * 0.3, showImg.rows * 0.3));
				cv::imshow(data.getImageId(), showImg);
			}
			*/
			
			std::cout << "\n[Main] Done for video id: " << imv->first 
								<< "\n       Labeled " << int(allObjects.size()) << " images" 
								<< "\n       " << (isNewLabelForThisVideo ? "IS NEW LABEL" : "EXISTING LABEL AVAILABLE") << std::endl;
			
			if (allObjects.size() > 0) {
				if (isNewLabelForThisVideo) {
					objectsFromMultipleImages.insert(
						std::pair<std::string, std::vector<MyImage>>(
							imv->first, allObjects
						)
					);
					std::cout << "[Main] video id: " << imv->first << ", " 
							<< utils::colorText(TextType::SUCCESS, cv::format("Add a new database. New size: %d", int(objectsFromMultipleImages.size())))
							<< std::endl;
				} else {
					it->second = allObjects;
					std::cout << "[Main] video id: " << imv->first << ", "
							<< utils::colorText(TextType::SUCCESS, "Updated existing data")
							<< std::endl;
				}
			}
			
			quit_all_ = (res == 0);
		} else {
			std::cout << utils::colorText(TextType::DANGER_B, "Failed to read video") << std::endl;
		}
		cap.release();
		imv++;
	}
	
	this->saveLabeledImages(objectsFromMultipleImages);
		
	cv::destroyAllWindows();
	std::cout << " ** Good bye" << std::endl;
}

int MyTools::readVideo(cv::VideoCapture& cap, std::vector<MyImage> &allObjects, cv::Rect panelRectInGlobal, std::string video_id)
{
	int response = 1;
	
	int frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);
	int imageWidth = cap.get(cv::CAP_PROP_FRAME_WIDTH);
	int imageHeight = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
	double fps = cap.get(cv::CAP_PROP_FPS);
	cap.set(cv::CAP_PROP_POS_FRAMES, (double)frame_no_);
	
	cv::Mat frame_test;
	cap >> frame_test;
	
	bool play_video = true;
	bool quit_this_video = false;
	show_object_id_ = false;
	bool requested_to_select_frame = false;
	
	cv::createTrackbar("Frame No", setting_winname_, &frame_no_, frame_count, 0);
	cv::Mat image;
	
	sourceImageRect_ = cv::Rect();
	currentPanelBox_ = MyPanelBox();
	selectedLabel_ = classnames_.size() > 0 ? classnames_.begin()->first : "";
	cv::Mat lastImage;
		
	panelRectInGlobal_ = panelRectInGlobal;
	currentImageObjects_ = MyImage();
	int labeled_object_counter = 0;
		
	while (!quit_this_video) {
				
		cv::Mat frame;
		if (play_video) {
			cap >> frame;
		} else {
			if (!lastImage.empty()) {
				lastImage.copyTo(frame);
			} else if (!frame_test.empty()) {
				frame_test.copyTo(frame);
			} 
		}
		
		double scale = (double)display_scale_ / 100.0;
		if (frame.cols > 50 && frame.rows > 50 && !frame.empty()) {
			// Do processing
			cv::resize(frame, image, cv::Size(int(imageWidth * scale), int(imageHeight * scale)));
		}
		
		if (image.cols > 50 && image.rows > 50) {
			if (requested_to_select_frame) {
				requested_to_select_frame = false;
				int frame_id = (int)cap.get(cv::CAP_PROP_POS_FRAMES);
				
				labeled_object_counter = 0;
				currentImageObjects_.init(image, "M" + utils::getStrId(frame_id, 6, '0'));

				std::cout << " ** Set image: " << utils::colorText(TextType::SUCCESS_B, currentImageObjects_.getInfo()) << std::endl;
			}
			
			// Just for visualization
			cv::String text = play_video 
				? cv::format("PLAYING : Video %s", video_id.c_str()) 
				: cv::format("PAUSE   : Video %s", video_id.c_str());
			cv::Scalar color = play_video ? cv::Scalar(0, 255, 0) : cv::Scalar(0,170, 255);
			int baseLine;
			cv::Size text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseLine);
			cv::rectangle(image, cv::Rect(5, 20 - baseLine * 3, text_size.width, text_size.height + baseLine * 2), color, -1);
			cv::putText(image, text, cv::Point(5, 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 1);
			
			cv::imshow(setting_winname_, image);
			
			image.copyTo(lastImage);
		}
		
		char key = cv::waitKey(5);
		
		switch (key) {
			case 27: {
				quit_this_video = true;
				response = 0;
				break;
			}
			case 'q': {
				quit_this_video = true;
				break;
			}
			case 'p': {
				play_video = !play_video;
				std::cout << " ** Video: " << utils::colorText((play_video ? TextType::SUCCESS_B : TextType::WARNING_B), (play_video ? "Play" : "Pause")) << std::endl;
				break;
			}
			case 's': {
				play_video = true;
				cap.set(cv::CAP_PROP_POS_FRAMES, (double)frame_no_);
				std::cout << " ** Set: " << utils::colorText(TextType::SUCCESS, "starting frame " + std::to_string(frame_no_)) << std::endl;
				break;
			}
			case 'f': {
				requested_to_select_frame = true;
				this->updateObjectsForVideo(allObjects, currentImageObjects_);
				break;
			}
			case 'a': {
				if (currentImageObjects_.isOk()) {
					// Add the current polygon				
					currentImageObjects_.addObject(
						this->getLabeledObject(
							selectedLabel_,
							classnames_[selectedLabel_].id,
							current_polygon_,
							currentImageObjects_.getImageSize()
						),
						utils::getStrId(labeled_object_counter, 3, '0')
					);
					labeled_object_counter++;
				} else {
					std::cout << " ** " << utils::colorText(TextType::DANGER_B, "No selected image for labeling") << std::endl;
				}
				break;
			}
			case 'd': {
				show_object_id_ = !show_object_id_;
				break;
			}
			case 'D': {
				std::string target_id("");
				std::cout << " ** Want to delete which id: ";
				std::cin >> target_id;
				currentImageObjects_.deleteObjectById(target_id);
				break;
			}
			case 'r': {
				std::cout << " ** " << utils::colorText(TextType::WARNING, "Reset polygon") << std::endl;
				this->setPolygon(current_polygon_, cv::Rect(rand() % 100, rand() % 100, 100, 100));
				break;
			}
			case 'h': { this->printHelp(); break; }
		}
		
		/*
		if (allObjects.size() > 0) {
			for (int i=0; i<allObjects.size(); i++) {
				auto data = allObjects[i];
				cv::Mat showImg = data.getImage().clone();
				cv::resize(showImg, showImg, cv::Size(showImg.cols * 0.3, showImg.rows * 0.3));
				cv::imshow(data.getImageId(), showImg);
			}
		}
		*/
		
		int usec_delay = int(1.0 / fps * 1000.0 * 1000.0);
		usleep(usec_delay);
	}
	
	this->updateObjectsForVideo(allObjects, currentImageObjects_);
	
	return response;
}

void MyTools::printHelp()
{
	std::stringstream ss;
	ss << "\nSoftware usage:"
			<< "\n   ESC | to quit the software"
			<< "\n   q   | to play the next video file"
			<< "\n   p   | 'TOGGLE' to play or pause the video"
			<< "\n   s   | to set the starting frame No. for video play"
			<< "\n   f   | to set the labels on the current image"
			<< "\n   a   | to add the labeled box to the database"
			<< "\n   d   | to display 'ID' to all labeled boxes"
			<< "\n   D   | to delete a labeled box by 'ID'"
			<< "\n   r   | to reset polygon position";
	std::cout << utils::colorText(TextType::INFO_B, ss.str()) << std::endl;
}

void MyTools::renderPolygonThread()
{
	double last_scale = 1.0;
	show_object_id_ = false;
	double fps = 40.0;
	while(!quit_all_) {
				
		if (currentImageObjects_.isOk() && panelRectInGlobal_.width > 0 && panelRectInGlobal_.height > 0) {
			double scale = (double)display_scale_ / 100.0;
			// cv::Mat src = currentImageObjects_.getImage().clone();
			cv::Mat image = currentImageObjects_.getImage().clone();
			// cv::resize(src, image, cv::Size(src.cols * scale, src.rows * scale));
			int globalHeight = image.rows + panelRectInGlobal_.height;
			int globalWidth = image.cols;
			cv::Mat globalImage = cv::Mat::zeros(globalHeight, globalWidth, CV_8UC3);
				
			currentImageObjects_.draw(image, classnames_, show_object_id_);
			this->drawPolygon(image, current_polygon_);
				
			// Vertical concat
			sourceImageRect_ = cv::Rect(0, 0, image.cols, image.rows);
			image.copyTo(globalImage(sourceImageRect_));
				
			cv::Mat panelImage = cv::Mat::zeros(panelRectInGlobal_.height, image.cols, CV_8UC3);
				
			double annotationScale = (last_scale != scale) ? scale : -1;
			last_scale = scale;
			this->renderAnnotationBox(panelImage, currentPanelBox_, annotationScale);
				
			if (!panelImage.empty()) {
				panelImage.copyTo(globalImage(cv::Rect(0, image.rows, panelImage.cols, panelImage.rows)));
			}
				
			cv::imshow(main_winname_, globalImage);
		}		
		
		int usec_delay = int(1.0 / fps * 1000.0 * 1000.0);
		usleep(usec_delay);
	}
}

LabeledObject MyTools::getLabeledObject(std::string label, int id, MyPolygon polygon, cv::Size imageSize)
{
	cv::Rect rect = this->getBoundingBox(polygon, -1);
	cv::Point center(
		rect.x + int(rect.width * 0.5),
		rect.y + int(rect.height * 0.5)
	);
	return LabeledObject(label, id, center, rect.width, rect.height, imageSize);
}

void MyTools::setPolygon(MyPolygon& polygon, cv::Rect rect)
{
	polygon.clear();
	polygon.push_back(cv::Point(rect.x, rect.y));
	polygon.push_back(cv::Point(rect.x + rect.width, rect.y));
	polygon.push_back(cv::Point(rect.x + rect.width, rect.y + rect.height));
	polygon.push_back(cv::Point(rect.x, rect.y + rect.height));
}

cv::Rect MyTools::getBoundingBox(MyPolygon polygon, int currentPtIndex) {
	cv::Point min_pt(10000, 10000), max_pt(0, 0);
	for (int i=0; i<polygon.size(); i++) {
		min_pt.x = std::min(min_pt.x, polygon[i].x);
		min_pt.y = std::min(min_pt.y, polygon[i].y);
		max_pt.x = std::max(max_pt.x, polygon[i].x);
		max_pt.y = std::max(max_pt.y, polygon[i].y);
	}

	if (currentPtIndex >= 0 && currentPtIndex < int(polygon.size())) {
		cv::Point request_pt =  polygon[currentPtIndex];
		switch (currentPtIndex) {
			case 0: { min_pt = request_pt; break; }
			case 1: { min_pt.y = request_pt.y; max_pt.x = request_pt.x; break; }
			case 2: { max_pt = request_pt; break; }
			case 3: { min_pt.x = request_pt.x; max_pt.y = request_pt.y; break; }
		}
	}

	return cv::Rect(min_pt.x, min_pt.y, abs(max_pt.x - min_pt.x), abs(max_pt.y - min_pt.y));
}

void MyTools::drawPolygon(cv::Mat& image, MyPolygon polygon)
{
	cv::Scalar color(0, 170, 255);
	int lineThickness = 2;
	if (current_polygon_mode_ == Mode::FIXED_RECT) {
		color = cv::Scalar(0, 0, 255);
	}
	
	if (click_polygon_status_ == POLYGON_CENTER) {
		color = cv::Scalar(0, 255, 255);
		lineThickness = 1;
	}
	
	for (int i=0; i<polygon.size(); i++) {
		int i2 = (i + 1) % int(polygon.size());
		cv::circle(image, polygon[i], 3, color, -1);
		cv::line(image, polygon[i], polygon[i2], color, lineThickness);
	}
		
	cv::Rect rect = this->getBoundingBox(polygon, -1);
	// cv::rectangle(image, rect, color, lineThickness);
	cv::Point center = cv::Point(rect.x + int(rect.width * 0.5), rect.y + int(rect.height * 0.5));
	cv::circle(image, center, 4, color, -1);
}

void MyTools::updateObjectsForVideo(std::vector<MyImage>& array, MyImage data)
{
	if (data.isOk() && data.hasObjects()) {
		// Check if already register this image
		bool isNewData = true;
		int index = 0;
		std::vector<MyImage>::iterator it;
		for (it = array.begin(); it != array.end() && isNewData; it++, index++) {
			if (it->getImageId() == data.getImageId()) {
				isNewData = false;
			}
		}
		if (isNewData) {
			array.push_back(data);
			auto lastItem = array.back();			
			std::cout << " ** Added a new database, New size: " << array.size() << std::endl;
		} else { 
			array[index] = data;
			std::cout << " ** Updated existing database index: " << index << ", New size: " << array.size() << std::endl;
		}
	} else {
		std::cout << " ** " << utils::colorText(TextType::WARNING, "No available objects labeled. Ignore database update.") << std::endl;
	}
}

void MyTools::saveLabeledImages(std::map<std::string, std::vector<MyImage> > data)
{
	if (data.size() > 0) {
		std::cout << "\n " << utils::colorText(TextType::SUCCESS_B, cv::format("Saving all %d labeled videos", int(data.size()))) << std::endl;
		std::map<std::string, std::vector<MyImage> >::iterator it1;
		std::vector<MyImage>::iterator it2;
		for (it1 = data.begin(); it1 != data.end(); it1++) {

			std::cout << "\n Recording data for video:" << utils::colorText(TextType::SUCCESS_B, it1->first) << std::endl;

			for (it2 = it1->second.begin(); it2 != it1->second.end(); it2++) {
			
				auto objects = it2->getObjects();
				if (objects.size() > 0) {

					std::string prefix = cv::format("%s/%s_%s", labeled_images_dir_.c_str(), it1->first.c_str(), it2->getImageId().c_str());
					std::string imageFile = prefix + image_subfix_;
					std::string labelBoxFile = prefix + ".txt";

					std::ofstream myfile;
					myfile.open(labelBoxFile);	
				
					cv::imwrite(imageFile, it2->getImage());
					std::cout << " ** Image: " << utils::colorText(TextType::SUCCESS_B, imageFile) << std::endl;
					
					std::map<std::string, LabeledObject>::iterator it3;
					for (it3 = objects.begin(); it3 != objects.end(); it3++) {
						myfile << it3->second.getLabelInfoStr() << "\n";
					}
					myfile.close();
					std::cout << "    File:  " << utils::colorText(TextType::SUCCESS_B, labelBoxFile) << std::endl;
				}
			}
		}
	} else {
		std::cout << " ** " << utils::colorText(TextType::DANGER_B, "No available labeled image") << std::endl;
	}
}
