#ifndef UNPACKER_H
#define UNPACKER_H

// C/C++ includes
#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

// Unpacker includes
#include "RingItem.h"
#include "Detector.h"

/**
 * !\brief Reads an NSCLDAQ .evt file and routes each fragment to the
 *         Detector registered for its source ID.
 *
 *     Unpacker unpacker;
 *     FROG frog;
 *     unpacker.Register(3, &frog);
 *     unpacker.Open(path);
 *     while (unpacker.NextEvent()){
 *         for (auto &hit : frog.hits) { ... }
 *     }
 *     unpacker.PrintSummary();
 *
 * Built and unbuilt files both work: if a PHYSICS_EVENT body is not a
 * valid built body, it is treated as one fragment whose source ID comes
 * from the ring item's body header.
 */
class Unpacker{
	public:
		Unpacker();
		~Unpacker();

		bool Open(const std::string &path);

		// detectors are not owned; one detector may own several source IDs
		void Register(uint32_t sourceID, Detector *det);

		// advance to the next PHYSICS_EVENT and unpack it; false at end of file
		bool NextEvent();

		void PrintSummary() const;

		// fragments of the current event (outer level), e.g. for timestamps
		const std::vector<Ring::Fragment> &Fragments() const { return fFrags; }

		// run / event info
		int      runNumber      = -1;
		uint64_t eventTimestamp = 0;	// body-header timestamp of the built event
		long     physicsEvents  = 0;

	private:
		bool ReadItem(uint32_t &type, uint32_t &sourceID, uint64_t &ts,
		              const uint8_t *&body, size_t &nbytes);
		void Dispatch(const Ring::Fragment &frag);

		std::ifstream                 fFile;
		std::vector<uint8_t>          fBuf;
		std::vector<Ring::Fragment>   fFrags;

		std::map<uint32_t, Detector*> fDetectors;	// source ID -> detector
		std::set<Detector*>           fUnique;

		// bookkeeping for PrintSummary()
		std::map<uint32_t, long> fItemTypes;
		std::map<uint32_t, long> fFragsBySID;
		std::map<uint32_t, long> fFailedBySID;
		std::map<uint32_t, long> fUnhandledBySID;
		bool fTruncated = false;
};

#endif // UNPACKER_H
