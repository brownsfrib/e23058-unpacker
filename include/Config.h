#ifndef CONFIG_H
#define CONFIG_H

// C/C++ includes
#include <map>
#include <set>
#include <string>
#include <vector>

/**
 * !\brief Reader for the detector config files (FROGConfig.dat, HiRAConfig.dat, ...).
 *
 *   % comment (also #)
 *   [section]                 geometry / calibration / physics
 *   KEY  value value ...      a key may repeat (e.g. DISABLE_CHANNEL)
 *
 * Paths in the files are relative to the directory the unpacker is run from.
 * Each class reads only its own section. ReportUnused() lists keys nobody
 * asked for, which catches typos.
 */
class Config{
	public:
		bool Load(const std::string &path);
		const std::string &File() const { return fFile; }

		bool        Has(const std::string &section, const std::string &key) const;
		std::string GetString(const std::string &section, const std::string &key, const std::string &def = "") const;
		double      GetDouble(const std::string &section, const std::string &key, double def) const;
		int         GetInt(const std::string &section, const std::string &key, int def) const;

		// every occurrence of a repeated key, each as its list of values
		std::vector<std::vector<std::string>> GetAll(const std::string &section, const std::string &key) const;

		void ReportUnused() const;

	private:
		typedef std::pair<std::string, std::string> Key;	// (section, key)
		std::map<Key, std::vector<std::vector<std::string>>> fEntries;
		mutable std::set<Key> fUsed;
		std::string fFile;
};

#endif // CONFIG_H
