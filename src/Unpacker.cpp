#include "Unpacker.h"

#include <iostream>

Unpacker::Unpacker(){}
Unpacker::~Unpacker(){
	if (fFile.is_open()){
		fFile.close();
	}
}

bool Unpacker::Open(const std::string &path){
	fFile.open(path.c_str(), std::ios::binary);
	if (!fFile.good()){
		std::cerr << "\033[1;31mUnpacker: cannot open " << path << "\033[0m\n";
		return false;
	}
	return true;
}

void Unpacker::Register(uint32_t sourceID, Detector *det){
	fDetectors[sourceID] = det;
	fUnique.insert(det);
}

// Read one ring item into fBuf. body/nbytes point past the body header.
bool Unpacker::ReadItem(uint32_t &type, uint32_t &sourceID, uint64_t &ts,
                        const uint8_t *&body, size_t &nbytes){
	uint32_t hdr[2];
	fFile.read((char*)hdr, 8);
	if (fFile.gcount() != 8){
		return false;			// clean end of file
	}
	uint32_t size = hdr[0];
	type = hdr[1];
	if (size < 12 || size > (64u << 20)){
		std::cerr << "\033[1;31mUnpacker: implausible ring item size " << size
		          << " - stopping.\033[0m\n";
		fTruncated = true;
		return false;
	}

	fBuf.resize(size - 8);
	fFile.read((char*)fBuf.data(), size - 8);
	if ((size_t)fFile.gcount() != size - 8){
		fTruncated = true;		// last item cut off (file still being written?)
		return false;
	}

	sourceID = 0;
	ts = 0;
	uint32_t bh = Ring::peek<uint32_t>(fBuf.data());
	if (bh == 20 && fBuf.size() >= 20){
		ts       = Ring::peek<uint64_t>(fBuf.data() + 4);
		sourceID = Ring::peek<uint32_t>(fBuf.data() + 12);
	}
	size_t skip = Ring::BodyHeaderBytes(fBuf.data());
	if (skip > fBuf.size()){
		fTruncated = true;
		return false;
	}
	body   = fBuf.data() + skip;
	nbytes = fBuf.size() - skip;
	return true;
}

void Unpacker::Dispatch(const Ring::Fragment &frag){
	fFragsBySID[frag.sourceID]++;
	std::map<uint32_t, Detector*>::iterator it = fDetectors.find(frag.sourceID);
	if (it == fDetectors.end()){
		fUnhandledBySID[frag.sourceID]++;
		return;
	}
	if (frag.ringType != Ring::PHYSICS_EVENT){
		return;				// e.g. a state change that rode along in a fragment
	}
	if (it->second->Unpack(frag)){
		it->second->fired = true;
	}
	else {
		fFailedBySID[frag.sourceID]++;
	}
}

bool Unpacker::NextEvent(){
	uint32_t type, sid;
	uint64_t ts;
	const uint8_t *body;
	size_t nbytes;

	while (ReadItem(type, sid, ts, body, nbytes)){
		fItemTypes[type]++;

		if (type == Ring::BEGIN_RUN){
			// one BEGIN_RUN arrives per source; keep the first run number
			if (runNumber < 0 && nbytes >= 4){
				runNumber = (int)Ring::peek<uint32_t>(body);
				for (std::set<Detector*>::iterator d = fUnique.begin(); d != fUnique.end(); ++d){
					(*d)->BeginRun(runNumber);
				}
			}
			continue;
		}
		if (type != Ring::PHYSICS_EVENT){
			continue;			// scalers, glom info, documentation, ...
		}

		for (std::set<Detector*>::iterator d = fUnique.begin(); d != fUnique.end(); ++d){
			(*d)->Clear();
			(*d)->fired = false;
		}
		eventTimestamp = ts;

		if (!Ring::SplitFragments(body, nbytes, fFrags)){
			// not built: the whole body is one source's payload
			Ring::Fragment f;
			f.timestamp    = ts;
			f.sourceID     = sid;
			f.ringType     = Ring::PHYSICS_EVENT;
			f.payload      = body;
			f.payloadBytes = nbytes;
			fFrags.assign(1, f);
		}
		for (size_t i = 0; i < fFrags.size(); i++){
			Dispatch(fFrags[i]);
		}

		physicsEvents++;
		return true;
	}
	return false;
}

void Unpacker::PrintSummary() const{
	std::cout << "\n\033[1;36m==== Unpacker summary ====\033[0m\n";
	std::cout << "run " << runNumber << "   physics events " << physicsEvents << "\n";
	if (fTruncated){
		std::cout << "\033[1;33m(file ended mid-item - last item ignored)\033[0m\n";
	}

	std::cout << "ring item types seen:";
	for (std::map<uint32_t, long>::const_iterator it = fItemTypes.begin(); it != fItemTypes.end(); ++it){
		std::cout << "  " << it->first << ":" << it->second;
	}
	std::cout << "\n";

	std::cout << "fragments by source ID:\n";
	for (std::map<uint32_t, long>::const_iterator it = fFragsBySID.begin(); it != fFragsBySID.end(); ++it){
		std::map<uint32_t, Detector*>::const_iterator d = fDetectors.find(it->first);
		std::cout << "  sid " << it->first << "  " << it->second << " fragments  -> ";
		if (d == fDetectors.end()){
			std::cout << "\033[1;33mno detector registered\033[0m\n";
			continue;
		}
		std::map<uint32_t, long>::const_iterator f = fFailedBySID.find(it->first);
		long nfail = (f == fFailedBySID.end()) ? 0 : f->second;
		std::cout << d->second->Name();
		if (nfail){
			std::cout << "  \033[1;31m(" << nfail << " failed)\033[0m";
		}
		std::cout << "\n";
	}

	for (std::set<Detector*>::const_iterator d = fUnique.begin(); d != fUnique.end(); ++d){
		(*d)->PrintSummary();
	}
}
