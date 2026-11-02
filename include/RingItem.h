#ifndef RINGITEM_H
#define RINGITEM_H

// C/C++ includes
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <vector>

/**
 * !\brief NSCLDAQ ring-item and event-builder (glom) plumbing.
 *
 * Nothing in here knows about any detector. It turns bytes into Fragments,
 * and the Unpacker hands each Fragment to whichever Detector owns its
 * source ID.
 *
 * Built (glommed) PHYSICS_EVENT body:
 *
 *     uint32 totalBytes                    (inclusive)
 *     fragment 0:
 *         uint64 timestamp
 *         uint32 sourceID
 *         uint32 payloadBytes
 *         uint32 barrierType
 *         <ring item>  uint32 size, uint32 type, body header, payload
 *     fragment 1: ...
 *
 * A fragment's payload can itself be a built body (FROG's sub-event builder
 * does this), so SplitFragments() is reused at every level.
 */
namespace Ring {

	enum ItemType : uint32_t {
		BEGIN_RUN           = 1,
		END_RUN             = 2,
		PAUSE_RUN           = 3,
		RESUME_RUN          = 4,
		ABNORMAL_ENDRUN     = 5,
		PACKET_TYPES        = 10,
		MONITORED_VARIABLES = 11,
		RING_FORMAT         = 12,
		PERIODIC_SCALERS    = 20,
		PHYSICS_EVENT       = 30,
		PHYSICS_EVENT_COUNT = 31,
		EVB_FRAGMENT        = 40,
		EVB_UNKNOWN_PAYLOAD = 41,
		EVB_GLOM_INFO       = 42
	};

	// little-endian read at an arbitrary (possibly misaligned) address
	template<class T> inline T peek(const uint8_t *p){
		T t;
		std::memcpy(&t, p, sizeof(T));
		return t;
	}

	// One piece of one source's data. 'payload' points into the Unpacker's
	// buffer and is only valid until the next call to Unpacker::NextEvent().
	struct Fragment {
		uint64_t       timestamp    = 0;
		uint32_t       sourceID     = 0;
		uint32_t       barrier      = 0;
		uint32_t       ringType     = 0;       // type of the wrapped ring item (30 for data)
		const uint8_t *payload      = nullptr; // after the wrapped item's body header
		size_t         payloadBytes = 0;
	};

	// Split a built body into its fragments. Returns true only if the
	// fragments tile the body exactly, so it doubles as a "is this a built
	// body?" test: false means treat the bytes as a single raw payload.
	bool SplitFragments(const uint8_t *body, size_t nbytes, std::vector<Fragment> &out);

	// Size of a ring item's body header (always at least the 4-byte size word).
	inline size_t BodyHeaderBytes(const uint8_t *afterRingHeader){
		uint32_t bh = peek<uint32_t>(afterRingHeader);
		return (bh > 4) ? bh : 4;
	}
}

#endif // RINGITEM_H
