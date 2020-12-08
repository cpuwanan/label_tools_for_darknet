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

MyTools::MyTools(
	std::string main_winname, 
	std::string setting_winname
)
	: main_winname_(main_winname), setting_winname_(setting_winname)
{
	click_polygon_status_ = -1;
	display_scale_ = 100;
	prev_clicked_pt_ = cv::Point(0, 0);
	current_polygon_mode_ = Mode::FREE_RECT;
	allLabeledImages_.clear();
	
	cv::namedWindow(main_winname_, 1);
	cv::setMouseCallback(main_winname_, onMouse, this);
}

MyTools::~MyTools()
{
}

void MyTools::setClassNames(MyLabel classnames)
{
	classnames_ = classnames;
	std::cout << " ** Set " << int(classnames_.size()) << " classnames" << std::endl;
}

void MyTools::setImageFilenames(std::vector<std::string> files, std::string input_dir, std::string output_dir)
{
	input_dir_ = input_dir;
	output_dir_ = output_dir;
	std::cout << utils::colorText(WARNING_B, "Loading images ...") << std::endl;
	image_list_.clear();
	std::vector<std::string>::iterator it;
	int index = 0;
	for (it = files.begin(); it != files.end(); it++, index++) {
		ImageInfo info;
		info.id = index;
		info.name = (*it);
		info.image = cv::imread(cv::format("%s/%s", input_dir_.c_str(), (*it).c_str()), cv::IMREAD_COLOR);
		image_list_.push_back(info);
		std::cout << " >> " << utils::colorText(WARNING_B, cv::format("%d images", int(image_list_.size()))) << std::endl;
	}
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
	//this->checkPolygonBorderCollision(rect, currentImageObjects_.getImageSize(), pt);
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

void MyTools::renderAnnotationBox(cv::Mat& image, MyPanelBox &panelBox, double scale, std::string image_label)
{	
	if (image.empty()) { return; }
	
	if (classnames_.size() == 0) {
		std::cout << utils::colorText(TextType::DANGER_B, " ** No classname was defined");
		return;
	}
	
	int fontFace = cv::FONT_HERSHEY_SIMPLEX;
	double fontScale = std::min(double(image.cols) / 800.0, 1.0);
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
	
	// Name label on image
	int label_box_height = 20;
	cv::Rect label_rect(0, image.rows - label_box_height, image.cols, label_box_height);
	cv::rectangle(image, label_rect, cv::Scalar(0, 0, 0), -1);
	int baseLine = 0;
	cv::Size text_size(0, 0);
	fontScale = 1.0;
	do {
		text_size = cv::getTextSize(image_label, cv::FONT_HERSHEY_SIMPLEX, fontScale, 1, &baseLine);
		fontScale -= 0.1;
	} while (text_size.width >= image.cols);
	cv::putText(image, image_label, cv::Point(5, label_rect.y + baseLine), cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(255, 255, 255), 1);
}

void MyTools::run()
{
	quit_all_ = false;

	// Map of 'video_id' (i.e., 000010) and 'full_path_filename'
	int panelImgHeight = 100;
	
	this->setPolygon(current_polygon_, cv::Rect(rand() % 100, rand() % 100, 100, 100));
		
	int index = 0;
	std::map<std::string, MyImage> drawing_output_list;
	std::map<std::string, MyImage>::iterator itd;
	
	while (!quit_all_) {
		int response = 1;
		MyImage drawing_output;
		cv::Mat src_image = image_list_[index].image.clone();

		std::cout << "[Main] Current image: " << utils::colorText(TextType::SUCCESS_B, image_list_[index].name) << std::endl;

		itd = drawing_output_list.find(image_list_[index].name);
		if (itd != drawing_output_list.end()) {
			drawing_output = itd->second;
		}
		
		if (!src_image.empty()) {			
			response = this->editImage(image_list_[index], drawing_output, cv::Rect(0, src_image.rows, src_image.cols, panelImgHeight));
		} else {
			std::cout << "Failed to load image: " << utils::colorText(TextType::DANGER_B, image_list_[index].name) << std::endl;
		}
		
		if (drawing_output.hasObjects()) {
			if (drawing_output.name() != "") {
				itd = drawing_output_list.find(drawing_output.name());
				if (itd != drawing_output_list.end()) {
					itd->second = drawing_output;
				} else {
					drawing_output_list.insert(std::pair<std::string, MyImage>(image_list_[index].name, drawing_output));
				}
			}
		}
		
		std::cout << "Available drawing list: " << std::endl;
		for (itd = drawing_output_list.begin(); itd != drawing_output_list.end(); itd++) {
			std::cout << " -- [" << itd->first << "] size: " << int(itd->second.getObjects().size()) << std::endl;
		}
		
		int N = int(image_list_.size());
		switch (response) {
			case 0: { quit_all_ = true; break; }
			case 1: { index = (index + 1) % N; break; }
			case -1: { index = (N + index - 1) % N; break; }
		}

		src_image.release();
	}
	
	this->saveToFile(drawing_output_list);
		
	cv::destroyAllWindows();
	std::cout << " ** Good bye" << std::endl;
}

int MyTools::editImage(ImageInfo input, MyImage& drawing, cv::Rect panel_rect)
{
	int response = 1;

	bool quit_this_video = false;
		
	sourceImageRect_ = cv::Rect();
	currentPanelBox_ = MyPanelBox();
	selectedLabel_ = classnames_.size() > 0 ? classnames_.begin()->first : "";
		
	panelRectInGlobal_ = panel_rect;
	
	if (drawing.hasObjects()) {
		current_drawing_ = drawing;
	} else {
		current_drawing_ = MyImage();
		current_drawing_.init(input.name, input.id);
	}
	
	int labeled_object_counter = 0;
  double last_scale = 1.0;
  show_object_id_ = false;
	int label_box_height = 20;
	int drawing_box_index = -1;
			
	while (!quit_this_video) {
		
		cv::Mat image;
		double scale = (double)display_scale_ / 100.0;
		cv::resize(input.image, image, cv::Size(int(input.image.cols * scale), int(input.image.rows * scale)));
		
		if (image.cols > 50 && image.rows > 50) {
			
			std::string text = cv::format("[%d / %d] %s", input.id, int(image_list_.size()), input.name.c_str());
			
			int globalHeight = image.rows + panelRectInGlobal_.height + label_box_height;
			int globalWidth = image.cols;
			cv::Mat globalImage = cv::Mat::zeros(globalHeight, globalWidth, CV_8UC3);
				
			current_drawing_.draw(image, classnames_, show_object_id_, drawing_box_index);
			this->drawPolygon(image, current_polygon_);
				
			// Vertical concat
			sourceImageRect_ = cv::Rect(0, 0, image.cols, image.rows);
			image.copyTo(globalImage(sourceImageRect_));
				
			cv::Mat panelImage = cv::Mat::zeros(panelRectInGlobal_.height + label_box_height, image.cols, CV_8UC3);
				
			double annotationScale = (last_scale != scale) ? scale : -1;
			last_scale = scale;
			this->renderAnnotationBox(panelImage, currentPanelBox_, annotationScale, text);
				
			if (!panelImage.empty()) {
				panelImage.copyTo(globalImage(cv::Rect(0, image.rows, panelImage.cols, panelImage.rows)));
			}
			cv::imshow(main_winname_, globalImage);
		}		
				
		char key = cv::waitKey(5);
		
		switch (key) {
			case 27: {
				quit_this_video = true;
				response = 0;
				break;
			}
			case '1': {
				quit_this_video = true;
				response = 1;
				break;
			}
			case '0': {
				quit_this_video = true;
				response = -1;
				break;
			}
			case '+': {
				display_scale_ += 10;
				display_scale_ = std::min(display_scale_, 200);
				std::cout << "Set display scale: " << display_scale_ << std::endl;
				break;
			}
			case '-': {
				display_scale_ -= 10;
				display_scale_ = std::max(display_scale_, 10);
				std::cout << "Set display scale: " << display_scale_ << std::endl;
				break;
			}
			case 'a': {
				cv::Rect rect = this->getBoundingBox(current_polygon_, -1);
				double x = double(rect.x) / double(image.cols);
				double y = double(rect.y) / double(image.rows);
				double w = double(rect.width) / double(image.cols);
				double h = double(rect.height) / double(image.rows);	
				current_drawing_.addObject(
					LabeledObject(selectedLabel_, classnames_[selectedLabel_].id, x, y, w, h),
					cv::format("%s_%d", selectedLabel_.c_str(), labeled_object_counter)
				);
				labeled_object_counter++;
				break;
			}
			case 's': {
				show_object_id_ = !show_object_id_;
				break;
			}
			case 'h': { this->printHelp(); break; }
			case 'r': {
				std::cout << utils::colorText(WARNING_B, "Clear all boxes !!!") << std::endl;
				current_drawing_.clearAllObjects();
				break;
			}
			default: {
				auto boxes = current_drawing_.getObjects();
				if (int(key) == 9) {
					// Tab press
					drawing_box_index++;
					if (drawing_box_index > 0 && int(boxes.size() > 0)) {
						// Set current_poygon to this box
						std::map<std::string, LabeledObject>::iterator itb = boxes.begin();
						std::advance(itb, drawing_box_index % int(boxes.size()));
						MyPolygon polygon = itb->second.polygon(image.cols, image.rows);
						if (polygon.size() == 4) {
							current_polygon_ = polygon;
						}
					}
					std::cout << drawing_box_index << ", Size: " << boxes.size() << std::endl;
				} else if (int(key) == 32) {
					// Spacebar press
					drawing_box_index = -1;
					std::cout << utils::colorText(WARNING_B, "Exit from editing mode") << std::endl;
				} else if (int(key) == 13) {
					// Enter press
					std::map<std::string, LabeledObject>::iterator itb = boxes.begin();
					std::advance(itb, drawing_box_index % int(boxes.size()));
					current_drawing_.deleteObjectById(itb->first);
					std::cout << utils::colorText(WARNING_B, "Delete an item '" + itb->first + "'") << ". Please redraw the polygon" << std::endl;					
					drawing_box_index = -1;
				}
			}
		}
	}
		
	drawing = current_drawing_;
		
	return response;
}

void MyTools::printHelp()
{
	std::stringstream ss;
	ss << "\nSoftware usage:"
			<< "\n   ESC | to quit the software"
			<< "\n   0   | previous image"
			<< "\n   1   | next image"
			<< "\n   +   | zoom in"
			<< "\n   -   | zoom out"
			<< "\n   a   | to add the labeled box to the database"
			<< "\n   s   | show object id"
			<< "\n   r   | clear all boxes"
			<< "\n   tab      | select a box"
			<< "\n   spacebar | exit box-editing mode"
			<< "\n   enter    | delete the selected box"			
			;
	std::cout << utils::colorText(TextType::INFO_B, ss.str()) << std::endl;
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

void MyTools::saveToFile(std::map<std::string, MyImage> data)
{
	if (data.size() > 0) {
		std::cout << "\n " << utils::colorText(TextType::SUCCESS_B, cv::format("Saving all %d labels", int(data.size()))) << std::endl;
		std::map<std::string, MyImage>::iterator it1;
		for (it1 = data.begin(); it1 != data.end(); it1++) {

			std::cout << "\n" << utils::colorText(TextType::SUCCESS_B, it1->first) << std::endl;

			auto objects = it1->second.getObjects();
			
			bool ok1 = objects.size() > 0;
			bool ok2 = it1->second.imageIndex() >= 0 && it1->second.imageIndex() < int(image_list_.size());
			
			if (ok1 && ok2) {
				std::size_t found = it1->second.name().find(".");
				
				std::string image_filename = it1->second.name();
				std::string data_filename = image_filename;
				if (found != std::string::npos)
					data_filename = image_filename.substr(0, int(found));
				
				data_filename = output_dir_ + "/" + data_filename + ".txt";
				std::ofstream writer;
				writer.open(data_filename);
				
				std::map<std::string, LabeledObject>::iterator it2;
				for (it2 = objects.begin(); it2 != objects.end(); it2++) {
					writer << it2->second.getInfo() << std::endl;
				}
				writer.close();
				
				image_filename = output_dir_ + "/" + image_filename;
				cv::imwrite(image_filename, image_list_[it1->second.imageIndex()].image);
				
				std::cout << " -- " << data_filename 
									<< "\n -- " << image_filename 
									<< std::endl;
			}
		}
	} else {
		std::cout << " ** " << utils::colorText(TextType::DANGER_B, "No available labeled image") << std::endl;
	}
}
