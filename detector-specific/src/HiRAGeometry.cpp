#include "HiRAGeometry.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

static const double kNone = -10000;

// numbers from one CSV line; false for header/comment lines
static bool csvNumbers(const std::string &line, std::vector<double> &v){
	v.clear();
	if (line.empty() || line[0] < '0' || line[0] > '9'){
		return false;
	}
	std::stringstream ss(line);
	std::string field;
	while (std::getline(ss, field, ',')){
		v.push_back(std::atof(field.c_str()));
	}
	return true;
}

bool HiRAGeometry::Load(const Config &cfg){
	bool ok = LoadStrips(cfg.GetString("geometry", "STRIP_TABLE"));
	ok &= LoadCsI(cfg.GetString("geometry", "CSI_TABLE"));
	ok &= LoadPixels(cfg.GetString("geometry", "PIXEL_TABLE"));
	return ok;
}

// 108-byte records written by toBinary_HIRA:
//   int32 motherboard, chip; char detCode[16]; int32 strip, telescope, face, csiA, csiB;
//   double theta, thetaMin, thetaMax, dtheta, phi, phiMin, phiMax, dphi
bool HiRAGeometry::LoadStrips(const std::string &path){
	std::ifstream fin(path.c_str(), std::ios::binary);
	if (!fin){
		std::cerr << "\033[1;31mHiRAGeometry: cannot open strip table '" << path << "'\033[0m\n";
		return false;
	}
	int32_t a[2], b[5];
	char    code[16];
	double  ang[8];
	fStrips.clear();
	while (fin.read((char*)a, sizeof a) && fin.read(code, sizeof code) &&
	       fin.read((char*)b, sizeof b) && fin.read((char*)ang, sizeof ang)){
		HiRAChannel c;
		c.motherboard = a[0];  c.chip = a[1];
		c.detCode     = std::string(code, strnlen(code, sizeof code));
		c.strip = b[0];  c.telescope = b[1];  c.face = b[2];  c.csi[0] = b[3];  c.csi[1] = b[4];
		c.theta = ang[0];  c.thetaMin = ang[1];  c.thetaMax = ang[2];  c.dtheta = ang[3];
		c.phi   = ang[4];  c.phiMin   = ang[5];  c.phiMax   = ang[6];  c.dphi   = ang[7];
		fStrips[Key(c.motherboard, c.chip, c.strip)] = c;
	}
	std::cout << "HiRAGeometry: loaded " << fStrips.size() << " strips from " << path << "\n";
	return !fStrips.empty();
}

// csi,telescope,quadrant,adc_module,adc_channel,tdc_channel,theta,theta_min,theta_max,dtheta,phi,phi_min,phi_max,dphi,...
bool HiRAGeometry::LoadCsI(const std::string &path){
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mHiRAGeometry: cannot open CsI table '" << path << "'\033[0m\n";
		return false;
	}
	fCsI.assign(kNCsI, HiRACsI());
	std::string line;
	std::vector<double> v;
	int n = 0;
	while (std::getline(fin, line)){
		if (!csvNumbers(line, v) || v.size() < 14 || v[0] < 0 || v[0] >= kNCsI){
			continue;
		}
		HiRACsI &c = fCsI[(int)v[0]];
		c.valid = true;
		c.id = (int)v[0];  c.telescope = (int)v[1];  c.quadrant = (int)v[2];
		c.adcModule = (int)v[3];  c.adcChannel = (int)v[4];  c.tdcChannel = (int)v[5];
		c.theta = v[6];   c.thetaMin = v[7];   c.thetaMax = v[8];   c.dtheta = v[9];
		c.phi   = v[10];  c.phiMin   = v[11];  c.phiMax   = v[12];  c.dphi   = v[13];
		n++;
	}
	std::cout << "HiRAGeometry: loaded " << n << " CsI from " << path << "\n";
	return n > 0;
}

