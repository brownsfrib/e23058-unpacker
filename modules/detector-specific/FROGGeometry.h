#ifndef FROGGEOMETRY_H
#define FROGGEOMETRY_H

// C/C++ includes
#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

struct FrogAngles{
	bool valid 	= false; // channel not in file
	double theta 	= 0.0;
	double dtheta	= 0.0;
	double phi	= 0.0;
	double dphi	= 0.0;
};

class FROGGeometry{
	public: 
		bool Load(const std::string& path){
			std::ifstream fileIn(path.c_str(), std::ios::binary);
			if (!fileIn){
				std::string msg = "\033[1;31mFROGGeometry: cannot open " + path + "\033[0m\n";
				throw std::runtime_error(msg);
			}
			// Global lookup data across all A5202s
			int32_t b;
			int32_t c;

			// Angular data
			double th; 
			double dth;
			double ph;
			double dph;

			fN = 0; // "lines" read
			while (
				fileIn.read((char*)&b, sizeof b)
				&& fileIn.read((char*)&c, sizeof c)
				&& fileIn.read((char*)&th, sizeof th)
				&& fileIn.read((char*)&dth, sizeof dth)
				&& fileIn.read((char*)&ph, sizeof ph)
				&& fileIn.read((char*)&dph, sizeof dph)
			){
				if ((b < 0) || (b >= kBoards) || (c < 0) || (c >= kChannels)){
					std::cerr << "FROGGeometry::Load : skipping board " << b << " channel " << c << '\n';
					continue;
				}
				FrogAngles &angles 	= fTable[b*kChannels + c];
				angles.valid 		= true;
				angles.theta 		= th;
				angles.dtheta 		= dth;
				angles.phi		= ph;
				angles.dphi		= dph;
				fN++;
			}
			std::cout << "FROGGeometry: loaded " << fN << " channels from " << path << ".\n";
			return (fN > 0);
		}

		// Global lookup method
		const FrogAngles& Get(int board, int channel) const {
			if ((board < 0) || (board >= kBoards) || (channel < 0) || (channel >= kChannels)){
				return fNone;
			}
			return fTable[board*kChannels + channel];
		};

		int Size() const{
			return fN;
		}

	private: // members yayyyy
		static const int kBoards 	= 256; 
		static const int kChannels 	= 64;
		std::vector<FrogAngles> fTable = std::vector<FrogAngles>(kBoards*kChannels);
		FrogAngles fNone;
		int fN = 0;
};
#endif
