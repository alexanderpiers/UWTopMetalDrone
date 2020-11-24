#include "TopMetalDroneConfig.h"

// For printing enums
const char * TopMetalReadoutModeChar[] = {
	"FullReadout",
	"SinglePixel"
};

const char * DigitizerTriggerModesChar[] = 
{
	"Continous",
	"HardwareTrigger",
	"SoftwareTrigger"
};

TopMetalDroneConfig::TopMetalDroneConfig(const std::string &configFilename){
	inputFilename = configFilename;
	ReadConfigFile(configFilename);
}

void PrintMissingElement(const char * ele){
	std::cout << FRED("Missing Element: ") << ele << ". Failed to parse config.\n\n";
}

bool TopMetalDroneConfig::ReadConfigFile(const std::string &configFilename){
	/*
		Reads the xml config file and parse the information into the TopMetalDrone Config Class
		Inputs:
			std::string & configFilename - (clearly) the configuration file path/name
		Outputs:
			bool success - flag on if the reading/parsing operation was a success
	*/

	bool parseSuccess = true;
	inputFilename = configFilename;

	std::cout << "Reading Config file:\n";

	// Open XML document
	tinyxml2::XMLDocument configXML;
	tinyxml2::XMLError loadStatus = configXML.LoadFile(configFilename.c_str());
	if (loadStatus != tinyxml2::XML_SUCCESS){
		std::cout << BOLD(FRED("Failed to load the config file: ")) << configFilename << std::endl;
		parseSuccess = false;
	}

	// Parse general settings
	tinyxml2::XMLElement * generalSettingElement = configXML.FirstChildElement("general");
	if( generalSettingElement == 0 ){
		PrintMissingElement("general");
		parseSuccess = false;
	} 

	int intReadOutMode;
	if( generalSettingElement->QueryIntAttribute("readOutMode", &intReadOutMode) != 0 ){
		PrintMissingElement("general:readOutMode");
		parseSuccess = false;
	} 
	readOutMode = static_cast<TopMetalReadoutMode>(intReadOutMode);

	if( generalSettingElement->QueryIntAttribute("numberFramesInReferenceImage", &numberFramesInReferenceImage) != 0 ){
		PrintMissingElement("general:numberFramesInReferenceImage");
		parseSuccess = false;
	}

	// Digitizer settings
	tinyxml2::XMLElement * digitizerSettingElement = configXML.FirstChildElement("digitizer");
	if( digitizerSettingElement == 0 ){
		PrintMissingElement("digitizer");
		parseSuccess = false;
	}

	if( digitizerSettingElement->QueryDoubleAttribute("samplingRate", &digitizerSettings.samplingRate) != 0){
		PrintMissingElement("digitizer:samplingRate");
		parseSuccess = false;
	}

	if( digitizerSettingElement->QueryIntAttribute("nSamplesPerTrigger", &digitizerSettings.nSamplesPerTrigger) != 0){
		PrintMissingElement("digitizer:nSamplesPerTrigger");
		parseSuccess = false;
	}

	if( digitizerSettingElement->QueryDoubleAttribute("postTriggerFraction", &digitizerSettings.postTriggerFraction) != 0){
		PrintMissingElement("digitizer:postTriggerFraction");
		parseSuccess = false;
	}

	if( digitizerSettingElement->QueryIntAttribute("maxNumberEventsTransferred", &digitizerSettings.maxNumberEventsTransferred) != 0) {
		PrintMissingElement("digitizer:maxNumberEventsTransferred");
		parseSuccess = false;
	}

	if( digitizerSettingElement->QueryIntAttribute("useExternalClock", &digitizerSettings.useExternalClock) != 0){
		PrintMissingElement("digitizer:useExternalClock");
		parseSuccess = false;
	}

	if ( digitizerSettingElement->QueryIntAttribute("triggerPolarity", &digitizerSettings.triggerPolarity) !=0){
		PrintMissingElement("digitizer:triggerPolarity");
		parseSuccess = false;
	}

	if( digitizerSettingElement->QueryAttribute("triggerThreshold", &digitizerSettings.triggerThreshold) != 0){
		PrintMissingElement("digitizer:triggerThreshold");
		parseSuccess = false;
	}

	int intTriggerMode;
	if( digitizerSettingElement->QueryIntAttribute("triggerMode", &intTriggerMode) != 0){
		PrintMissingElement("digitizer:triggerMode");
		parseSuccess = false;
	}
	digitizerSettings.triggerMode = static_cast<DigitizerTriggerModes>(intTriggerMode);

	int intDCOffset = 0;
	if( digitizerSettingElement->QueryIntAttribute("dcOffset", &intDCOffset) != 0){
		PrintMissingElement("digitizer:dcOffset");
		std::cout << "Falling back to default value of 0x0000\n";
	}
	digitizerSettings.acquisitionDCOffset = static_cast<uint16_t> (intDCOffset);


	if( digitizerSettingElement->QueryIntAttribute("numberOfBoards", &digitizerSettings.numberOfBoards) != 0){
		PrintMissingElement("digitizer:numberOfBoards");
		parseSuccess = false;
	}


	// FPGA settings
	tinyxml2::XMLElement * fpgaSettingElement = configXML.FirstChildElement("fpga");
	if( fpgaSettingElement == 0 ){
		PrintMissingElement("fpga");
		parseSuccess = false;
	}

	if( fpgaSettingElement->QueryBoolAttribute("stopAddressMode", &fpgaSettings.stopAddressEnable) != 0){
		PrintMissingElement("fpga:stopAddressMode");
		parseSuccess = false;
	}

	if( fpgaSettingElement->QueryIntAttribute("stopAddress", &fpgaSettings.stopAddress) != 0){
		PrintMissingElement("fpga:stopAddress");
		parseSuccess = false;
	}

	// output settings
	tinyxml2::XMLElement * outputSettingElement = configXML.FirstChildElement("output");
	if( outputSettingElement == 0 ){
		PrintMissingElement("output");
		parseSuccess = false;
	}

	if( outputSettingElement->Attribute("filename") ){
		outputFilename = outputSettingElement->Attribute("filename");
	}
	else{
		PrintMissingElement("output:filename");
		parseSuccess = false;
	}

	if( outputSettingElement->QueryIntAttribute("waveformDownsample", &waveformDownsample) != 0){
		PrintMissingElement("output:waveformDownsample");
		parseSuccess = false;
	}




	return parseSuccess;
}