// telescope,ef,eb,csi,theta,dtheta,phi,dphi,...
bool HiRAGeometry::LoadPixels(const std::string &path){
	std::ifstream fin(path.c_str());
	if (!fin){
		std::cerr << "\033[1;31mHiRAGeometry: cannot open pixel table '" << path << "'\033[0m\n";
		return false;
	}
	fPix.assign(kNTele * kNStrips * kNStrips, HiRAPixel());
	std::string line;
	std::vector<double> v;
	int n = 0;
	while (std::getline(fin, line)){
		if (!csvNumbers(line, v) || v.size() < 8){
			continue;
		}
		int t = (int)v[0], ef = (int)v[1], eb = (int)v[2];
		if (t < 0 || t >= kNTele || ef < 0 || ef >= kNStrips || eb < 0 || eb >= kNStrips){
			continue;
		}
		HiRAPixel &p = fPix[(t * kNStrips + ef) * kNStrips + eb];
		p.valid = true;
		p.csi = (int)v[3];
		p.theta = v[4];  p.dtheta = v[5];  p.phi = v[6];  p.dphi = v[7];
		n++;
	}
	std::cout << "HiRAGeometry: loaded " << n << " pixels from " << path << "\n";
	return n > 0;
}

const HiRAChannel *HiRAGeometry::Get(int mb, int chip, int strip) const{
	std::unordered_map<uint32_t, HiRAChannel>::const_iterator it = fStrips.find(Key(mb, chip, strip));
	return (it == fStrips.end()) ? nullptr : &it->second;
}

const HiRACsI *HiRAGeometry::GetCsI(int id) const{
	if (id < 0 || id >= (int)fCsI.size() || !fCsI[id].valid){
		return nullptr;
	}
	return &fCsI[id];
}

const HiRAPixel *HiRAGeometry::GetPixel(int t, int ef, int eb) const{
	if (fPix.empty() || t < 0 || t >= kNTele || ef < 0 || ef >= kNStrips || eb < 0 || eb >= kNStrips){
		return nullptr;
	}
	const HiRAPixel &p = fPix[(t * kNStrips + ef) * kNStrips + eb];
	return p.valid ? &p : nullptr;
}

void HiRAGeometry::Map(const HiRA &r, HiRAData &d){
	d.Clear();

	// ---- strips -> front/back blocks
	for (size_t i = 0; i < r.strips.size(); i++){
		const HINPStrip &s = r.strips[i];
		const HiRAChannel *g = Get(s.motherboard, s.chip, s.strip);
		if (!g){
			fUnmapped++;
			continue;
		}
		bool front = (g->face == 0);
		(front ? d.fEFnumtel    : d.fEBnumtel   ).push_back(g->telescope);
		(front ? d.fEFnumstrip  : d.fEBnumstrip ).push_back(s.strip);
		(front ? d.fEFmb        : d.fEBmb       ).push_back(s.motherboard);
		(front ? d.fEFchip      : d.fEBchip     ).push_back(s.chip);
		(front ? d.fEFEnergyHi  : d.fEBEnergyHi ).push_back(s.highGain);
		(front ? d.fEFEnergyLo  : d.fEBEnergyLo ).push_back(s.lowGain);
		(front ? d.fEFTime      : d.fEBTime     ).push_back(s.time);
		(front ? d.fEFEnergyCal : d.fEBEnergyCal).push_back(kNone);
	}
	d.fEFmulti = (Int_t)d.fEFnumtel.size();
	d.fEBmulti = (Int_t)d.fEBnumtel.size();

	// ---- CsI: every crystal the ADCs reported (HiRAPhysics applies the threshold)
	for (int id = 0; id < (int)fCsI.size(); id++){
		const HiRACsI &c = fCsI[id];
		if (!c.valid || c.adcModule < 0 || c.adcChannel < 0 || c.adcChannel >= V785::kChannels){
			continue;
		}
		const V785 &adc = (c.adcModule == 0) ? r.adc1 : r.adc2;
		if (!adc.has[c.adcChannel]){
			continue;
		}
		double time = kNone;
		for (size_t k = 0; k < r.tdc.hits.size(); k++){
			const V1190Hit &h = r.tdc.hits[k];
			if (h.channel == c.tdcChannel && !h.trailing){
				time = h.time;
				break;
			}
		}
		d.fCsInumtel.push_back(c.telescope);
		d.fCsInumcsi.push_back(c.quadrant);
		d.fCsIRawADC.push_back(adc.value[c.adcChannel]);
		d.fCsITime.push_back(time);
	}
	d.fCsImulti = (Int_t)d.fCsInumtel.size();
}

void HiRAGeometry::PrintSummary() const{
	if (fUnmapped){
		std::cout << "  \033[1;33mHiRAGeometry: " << fUnmapped << " strip hits not in the strip table\033[0m\n";
	}
}
