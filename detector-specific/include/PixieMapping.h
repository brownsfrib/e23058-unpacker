#ifndef PIXIEMAPPING_H
#define PIXIEMAPPING_H

// C/C++ includes
#include <map>
#include <string>
#include <vector>

// Unpacker includes
#include "Config.h"
#include "PixieData.h"

/**
 * !\brief Pixie channel map: (module, channel) -> wall, bar, side, and the
 *         split of each event's hits into the LANA. and VETO. branches.
 *
 * PixieConfig.dat [geometry]:
 *     CHANNEL_MAP  <path>     input/PIXIE/Pixie_ChannelMapping.txt
 *     FIRST_SLOT   2          crate slot of module 0 (module = slot - FIRST_SLOT)
 *
 * Map file, whitespace separated, first line is the header:
 *     Module  Channel  Wall  Bar  Side  Index
 *     0       0        B     0    L     BL00
 * Wall: B = neutron wall B (LANA, facing the target), A = neutron wall A
 *       (LANA, behind B), V = WMU VETO wall.
 * Side: L / T -> 0,  R / B -> 1   (LANA left/right, VETO top/bottom).
 * Index is a label only. One crate: the crate number is not used.
 * Hits on channels not in the file go to neither branch (counted in PrintSummary).
 */
class PixieMapping{
	public:
		bool Load(const Config &cfg);
		bool Load(const std::string &path);

		// fill the mapped fields of 'all' (every decoded hit), then copy each
		// mapped hit to lana (walls A, B) or veto; both are cleared first
		void Split(PixieData &all, PixieData &lana, PixieData &veto);
		void PrintSummary() const;

		int firstSlot = 2;

	private:
		struct Entry {
			int detector = PixieData::kUnmapped;
			int bar      = -1;
			int side     = -1;
		};
		static const int kModules  = 16;
		static const int kChannels = 64;	// 16 for Rev F, 32 for Rev H
		static int Index(int module, int channel);

		std::vector<Entry> fTable = std::vector<Entry>(kModules * kChannels);
		int fN = 0;
		long fUnmappedHits = 0;
		std::map<int, long> fUnmappedBySlotCh;	// slot*100 + channel
};

#endif // PIXIEMAPPING_H
