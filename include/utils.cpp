#include "utils.h"

#include <sstream>
#include <boost/filesystem.hpp>

namespace utils {
const std::string COLOR_OFF = "\033[0m";	
const std::string SUCCESS = "\033[0;32m";
const std::string DANGER = "\033[0;31m";	
const std::string WARNING = "\033[0;33m";
const std::string SECONDARY = "\033[0;35m";
const std::string CYAN = "\033[0;36m";
const std::string SUCCESS2 = "\033[1;32m";
const std::string DANGER2 = "\033[1;31m";
const std::string WARNING2 = "\033[1;33m";
const std::string SECONDARY2 = "\033[1;35m";	
const std::string CYAN2 = "\033[1;36m";	
}

std::string utils::colorText(int state, std::string text)
{
	switch (state) {
		case TextType::INFO: { text = utils::CYAN + text + utils::COLOR_OFF; break; }
		case TextType::INFO_B: { text = utils::CYAN2 + text + utils::COLOR_OFF; break; }
		case TextType::SUCCESS: { text = utils::SUCCESS + text + utils::COLOR_OFF; break; }
		case TextType::SUCCESS_B: { text = utils::SUCCESS2 + text + utils::COLOR_OFF; break; }
		case TextType::WARNING: { text = utils::WARNING + text + utils::COLOR_OFF; break; }
		case TextType::WARNING_B: { text = utils::WARNING2 + text + utils::COLOR_OFF; break; }
		case TextType::DANGER: { text = utils::DANGER + text + utils::COLOR_OFF; break; }
		case TextType::DANGER_B: { text = utils::DANGER2 + text + utils::COLOR_OFF; break; }
	}
	return text;
}

void utils::findFiles(const std::string path, std::string filter, std::vector<std::string>& outputs)
{
	outputs.clear();
	for (auto &p : boost::filesystem::recursive_directory_iterator(path)) {
		std::stringstream value;
		value << p.path().c_str();
		if (value.str().find(filter) != std::string::npos) {
			outputs.push_back(value.str());
		}
	}
}

std::string utils::getStrId(int id, int N, char prefix)
{
	std::string text = std::to_string(id);
	while (text.size() < N) {
		text = std::string(1, prefix) + text;
	}
	return text;
}

