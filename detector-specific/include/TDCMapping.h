#ifndef TDCMAPPING_H
#define TDCMAPPING_H

// C/C++ includes
#include <string>
#include <vector>

// Unpacker includes
#include "TDCData.h"
#include "V1190.h"

/**
 * !\brief V1190 channel -> TDCData member, from input/TDC/TDCMapping.csv ("Channel, Variable").
 *
 * Empty names and csi_* channels are skipped (CsI times go to HIRA.fCsITime).
 * Fill() stores the first leading-edge hit of each named channel.
 */
class TDCMapping{
	public:
		bool Load(const std::string &path);
		void Fill(const V1190 &tdc, TDCData &data) const;

	private:
		std::vector<std::string> fName;	// channel -> variable ("" = not used)
};

#endif // TDCMAPPING_H
