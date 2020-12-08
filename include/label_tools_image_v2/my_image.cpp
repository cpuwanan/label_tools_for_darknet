#include "my_image.h"
#include "utils.h"

// ################################
// Labeled Object in an image

LabeledObject::LabeledObject(std::string name, int class_id, double x, double y, double w, double h)
{
	classname_ = name;
	class_id_ = class_id;
	x_ = x;
	y_ = y;
	w_ = w;
	h_ = h;
	
	if (x_ < 0) { x_ = 0.0; }
	if (x_ > 1.0) { x_ = 1.0; }
	if (y_ < 0) { y_ = 0.0; }
	if (y_ > 1.0) { y_ = 1.0; }
}

LabeledObject::~LabeledObject()
{
}

cv::Rect LabeledObject::rectangle(int width, int height)
{
	if (class_id_ == -1) {
		return cv::Rect();
	}
	return cv::Rect(
		int(x_ * width),
		int(y_ * height),
		int(w_ * width),
		int(h_ * height)
	);
}

MyPolygon LabeledObject::polygon(int width, int height)
{
	cv::Rect rect = this->rectangle(width, height);
	if (rect.width == 0 || rect.height == 0) {
		return std::vector<cv::Point>();
	}
	
	std::vector<cv::Point> points;
	points.push_back(cv::Point(rect.x, rect.y));
	points.push_back(cv::Point(rect.x + rect.width, rect.y));
	points.push_back(cv::Point(rect.x + rect.width, rect.y + rect.height));
	points.push_back(cv::Point(rect.x, rect.y + rect.height));
	return points;
}

std::string LabeledObject::getInfo()
{
	double cx = x_ + w_ / 2;
	double cy = y_ + h_ / 2;
	std::stringstream ss;
	ss << std::setprecision(4) << std::fixed;
	ss << class_id_ << " " << cx << " " << cy << " " << w_ << " " << h_;
	return ss.str();
}

// ################################
// My Image

MyImage::MyImage()
{
	name_ = "";
	objects_.clear();
}

void MyImage::init(std::string id, int image_index)
{
	image_index_ = image_index;
	name_ = id;
	objects_.clear();
}

MyImage::~MyImage()
{
	name_ = "";
	objects_.clear();
}

void MyImage::clearAllObjects()
{
	objects_.clear();
}

void MyImage::addObject(LabeledObject object, std::string id)
{
	objects_.insert(std::pair<std::string, LabeledObject>(id, object));
	std::cout << "[Add] " << utils::colorText(SUCCESS_B, id) << ", current size: " << int(objects_.size()) << std::endl;
}

void MyImage::deleteObjectById(std::string id)
{
	std::map<std::string, LabeledObject>::iterator it = objects_.find(id);
	if (it != objects_.end()) {
		// Delete the object
		objects_.erase(it);
		std::cout << utils::colorText(TextType::WARNING, " ** Delete label id: " + id) << std::endl;
	} else {
		std::cout << utils::colorText(TextType::DANGER, " ** Incorrect target id. Nothing will be deleted.") << std::endl;		
	}
}

void MyImage::deleteLastObject()
{
	if (objects_.size() > 0) {
		std::map<std::string, LabeledObject>::iterator it = --objects_.end();
		objects_.erase(it);
	}
}

void MyImage::draw(cv::Mat& image, MyLabel classnames, bool showId, int selectedIndex)
{
	std::map<std::string, LabeledObject>::iterator it;
	int index = 0;
	int N = (int)objects_.size();
	for (it = objects_.begin(); it != objects_.end(); it++, index++) {
		cv::Rect rect = it->second.rectangle(image.cols, image.rows);
		int top = rect.y;
		int left = rect.x;
		
		MyLabel::iterator it2 = classnames.find(it->second.name());
		
		if (it2 != classnames.end()) {			
			cv::Scalar bg_color = it2->second.color;
			double fontScale = 0.5;
			int thickness = 2;
			if (selectedIndex >= 0) {
				if (selectedIndex % N == index) {
					bg_color = cv::Scalar(0, 0, 255);
					fontScale = 0.7;
					thickness = 3;
				}
			}
			
			cv::rectangle(image, rect, bg_color, thickness);

			this->drawTextWithBackground(
				image,
				top, 
				left,
				it->second.name(),
				cv::FONT_HERSHEY_SIMPLEX,
				fontScale,
				1,
				cv::Scalar(255, 255, 255),
				bg_color
			);
			
			if (showId) {
				// Display object id for 'delete' or 'edit'
				cv::putText(image, it->first, cv::Point(left + 5, rect.y + rect.height * 0.5), cv::FONT_HERSHEY_SIMPLEX, 0.5, it2->second.color, 1);
				this->drawTextWithBackground(
					image,
					rect.y + rect.height * 0.5, 
					left + 5,
					it->first,
					cv::FONT_HERSHEY_SIMPLEX,
					0.6,
					1,
					cv::Scalar(255, 255, 255),
					bg_color
				);
			}
		}
	}
}

void MyImage::drawTextWithBackground(cv::Mat& image, int top, int left, std::string text, int fontFace, double fontScale, int fontThickness, cv::Scalar textColor, cv::Scalar bgColor)
{
	// Display
	int baseLine;
	cv::Size labelSize = cv::getTextSize(text, fontFace, fontScale, fontThickness, &baseLine);
	top = std::max(top, labelSize.height) - baseLine;
	cv::rectangle(image, cv::Rect(left, top - labelSize.height, labelSize.width, labelSize.height + baseLine), bgColor, -1);
	cv::putText(image, text, cv::Point(left, top), fontFace, fontScale, textColor, fontThickness);
}


