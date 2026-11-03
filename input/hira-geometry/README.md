File usage

File				Read by			Purpose
hira_config.json 		builder			slots, detector codes, thickness, survey block per telescope (alignment)
.xlsx				builder			fiducials and beam axis gives all angles	
ADC/TDCMapping.csv		builder			copied into HiRA_csi.csv (later also TDCMap)
HiRA_channel_theta_phi.txt	calbirations/toBinary	standard text file, convert to .dat binary file (performance)
HiRA_channel_theta_phi.dat	HiRAGeometry::Load	strip -> telescope face, CsI detectors, angles
HiRA_pixels.csv			LoadPixesl (to write)	final theta/phi for each track after reconstruction checker
HiRA_strips.csv			nothing			just a reference of the E-strip table
HiRA_strip_calibration.txt	HiRACalibration		raw E data -> MeV (need this for front/back energy pairing)
ConfigHira.dat			HiRAConfig		thresholds, pairing, disabled channels, etc.



