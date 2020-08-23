#include <iostream>

#include <opencv2/opencv.hpp>

#include "utils.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace my_utils {
	std::string getNameFromFile(std::string filename, char startBit, std::string fileType) {
		std::string name("");
		std::size_t found1 = filename.find(fileType);
		if (found1 != std::string::npos) {
			int startIndex = -1;
			for (int i=int(found1); i>=0 && startIndex == -1; i--) {
				if (filename[i] == startBit) {
					startIndex = i;
				}
			}
			if (startIndex != -1) {
				int length = int(found1) - startIndex - 1;
				name = filename.substr(startIndex + 1, length);
			}
		}
		return name;
	}
};

class LabelInfo {
public:
	
	LabelInfo(std::string filename, std::string imageType = ".png") 
	{
		name = my_utils::getNameFromFile(filename, '/', imageType);
		if (name == "") {
			return;
		}
		image = cv::imread(filename, 1);
		rects.clear();
	}
	
	bool setBoundingBox(std::string labelTextFile, std::vector<std::string> objectNameArray) {
		std::ifstream myfile;
		myfile.open(labelTextFile);
		if (!myfile.is_open()) {
			std::cout << "Failed to read label text from " << utils::colorText(TextType::DANGER_B, labelTextFile) << std::endl;
			return false;
		}
		
		if (image.empty()) {
			std::cout << utils::colorText(TextType::DANGER_B, "Invalid image") << std::endl;
			return false;
		}
		
		std::cout << " ** Reading label: " << utils::colorText(TextType::SUCCESS_B, labelTextFile) << std::endl;
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
			rects.push_back(rect);
			
			std::string objName("null");
			if (objectId >= 0 && objectId < objectNameArray.size()) {
				objName = objectNameArray[objectId];
			}
			objectNames.push_back(objName);
			objectIds.push_back(objectId);

			std::cout << "    -- " << objName << ", " << rect << std::endl;

		}
		std::cout << "    Found " << int(rects.size()) << " objects" << std::endl;
		return true;
	}
	
	std::string name = "";
	cv::Mat image;
	std::vector<int> objectIds;
	std::vector<std::string> objectNames;
	std::vector<cv::Rect> rects;
};

class FileManager {
public:
	FileManager() {}
	
	void init(std::string config_file) {
		labeled_images_dir_ = "";
		node_ = YAML::LoadFile(config_file);
		
		if (node_["output"]) {
			if (node_["output"]["labeled_images_dir"]) {
				labeled_images_dir_ = node_["output"]["labeled_images_dir"].as<std::string>();
				std::cout << " ** Image dir: " << utils::colorText(TextType::SUCCESS_B, labeled_images_dir_) << std::endl;
			}
		}
		
		if (node_["deep_learning"]) {
			if (node_["deep_learning"]["class_names"]) {
				auto data = node_["deep_learning"]["class_names"];
				if (data.size() > 0) {
					objectNames_.clear();
					for (int i=0; i<data.size(); i++) {
						objectNames_.push_back(data[i].as<std::string>());
					}
				}
			}
		}
	}
	
	std::vector<std::string> getListOfFilesInLabelDir() {
		std::vector<std::string> files;
		for (auto &p : boost::filesystem::recursive_directory_iterator(labeled_images_dir_)) {
			files.push_back(p.path().c_str());
		}
		return files;
	}
	
	std::vector<std::string> getObjectNames() {
		return objectNames_;
	}
	
private:
	YAML::Node node_;
	std::string labeled_images_dir_;
	std::vector<std::string> objectNames_;
};

// **********************************************

