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

MyTools::MyTools(std::string main_winname)
	: main_winname_(main_winname)
{
	click_polygon_status_ = -1;
	prev_clicked_pt_ = cv::Point(0, 0);
	current_polygon_mode_ = Mode::FREE_RECT;
	
	cv::namedWindow(main_winname_, 1);
	cv::setMouseCallback(main_winname_, onMouse, this);
}

MyTools::~MyTools()
{

}

void MyTools::setClassNames(MyLabel classnames)
{
	classnames_ = classnames;
	std::cout << " ** Set " << int(classnames_.size()) << " class names" << std::endl;
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
	
	if (currentImageSize_.width > 0 && currentImageSize_.height > 0) {
    cv::Rect rect = this->getBoundingBox(current_polygon_, -1);
    this->checkPolygonBorderCollision(rect, currentImageSize_, pt);
    this->setPolygon(current_polygon_, rect);
  }
	
	prev_clicked_pt_ = pt;
}

void MyTools::mouseRelease()
{
	click_polygon_status_ = -1;
}

void MyTools::checkPolygonBorderCollision(cv::Rect &rect, cv::Size imageSize, cv::Point &mousePt)
{
	if (mousePt.x < 0) { 
    mousePt.x = 0; 
  } else if (mousePt.x > imageSize.width) { 
    mousePt.x = imageSize.width; 
  }

	if (mousePt.y < 0) { 
    mousePt.y = 0; 
  } else if (mousePt.y > imageSize.height) { 
    mousePt.y = imageSize.height; 
  }
	
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

void MyTools::loop(std::vector<MyImage> &images)
{
	quit_all_ = false;
  show_object_id_ = false;
  
	if (images.size() == 0) {
		std::cout << utils::colorText(TextType::DANGER_B, " ** No available input images") << std::endl;
		return;
	}
		
	std::vector<MyImage>::iterator imv = images.begin();
	int panelImgHeight = 100;
	
	this->setPolygon(current_polygon_, cv::Rect(rand() % 100, rand() % 100, 100, 100));
  currentPanelBox_ = MyPanelBox();
  selectedLabel_ = classnames_.size() > 0 ? classnames_.begin()->first : "";
      
	while (!quit_all_) {
		if (imv == images.end()) {
			imv = images.begin();
		}

    cv::Mat image = imv->getImage().clone();
    currentImageSize_ = image.size();
    
		std::cout << "\nCurrent Image " 
      << utils::colorText(
          TextType::SUCCESS_B, 
          cv::format("id: %s, size: %d x %d", imv->getInfo().c_str(), image.cols, image.rows)
        ) 
      << std::endl;
		
    
		if (!image.empty()) {
      sourceImageRect_ = cv::Rect(0, 0, image.cols, image.rows);
      cv::Rect panelImageRect(0, image.rows, image.cols, panelImgHeight);
      
      MyImage objects = (*imv);
			int res = this->editImage(image, objects, panelImageRect, int(images.size()));
      (*imv) = objects;
      
      std::cout << " -- Label: " << (*imv).getObjects().size() << std::endl;
			switch (res) {
        case 0:   { quit_all_ = true; break; }
        case 1:   { imv++; break; }
        case -1:  { imv--; break; }
      }
		} else {
			std::cout << utils::colorText(TextType::DANGER_B, "Failed to read image: " + imv->getInfo()) << std::endl;
      imv++;
		}
	}
	cv::destroyAllWindows();
}

int MyTools::editImage(cv::Mat& image, MyImage& objects, cv::Rect panelRectInGlobal, int totalImageNum)
{
  int response = 1;
  bool quit_this_action = false;

  cv::Rect sourceRectInGlobal(0, 0, image.cols, image.rows);
  cv::Mat bottomPanelImage = cv::Mat::zeros(panelRectInGlobal.height, image.cols, CV_8UC3);
  double flag = -1;
  int counter = objects.getObjects().size();
  std::string selectedBoxId = "";
  
  while (!quit_this_action) {
    
    cv::Mat displayImage;
    if (objects.isOk() && panelRectInGlobal.width > 0 && panelRectInGlobal.height > 0) {
      cv::Mat frame = image.clone();
      int height = frame.rows + panelRectInGlobal.height;
			int width = frame.cols;
			displayImage = cv::Mat::zeros(height, width, CV_8UC3);
      objects.draw(frame, classnames_, show_object_id_);
			this->drawPolygon(frame, current_polygon_);

      // vertical concat
      frame.copyTo(displayImage(sourceRectInGlobal));
      cv::Mat panelImage = bottomPanelImage.clone();
      this->renderAnnotationBox(panelImage, currentPanelBox_, flag);
      
      if (!panelImage.empty()) {
        panelImage.copyTo(displayImage(panelRectInGlobal));
      }
    }
    
    if (!displayImage.empty()) {
			int baseLine;
      std::string text = cv::format("[%d of %d] Id: %s (%d boxes)", objects.getIndex(), totalImageNum, objects.getImageId().c_str(), (int)objects.getObjects().size());
			cv::Size text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseLine);
			cv::rectangle(displayImage, cv::Rect(5, 20 - baseLine * 3, text_size.width, text_size.height + baseLine * 2), cv::Scalar(255, 0, 0), -1);
			cv::putText(
        displayImage, 
        text, 
        cv::Point(5, 20), 
        cv::FONT_HERSHEY_SIMPLEX, 0.6, 
        cv::Scalar(255, 255, 255), 1
      );
      
      cv::imshow(main_winname_, displayImage);
    }
    
    char key = cv::waitKey(5);
    switch (key) {
      case 27: {
        // Quit all
        quit_this_action = true;
        response = 0;
        break;
      }
      case 'q': {
        // Go to next image
        quit_this_action = true;
        response = 1;
        break;
      }
      case 'w': {
        // Go to previous image
        quit_this_action = true;
        response = -1;
        break;
      } 
      case '1': {
        show_object_id_ = !show_object_id_;
        break;
      }
      case 'd': {
				std::string target_id("");
				std::cout << " ** Want to delete which id: ";
				std::cin >> target_id;
				objects.deleteObjectById(target_id);
				break;
			}
      case 'f': {
        std::cout << " ** Delete the last object" << std::endl;
        objects.deleteLastObject();
        break;
      }
      case 'a': {
        // Add a new object
        if (objects.isOk()) {
          objects.addObject(
            this->getLabeledObject(
              selectedLabel_,
              classnames_[selectedLabel_].id,
              current_polygon_,
              image.size()
            ),
            "L" + utils::getStrId(counter, 3, '0')
          );
          counter++;
        }
        break;
      }
			case 'r': {
				std::cout << " ** " << utils::colorText(TextType::WARNING, "Reset polygon") << std::endl;
				this->setPolygon(current_polygon_, cv::Rect(rand() % 100, rand() % 100, 100, 100));
				break;
			}
			case 'h': { 
        this->printHelp(); 
        break; 
      }
    }
  }
  return response;
}

void MyTools::printHelp()
{
	std::stringstream ss;
	ss << "\nSoftware usage:"
			<< "\n   ESC | to quit the software"
			<< "\n   q   | to show the next image"
			<< "\n   w   | to show the previous image"
			<< "\n   a   | to add the labeled box to the database"
			<< "\n   1   | to display 'ID' to all labeled boxes"
			<< "\n   d   | to delete a labeled box by 'ID'"
			<< "\n   f   | to delete the last labeled box"
      << "\n   r   | to reset polygon position";
	std::cout << utils::colorText(TextType::INFO_B, ss.str()) << std::endl;
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
