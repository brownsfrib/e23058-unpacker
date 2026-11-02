#include "Pixie.h"

#include <iostream>

Pixie::Pixie() : Detector("Pixie") {}
Pixie::~Pixie() {}

void Pixie::Clear(){
	hits.clear();
}

bool Pixie::Unpack(const Ring::Fragment &frag){
	fFrags++;

	PixieHit h;
	h.sourceID  = frag.sourceID;
	h.timestamp = frag.timestamp;
	// TODO: decode frag.payload (frag.payloadBytes bytes) into h
	hits.push_back(h);

	return true;
}

void Pixie::PrintSummary() const{
	std::cout << "Pixie: " << fFrags << " fragments (not decoded - skeleton)\n";
}
