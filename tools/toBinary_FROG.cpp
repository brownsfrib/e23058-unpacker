// toBinary_FROG.cpp
// Converts the FROG channel table (from map-channels.py) into the binary .dat
// read by FROGGeometry (CHANNEL_TABLE in FROGConfig.dat).
//
// Usage: bin/toBinary_FROG input/FROG/geometry/channel_theta_phi.txt input/FROG/FROG_channel_theta_phi.dat
//
// Text input, one channel per line, '#' = comment:
//   board.channel barrel face sipm theta dtheta phi dphi      (degrees, d* = half-widths)
// Binary record (40 bytes, must match FROGGeometry::Load):
//   int32 board, channel; double theta, dtheta, phi, dphi
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char **argv){
	const char *in  = argc > 1 ? argv[1] : "channel_theta_phi.txt";
	const char *out = argc > 2 ? argv[2] : "FROG_channel_theta_phi.dat";

	std::ifstream fin(in);
	std::ofstream fout(out, std::ios::binary);
	if (!fin || !fout){
		std::fprintf(stderr, "Cannot open %s or %s\n", in, out);
		return 1;
	}
	std::string line;
	int n = 0, lineNo = 0;
	while (std::getline(fin, line)){
		lineNo++;
		size_t first = line.find_first_not_of(" \t\r");
		if (first == std::string::npos || line[first] == '#'){
			continue;
		}
		int32_t board, channel;
		int barrel, face, sipm;
		char dot;
		double a[4];		// theta, dtheta, phi, dphi
		std::istringstream ss(line);
		if (!(ss >> board >> dot >> channel >> barrel >> face >> sipm >> a[0] >> a[1] >> a[2] >> a[3]) || dot != '.'){
			std::fprintf(stderr, "line %d: could not parse, skipped\n", lineNo);
			continue;
		}
		fout.write((const char*)&board,   sizeof board);
		fout.write((const char*)&channel, sizeof channel);
		fout.write((const char*)a,        sizeof a);
		n++;
	}
	std::printf("Wrote %d channels to %s\n", n, out);
	return 0;
}
