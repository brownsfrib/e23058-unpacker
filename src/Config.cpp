#include "Config.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

bool Config::Load(const std::string &path){
	fFile = path;
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mConfig: cannot open " << path << "\033[0m\n";
		return false;
	}
	std::string line, section;
	while (std::getline(fin, line)){
		size_t c = line.find_first_of("%#");
		if (c != std::string::npos){
			line = line.substr(0, c);
		}
		std::istringstream ss(line);
		std::string key;
		if (!(ss >> key)){
			continue;
		}
		if (key[0] == '['){
			section = key.substr(1, key.find(']') - 1);
			continue;
		}
		std::vector<std::string> values;
		std::string v;
		while (ss >> v){
			values.push_back(v);
		}
		fEntries[Key(section, key)].push_back(values);
	}
	return true;
}

bool Config::Has(const std::string &section, const std::string &key) const{
	return fEntries.count(Key(section, key)) > 0;
}

std::vector<std::vector<std::string>> Config::GetAll(const std::string &section, const std::string &key) const{
	Key k(section, key);
	std::map<Key, std::vector<std::vector<std::string>>>::const_iterator it = fEntries.find(k);
	if (it == fEntries.end()){
		return std::vector<std::vector<std::string>>();
	}
	fUsed.insert(k);
	return it->second;
}

std::string Config::GetString(const std::string &section, const std::string &key, const std::string &def) const{
	std::vector<std::vector<std::string>> all = GetAll(section, key);
	if (all.empty() || all.back().empty()){
		return def;
	}
	return all.back()[0];		// last occurrence wins
}

double Config::GetDouble(const std::string &section, const std::string &key, double def) const{
	std::string s = GetString(section, key);
	return s.empty() ? def : std::atof(s.c_str());
}

int Config::GetInt(const std::string &section, const std::string &key, int def) const{
	std::string s = GetString(section, key);
	return s.empty() ? def : std::atoi(s.c_str());
}

void Config::ReportUnused() const{
	for (std::map<Key, std::vector<std::vector<std::string>>>::const_iterator it = fEntries.begin(); it != fEntries.end(); ++it){
		if (!fUsed.count(it->first)){
			std::cerr << "\033[1;33mConfig " << fFile << ": [" << it->first.first << "] "
			          << it->first.second << " is not used by any class (typo?)\033[0m\n";
		}
	}
}
