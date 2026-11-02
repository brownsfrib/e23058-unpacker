#ifndef HIRAGEOMETRY_H
#define HIRAGEOMETRY_H

// C/C++ includes
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

// One silicon strip: which detector it belongs to and where it points
struct HiRAChannel {
	int         motherboard = 0;
	int         chip        = 0;
	int         strip       = 0;	// board strip, 0-31
	std::string detCode;		// silicon detector code, e.g. "2344-11"
	double      theta       = 0.0;
	double      dtheta      = 0.0;
	double      phi         = 0.0;
	double      dphi        = 0.0;
};

/**
 * !\brief (motherboard, chip, strip) -> detector code + theta/phi lookup.
 *
 * Record format of the .dat (60 bytes each, written by write_hira_channels):
 *     int32 motherboard, int32 chip, char detCode[16], int32 strip,
 *     double theta, dtheta, phi, dphi
 * detCode is NUL-padded. Record order does not matter.
 */
class HiRAGeometry{
	public:
		static const int kCodeLen = 16;

		bool Load(const std::string &path){
			std::ifstream fin(path.c_str(), std::ios::binary);
			if (!fin){
				std::cerr << "\033[1;31mHiRAGeometry: cannot open " << path << "\033[0m\n";
				return false;
			}
			int32_t mb, chip, strip;
			char    code[kCodeLen];
			double  th, dth, ph, dph;
			fTable.clear();
			while (fin.read((char*)&mb,    sizeof mb)    &&
			       fin.read((char*)&chip,  sizeof chip)  &&
			       fin.read(code,          kCodeLen)     &&
			       fin.read((char*)&strip, sizeof strip) &&
			       fin.read((char*)&th,    sizeof th)    &&
			       fin.read((char*)&dth,   sizeof dth)   &&
			       fin.read((char*)&ph,    sizeof ph)    &&
			       fin.read((char*)&dph,   sizeof dph)){
				HiRAChannel c;
				c.motherboard = mb;
				c.chip        = chip;
				c.strip       = strip;
				c.detCode     = std::string(code, strnlen(code, kCodeLen));
				c.theta = th;  c.dtheta = dth;  c.phi = ph;  c.dphi = dph;
				fTable[Key(mb, chip, strip)] = c;
			}
			std::cout << "HiRAGeometry: loaded " << fTable.size() << " strips from " << path << "\n";
			return !fTable.empty();
		}

		// nullptr if the strip is not in the file
		const HiRAChannel *Get(int motherboard, int chip, int strip) const{
			std::unordered_map<uint32_t, HiRAChannel>::const_iterator it =
				fTable.find(Key(motherboard, chip, strip));
			return (it == fTable.end()) ? nullptr : &it->second;
		}

		int Size() const { return (int)fTable.size(); }

	private:
		static uint32_t Key(int mb, int chip, int strip){
			return ((uint32_t)(mb & 0xff) << 16) | ((uint32_t)(chip & 0xff) << 8) | (uint32_t)(strip & 0xff);
		}
		std::unordered_map<uint32_t, HiRAChannel> fTable;
};

#endif // HIRAGEOMETRY_H
