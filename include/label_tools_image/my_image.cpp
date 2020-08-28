#include "my_image.h"
#include "utils.h"

// ################################
// Labeled Object in an image

LabeledObject::LabeledObject(std::string label, int class_id, cv::Point center, int w, int h, cv::Size imageSize)
{
	label_ = label;
	class_id_ = class_id;
	center_ = cv::Point2f(
							(float)center.x / (float)imageSize.width, 
							(float)center.y / (float)imageSize.height
						);
	size_ = cv::Point2f(
							(float)w / (float)imageSize.width,
							(float)h / (float)imageSize.height
						);
	displayRect_ = cv::Rect(
							center.x - int(w * 0.5),
							center.y - int(h * 0.5),
							w,
							h
						);
	imageSize_ = imageSize;
}

LabeledObject::~LabeledObject()
{
}

cv::Rect LabeledObject::rectangle()
{
	if (class_id_ == -1) {
		return cv::Rect();
	}
	return displayRect_;
}

cv::Rect LabeledObject::rectangle(cv::Size imageSize)
{
	if (class_id_ == -1) {
		return cv::Rect();
	}
	return cv::Rect(
		int((center_.x - (size_.x * 0.5)) * imageSize.width),
		int((center_.y - (size_.y * 0.5)) * imageSize.height),
		int(size_.x * imageSize.width),
		int(size_.y * imageSize.height)
	);
}

std::string LabeledObject::getInfo()
{
	return label_;
}

std::string LabeledObject::getLabel()
{
	return label_;
}

std::string LabeledObject::getLabelInfoStr()
{
	std::stringstream ss;
	ss << class_id_ << " " << center_.x << " " << center_.y << " " << size_.x << " " << size_.y;
	return ss.str();
}

// ################################
// My Image

MyImage::MyImage()
{
	this->init(cv::Mat(), "", -1);
}

MyImage::MyImage(cv::Mat image, std::string id, int index)
{
	this->init(image, id, index);
}

void MyImage::init(cv::Mat image, std::string id, int index)
{
  index_ = index;
	frame_id_ = id;
	image_ = image.clone();
	objects_.clear();
}

MyImage::~MyImage()
{
	if (!image_.empty())
		image_.release();

	objects_.clear();
}

std::string MyImage::getInfo()
{
	return frame_id_;
}

bool MyImage::addObjectsFromLabelFile(std::string file, std::map<std::string, int> annotations) {
  
    // set bounding box
  
    std::ifstream myfile;
		myfile.open(file);
		if (!myfile.is_open()) {
			std::cout << "Failed to read label text from " << utils::colorText(TextType::DANGER_B, file) << std::endl;
			return false;
		}
		
		cv::Mat image = image_.clone();
		if (image.empty()) {
			std::cout << utils::colorText(TextType::DANGER_B, "Invalid image") << std::endl;
			return false;
		}
		
		while(!myfile.eof()) {
			int objectId;
			float cx, cy, w, h;

			myfile >> objectId;
			myfile >> cx;
			myfile >> cy;
			myfile >> w;
			myfile >> h;
			
			cv::Rect rect;
			rect.width = int(w * image.cols);
			rect.height = int(h * image.rows);
			rect.x = int(cx * image.cols) - rect.width / 2;
			rect.y = int(cy * image.rows) - rect.height / 2;
			
      if (objectId >= 0) {
        std::map<std::string, int>::iterator it;
        bool done = false;
        for (it = annotations.begin(); it != annotations.end() && !done; it++) {
          if (objectId == it->second) {
            objects_.insert(
              std::pair<std::string, LabeledObject>(
                "L" + utils::getStrId(int(objects_.size()), 3, '0'), 
                LabeledObject(
                  it->first, 
                  objectId, 
                  cv::Point(rect.x + int(rect.width / 2.0), rect.y + int(rect.height / 2.0)), 
                  rect.width, 
                  rect.height, 
                  image.size()
                )
              )
            );
            done = true;
          }
        }
      }
		}
		std::cout << " -- Image id: " << frame_id_ << utils::colorText(TextType::SUCCESS, cv::format(" has %d objects", int(objects_.size()))) << std::endl;
		return true;
}

void MyImage::addObject(LabeledObject object, std::string id)
{
	// id = frame_id_ + "_" + id;
	objects_.insert(std::pair<std::string, LabeledObject>(id, object));
	std::cout << "    Add: " 
						<< utils::colorText(
								TextType::SUCCESS, 
								cv::format("id: %s, label: %s\t(size: %d)", 
													id.c_str(), 
													object.getInfo().c_str(),
													int(objects_.size())
													)
							)
						<< std::endl;
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

void MyImage::draw(cv::Mat& image, MyLabel classnames, bool showId)
{
	std::map<std::string, LabeledObject>::iterator it;
	for (it = objects_.begin(); it != objects_.end(); it++) {
		cv::Rect rect = it->second.rectangle(image.size());
		int top = rect.y;
		int left = rect.x;
		
		MyLabel::iterator it2 = classnames.find(it->second.getLabel());
		
		if (it2 != classnames.end()) {
			cv::rectangle(image, rect, it2->second.color, 2);

			this->drawTextWithBackground(
				image,
				top, 
				left,
				it->second.getLabel(),
				cv::FONT_HERSHEY_SIMPLEX,
				0.5,
				1,
				cv::Scalar(255, 255, 255),
				it2->second.color
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
					it2->second.color
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


