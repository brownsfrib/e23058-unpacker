#ifndef DETECTOR_H
#define DETECTOR_H

// C/C++ includes
#include <string>

// Unpacker includes
#include "RingItem.h"

/**
 * !\brief Base class every detector unpacker derives from.
 *
 * To add a detector:
 *   1. derive from Detector, implement Clear() and Unpack()
 *   2. add the class name to 'classes' in compile.sh
 *   3. in your program: unpacker.Register(<sourceID>, &yourDetector);
 *
 * Per physics event the Unpacker calls Clear() on every registered
 * detector, then Unpack() once for EACH fragment carrying one of that
 * detector's source IDs. A detector can therefore get zero, one, or many
 * fragments per event (Pixie: one fragment per hit), so Unpack() should
 * append to the event's data, not overwrite it.
 */
class Detector{
	public:
		explicit Detector(const std::string &name) : fName(name) {}
		virtual ~Detector() {}

		const std::string &Name() const { return fName; }

		// start of every physics event: empty this event's data
		virtual void Clear() = 0;

		// one fragment of this detector's data; return false if it was not
		// understood (the Unpacker counts failures per source ID)
		virtual bool Unpack(const Ring::Fragment &frag) = 0;

		// optional hooks
		virtual void BeginRun(int /*runNumber*/) {}
		virtual void PrintSummary() const {}

		// set by the Unpacker: true if >= 1 fragment unpacked OK this event
		bool fired = false;

	protected:
		std::string fName;
};

#endif // DETECTOR_H
