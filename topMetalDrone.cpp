// Standard headers
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

// Utility headers
#include "tinyxml2.h"
#include "CLI11.hpp"

//Serial communication headers
#include <fcntl.h> //file controls
#include <termios.h> //terminal control
#include <unistd.h> //write, read and close

// Custom headers
#include "TopMetalDroneConfig.h"
#include "TopMetalDigitizer.h"

// Define constants
const int npx = 72;
const int npy = 72;

int main(int argc, char const *argv[])
{
	

	CLI::App topMetalDroneParser{"UW Top Metal II- Drone Controller"};

	// command line input variables
	std::string infile  = "topMetalConfig.xml";
	std::string outfile = "";

	// define command line arguments
	topMetalDroneParser.add_option("-c, --config", infile, "Top Metal Drone Config file (xml)")->check(CLI::ExistingFile);
	topMetalDroneParser.add_option("-o, --output", outfile, "Output file name");

	// parse command line arguments
	try{
		topMetalDroneParser.parse(argc, argv);
	}
	catch (const CLI::ParseError &e) {
		return topMetalDroneParser.exit(e);
	}

	// start controller
	std::cout << "Starting UW Top Metal II- Drone Controller\n";
	TopMetalDroneConfig config;
	bool configSucces = config.ReadConfigFile(infile);

	// Command line output file (if provided) supercedes config output filename
	if(!outfile.empty()) config.SetOutputFilename(outfile);


	if(configSucces) config.PrintConfigSettings();
	//fpga settings	
    TopMetalFPGASettings test = config.GetTopMetalFPGASettings();
	
	//Setup Serial Port to communicate with FPGA
	int serial_port = open("/dev/ttyUSB2", O_RDWR);
	if (serial_port < 0) {
		std::printf("Error opening serial port to FPGA");
	}
	struct termios tty;

	if(tcgetattr(serial_port, &tty) != 0) {
    printf("Error from tcgetattr");
	}
	tty.c_cflag &=CSIZE;
	tty.c_cflag |= CS8;
	cfsetispeed(&tty, B9600);
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		printf("Error from tcsetattr");
	}

	// Create and communicate with digitizer
	std::printf("Connect and configure digitizer....\n");
	CaenDigitizerSettings digitizerSettings = config.GetDigitizerSettings();
	TopMetalDigitizer digitizer(digitizerSettings);
	digitizer.SetVerboseLevel(1);
	digitizer.ConfigureDigitizer();
	digitizer.StartDataAcquisition();
 	/*
	  Compute median and mad image if necessary
	*/

	// Define array size
	double medianImage[npx][npy], madImage[npx][npy];
	uint16_t * referenceImageRaw = new uint16_t[npx * npy * config.GetNumberFramesInReferenceImage()];
	delete referenceImageRaw;
	
	int eventTransferredCounter = 0;
	// Open file handler
	std::ofstream wf(config.GetOutputFilename(), std::ios::out | std::ios::binary);
	if(!wf) {
	      std::cout << "Cannot open file!" << std::endl;
	      return 1;
	   }
	// Get event info
	CAEN_DGTZ_EventInfo_t eventInfo;
	while(eventTransferredCounter < config.GetDigitizerSettings().maxNumberEventsTransferred){

		// Depending on trigger type, send SW trigger or not
		if(config.GetDigitizerSettings().triggerMode == SoftwareTrigger) digitizer.SendSWTrigger();

		// usleep(20000);
		digitizer.TransferData();
		eventTransferredCounter += digitizer.GetNumberOfEventsRead();

		for (int i = 0; i < digitizer.GetNumberOfEventsRead(); ++i)
		{
			char * evtptr = digitizer.GetEventPtr();
			void * eventBuffer = digitizer.GetEventBuffer();
			
			CAEN_DGTZ_GetEventInfo(digitizer.GetBoardAddress(), digitizer.GetBuffer(), digitizer.GetReadoutSize(), i, &eventInfo, &evtptr);
			std::cout << "EventCounter: " << eventInfo.EventCounter << "\t";
			CAEN_DGTZ_DecodeEvent(digitizer.GetBoardAddress(), evtptr, &eventBuffer);
			CAEN_DGTZ_UINT16_EVENT_t * test = static_cast<	CAEN_DGTZ_UINT16_EVENT_t * >(eventBuffer);
			int waveformDownsamplingRate = config.GetWaveformDownsamplingRate();
			

			// Write values to file
			std::cout << "Mean ADU ";
			for (int i = 0; i < digitizer.GetNumberOfChannels(); ++i)
			{
				int channelNumber = digitizerSettings.channelSettings[i].channelNumber;
				wf.write((char *)test->DataChannel[channelNumber], sizeof(uint16_t)*test->ChSize[0] );


				if(digitizer.GetVerboseLevel() > 0){
					double mean = 0;

					for(int j=0; j < test->ChSize[channelNumber]; j++){
						if (j % waveformDownsamplingRate == 0)
						{
							mean += test->DataChannel[channelNumber][j];
						}
						
					}
					mean /= (test->ChSize[channelNumber] / waveformDownsamplingRate);
					std::cout << "Channel " << channelNumber << ": " << mean << "\t";	

				}
			}


			std::cout << "\n";
	        CAEN_DGTZ_FreeEvent(digitizer.GetBoardAddress(), &eventBuffer);
		}
		
	}
	wf.close();
	
	std::cout << "Number of Events: " << digitizer.GetNumberOfEventsRead() << std::endl;
	

	return 0;
}
