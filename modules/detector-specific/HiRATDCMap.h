#ifndef HIRATDCMAP_H
#define HIRATDCMAP_H

// C/C++ includes
#include <iostream>
#include <string>
#include <vector>

// What one TDC channel means
struct TDCMapEntry {
	bool        valid            = false;	// false: channel not mapped
	std::string name;				// variable name, e.g. "CsI_3"
	int         referenceChannel = -1;	// -1: no reference subtraction
};

/**
 * !\brief TDC channel -> variable mapping. SKELETON.
 *
 * Load() is a placeholder until the file format (CSV / binary) is decided.
 * Set() fills entries by hand in the meantime.
 */
class HiRATDCMap{
	public:
		static const int kChannels = 128;

		bool Load(const std::string &path){
			// TODO: read the mapping file once its format is fixed
			std::cerr << "\033[1;33mHiRATDCMap::Load(" << path
			          << "): not implemented yet - no TDC mapping loaded\033[0m\n";
			return false;
		}

		void Set(int channel, const std::string &name, int referenceChannel){
			if (channel < 0 || channel >= kChannels){
				return;
			}
			TDCMapEntry &e = fTable[channel];
			e.valid            = true;
			e.name             = name;
			e.referenceChannel = referenceChannel;
		}

		const TDCMapEntry &Get(int channel) const{
			if (channel < 0 || channel >= kChannels){
				return fNone;
			}
			return fTable[channel];
		}

	private:
		std::vector<TDCMapEntry> fTable = std::vector<TDCMapEntry>(kChannels);
		TDCMapEntry fNone;
};

#endif // HIRATDCMAP_H
