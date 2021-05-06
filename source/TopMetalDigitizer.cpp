#include "TopMetalDigitizer.h"
#include "CAENDigitizer.h"

#include <typeinfo>
#include <cmath>


TopMetalDigitizer::TopMetalDigitizer(){
	/*
		Creates a digitizer object with default configuration.
		Should in general not be used if actual operation of a digitizer is intended
		(defaults will often not be good for a specific application)
	*/
	nboards = 1;

}

TopMetalDigitizer::TopMetalDigitizer(CaenDigitizerSettings & digitizerSettings){

	nchannels = digitizerSettings.nchannels;
	nboards = digitizerSettings.numberOfBoards;
	nSamplesPerTrigger = digitizerSettings.nSamplesPerTrigger;
	useExternalClock = digitizerSettings.useExternalClock;
	postTriggerFraction = digitizerSettings.postTriggerFraction;
	triggerMode = digitizerSettings.triggerMode;
	triggerThreshold = digitizerSettings.triggerThreshold;
	triggerPolarity  = static_cast<CAEN_DGTZ_PulsePolarity_t> (digitizerSettings.triggerPolarity);
	maxNumberEventsTransferred = digitizerSettings.maxNumberEventsTransferred;
	channelSettings = digitizerSettings.channelSettings;
}

TopMetalDigitizer::~TopMetalDigitizer(){

	err = CAEN_DGTZ_FreeReadoutBuffer(&buffer);
	err = CAEN_DGTZ_CloseDigitizer(boardAddr);
	if (verbose) std::cout << "Closing Digitizer...\t\tStatus: " << err << "\n";
}


CAEN_DGTZ_ErrorCode TopMetalDigitizer::ConfigureDigitizer(){
	/*
		Uses the class settings to connect and configure the physical hardware
	*/

	// Open connection to digitizer
	err = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, 0, 0, 0, &boardAddr);
	if (verbose) std::cout << "Opening communication with digitizer...\t\tStatus: " << err << "\n";
	// Get Board info
	err = CAEN_DGTZ_Reset(boardAddr); 

	err = CAEN_DGTZ_GetInfo(boardAddr, &boardInformation);

	// Clock source for data acquisition
	uint32_t clockRegister;
	err = CAEN_DGTZ_ReadRegister(boardAddr, 0x8100, &clockRegister);
	if((clockRegister/64)%2==0 == useExternalClock) // Check if current state is desired state. If not change bit 6
	{
	  clockRegister += (uint32_t)64; // set bit 6 to 1.
	}
	err = CAEN_DGTZ_WriteRegister(boardAddr, 0x8100, clockRegister);
	if (verbose) std :: cout << "Writing clock source register...\t\tStatus: " << err << "\n";

	// Congifure board with settings
	uint32_t channelMask = 0;
	err = CAEN_DGTZ_SetRecordLength(boardAddr, nSamplesPerTrigger);
	err = CAEN_DGTZ_SetPostTriggerSize(boardAddr, (uint32_t) (postTriggerFraction * 100) );
	err = CAEN_DGTZ_SetMaxNumEventsBLT(boardAddr, maxNumberEventsTransferred);
	err = CAEN_DGTZ_SetAcquisitionMode(boardAddr, CAEN_DGTZ_SW_CONTROLLED);
	err = CAEN_DGTZ_SetIOLevel(boardAddr, CAEN_DGTZ_IOLevel_TTL); // Set the front panel inputs to accept TTL signals
	

	// Set individual channel settings
	for (int i = 0; i < nchannels; ++i)
	{
		uint32_t channelNumber = channelSettings[i].channelNumber;
		err = CAEN_DGTZ_SetChannelDCOffset(boardAddr, channelNumber, channelSettings[i].channelDCOffset);
		err = CAEN_DGTZ_SetChannelPulsePolarity(boardAddr, channelNumber, static_cast<CAEN_DGTZ_PulsePolarity_t> (channelSettings[i].pulsePolarity) ); 
		channelMask += pow(2, channelNumber);
	}
	err = CAEN_DGTZ_SetChannelEnableMask(boardAddr, channelMask);

	if (verbose) std::cout << "Congfigure board settings...\t\tStatus: " << err << "\n";

	// Configure Trigger and acquisition settings depending on the settings file


	switch(triggerMode){
		case Continuous:
			break;
		case SelfTrigger:

			err = CAEN_DGTZ_SetChannelSelfTrigger(boardAddr, CAEN_DGTZ_TRGMODE_ACQ_ONLY, channelMask);
			
			for (int i = 0; i < nchannels; ++i)
			{
				uint32_t channelNumber = channelSettings[i].channelNumber;

				err = CAEN_DGTZ_SetTriggerPolarity(boardAddr, channelNumber, static_cast<CAEN_DGTZ_TriggerPolarity_t> (triggerPolarity));
				if (channelSettings[i].isChannelTrigger) err = CAEN_DGTZ_SetChannelTriggerThreshold(boardAddr, channelNumber, triggerThreshold);
			}

			break;
		case SoftwareTrigger:
			err = CAEN_DGTZ_SetSWTriggerMode(boardAddr, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
			break;
		case ExternalTrigger:
			err = CAEN_DGTZ_SetExtTriggerInputMode(boardAddr, CAEN_DGTZ_TRGMODE_ACQ_ONLY);
			break;
	}
	if (verbose) std::cout << "Configure trigger settings...\t\tStatus: " << err << "\n";
	
	// Allocate buffer memory
	err = CAEN_DGTZ_MallocReadoutBuffer(boardAddr, &buffer, &bufferSize);
	if (verbose) std::cout << "Allocate buffer memory...\t\tStatus: " << err << "\n";
	
	return err;
}

CAEN_DGTZ_ErrorCode TopMetalDigitizer::TransferData(){

	// Readout data
	err = CAEN_DGTZ_ReadData(boardAddr, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer, &readoutSize);
	err = CAEN_DGTZ_GetNumEvents(boardAddr, buffer, readoutSize, &numberEventsRead);

	return err;
}

CAEN_DGTZ_ErrorCode TopMetalDigitizer::StartDataAcquisition(){
	err = CAEN_DGTZ_SWStartAcquisition(boardAddr);
	if (verbose) std::cout << "Start data acquisition...\t\tStatus: " << err << "\n";
	return err;
}

CAEN_DGTZ_ErrorCode TopMetalDigitizer::StopDataAcquisition(){
	err = CAEN_DGTZ_SWStopAcquisition(boardAddr);
	if (verbose) std::cout << "Stop data acquisition...\t\tStatus: " << err << "\n";
	return err;
}

CAEN_DGTZ_ErrorCode TopMetalDigitizer::SendSWTrigger(){
	err = CAEN_DGTZ_SendSWtrigger(boardAddr);
	return err;
}