void TopMetalDroneConfig::PrintConfigSettings () const {

	std::cout << "Top Metal II- Control Settings\n";

	// General Settings
	std::cout << "General Settings\n";
	std::cout << "\tConfiguration Filename: " << inputFilename 						  << "\n";
	std::cout << "\tReadout Mode: " 		  << TopMetalReadoutModeChar[readOutMode] << "\n";
	std::cout << "\tNumber of Frames in Reference Image: " << numberFramesInReferenceImage << "\n";

	// Digitizer Settings
	std::cout << "Digitizer Settings\n";
	std::cout << "\tSampling Rate (MHz): " 	  	 << digitizerSettings.samplingRate 		            			 << "\n";
	std::cout << "\tNumber of Samples Per Trigger: " << digitizerSettings.nSamplesPerTrigger 					 << "\n";
	std::cout << "\tFraction of Waveform Post Trigger: " << digitizerSettings.postTriggerFraction 					 << "\n";
	std::cout << "\tTrigger Mode: "                  << DigitizerTriggerModesChar[digitizerSettings.triggerMode]                     << "\n";
	std::cout << "\tTrigger Threshold: "             << digitizerSettings.triggerThreshold   					 << "\n";
	std::cout << "\tTrigger Polarity: "              << digitizerSettings.triggerPolarity 						 << "\n";
	std::cout << "\tMaximum Number of Events Transferred per Read: " << digitizerSettings.maxNumberEventsTransferred                 << "\n";
	std::cout << "\tDC Offset of Dynamic Voltage Range (Negative Direction): " << digitizerSettings.acquisitionDCOffset               << "\n";
	std::cout << "\tNumber of Digitizer Boards: "    << digitizerSettings.numberOfBoards 	 					 << "\n";
	
	// FPGA Settings
	std::cout << "FPGA Settings\n";
	std::cout << "\tFPGA Stop Address Enabled: " << fpgaSettings.stopAddressEnable << "\n";
	std::cout << "\tFPGA Stop Address Value: "	 << fpgaSettings.stopAddress       << "\n";

	// Output settings
	std::cout << "Output Settings\n";
	std::cout << "\tOutput filename: " << outputFilename << "\n";
	std::cout << "\tWaveform Downsampling Factor: " << waveformDownsample << "\n";
}