class LabelChecker {
public:
	LabelChecker(std::string appname, std::string config_file) 
		: winname_(appname)
	{
		manager_.init(config_file);
		
		std::vector<std::string> files = manager_.getListOfFilesInLabelDir();
		std::vector<std::string> objectNames = manager_.getObjectNames();
		
		// Set labels
		std::map<std::string, std::string> labelTextFiles;
		std::string fileType(".txt");
		for (int i=0; i<files.size(); i++) {
			std::size_t found = files[i].find(fileType);
			if (found != std::string::npos) {
				std::string name = my_utils::getNameFromFile(files[i], '/', fileType);
				labelTextFiles.insert(std::pair<std::string, std::string>(name, files[i]));
			}
		}
		
		// Set images
		std::string imageType(".png");
		for (int i=0; i<files.size(); i++) {
			std::size_t found = files[i].find(imageType);
			if (found != std::string::npos) {
				LabelInfo labelInfo(files[i], ".png");
				if (labelInfo.name != "") {
					std::map<std::string, std::string>::iterator it = labelTextFiles.find(labelInfo.name);
					if (it != labelTextFiles.end()) {
						bool isOk = labelInfo.setBoundingBox(it->second, objectNames);
						if (isOk) {
							allLabels_.push_back(labelInfo);
						}
					}
				}
			}
		}
		
		this->run(int(objectNames.size()));
	}
	
	void run(int objectNum) {
		
		// Init random color for each class
		std::vector<cv::Scalar> colors;
		for (int i=0; i<objectNum; i++) {
			colors.push_back(cv::Scalar(rand() % 200, rand() % 200, rand() % 200));
		}
		
		double textWidthScale = 1.5;
		int fontType = cv::FONT_HERSHEY_SIMPLEX;
		double fontScale = 0.8;
		int fontThickness = 1;
	
		std::vector<LabelInfo>::iterator it;
		for (it = allLabels_.begin(); it != allLabels_.end(); it++) {
			cv::Mat image = it->image.clone();
			
			for (int i=0; i<it->rects.size(); i++) {
				cv::Rect rect = it->rects[i];
				cv::Scalar color = colors[it->objectIds[i]];
				std::string text = it->objectNames[i];
				
				int baseLine = 0;
				cv::Size textSize = cv::getTextSize(text, fontType, fontScale, fontThickness, &baseLine);
				textSize.width *= textWidthScale;
				
				cv::rectangle(image, rect, color, 2);
				cv::rectangle(image, cv::Rect(rect.x, rect.y - textSize.height, textSize.width, textSize.height + baseLine), color, -1);
				cv::putText(image, text, cv::Point(rect.x + 0.1 * textSize.width, rect.y), fontType, fontScale, cv::Scalar(255, 255, 255), fontThickness);
			}
			
			cv::putText(image, "Image name: " + it->name, cv::Point(5, 50), fontThickness, fontScale, cv::Scalar(0, 255, 0), fontThickness);

			cv::imshow(winname_, image);
			cv::waitKey(0);
		}
	}
	
private:
	std::string winname_;
	FileManager manager_;
	std::vector<LabelInfo> allLabels_;
};

// **********************************************

void checkInput(int argc, char **argv, int &i, std::string key, std::string &value) {
	if (i+1 < argc) {
		value = argv[i+1];
		i = i+1;
	} else {
		std::cerr << utils::colorText(TextType::DANGER_B, "'" + key + "' option requires one argument") << std::endl;
	}
}

int main(int argc, char **argv) {
	
	// Create a folder to store results
	std::string appname = argv[0];
	std::size_t found = appname.find("/");
	if (found != std::string::npos) {
		appname = appname.substr(int(found) + 1, appname.size() - int(found));
		std::cout << "\n ** App name: " << utils::colorText(TextType::SUCCESS, appname) << std::endl;
	}
	
	std::string config_file("");
	
	for (int i=1; i<argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			return 1;
		} else if (arg == "-c" || arg == "--config") {
			checkInput(argc, argv, i, "--config", config_file);
		}
	}
	
	if (config_file == "") {
		std::cout << utils::colorText(TextType::DANGER_B, "Invalid config file") << std::endl;
		return -1;
	}
	
	std::ifstream myfile;
	myfile.open(config_file);
	if (!myfile.is_open()) {
		std::cout << utils::colorText(TextType::DANGER_B, "Failed to read " + config_file) << std::endl;
		return -1;
	}
	
	LabelChecker checker(appname, config_file);
	
	return 0;
}
