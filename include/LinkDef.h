// ROOT dictionary for the classes written to the E23058 tree.
// Add a line here (and the header in CMakeLists.txt) for every new data class.
#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class FROGData+;
#pragma link C++ class HiRAData+;
#pragma link C++ class TDCData+;
#pragma link C++ class std::map<std::string, double>+;
#endif
