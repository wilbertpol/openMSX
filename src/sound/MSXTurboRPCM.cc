// $Id$

#include "MSXTurboRPCM.hh"
#include "MSXCPUInterface.hh"
#include "DACSound.hh"


MSXTurboRPCM::MSXTurboRPCM(MSXConfig::Device *config, const EmuTime &time)
	: MSXDevice(config, time), MSXIODevice(config, time)
{
	short volume = (short)deviceConfig->getParameterAsInt("volume");
	dac = new DACSound(volume, time);
	
	MSXCPUInterface::instance()->register_IO_Out(0xA4, this);
	MSXCPUInterface::instance()->register_IO_Out(0xA5, this);
	MSXCPUInterface::instance()->register_IO_In (0xA4, this);
	MSXCPUInterface::instance()->register_IO_In (0xA5, this);
	
	reset(time);
}

MSXTurboRPCM::~MSXTurboRPCM()
{
	delete dac;
}


void MSXTurboRPCM::reset(const EmuTime &time)
{
	reference = time;
	status = 0;
	dac->reset(time);
}

byte MSXTurboRPCM::readIO(byte port, const EmuTime &time)
{
	byte result;
	switch (port) {
		case 0xA4:
			// bit 0-1  15.75kHz counter
			// bit 2-7  not used
			result = reference.getTicksTill(time) & 0x03;
			break;

		case 0xA5:
			// bit 0   BUFF  0->D/A
			//               1->A/D
			// bit 1   MUTE  mute ALL sound  0->muted
			// bit 2   FILT  filter  0->standard signal
			//                       1->filtered signal
			// bit 3   SEL   select 0->D/A 
			//                      1->Mic/Jack
			// bit 4   SMPL  sample/hold  0->sample
			//                            1->hold
			// bit 5-6       not used
			// bit 7   COMP  comparator result 0->greater
			//                                 1->smaller

			result = getComp() ? 0x80 : 0x00;
			result |= status & 0x1F; 
			break;
	}
	return result;
}

void MSXTurboRPCM::writeIO(byte port, byte value, const EmuTime &time)
{
	switch (port) {
		case 0xA4:
			// While playing: sample value
			//       recording: compare value
			// Resets counter
			reference = time;
			DValue = value;
			
			if ((status & 0x01) == 0) {
				dac->writeDAC(DValue, time);
			}
			break;
		
		case 0xA5:
			// bit 0   BUFF 
			// bit 1   MUTE  mute _all_ sound  0->no sound
			// bit 2   FILT  filter  1->filter on
			// bit 3   SEL   select  1->Mic/Jack  0->D/A
			// bit 4   SMPL  sample/hold  1->hold
			// bit 5-7  not used
			byte change = status ^ value;
			status = value;

			if ((change & 0x01) && ((status & 0x01) == 0)) {
				dac->writeDAC(DValue, time);
			}
			// TODO hardwareMute(status & 0x02);
			// TODO status & 0x08
			if ((change & 0x10) && (status & 0x10)) {
				hold = getSample();
			}
			break;
	}
}


byte MSXTurboRPCM::readSample()
{
	return 0x80;	// TODO  read real sample
}

byte MSXTurboRPCM::getSample()
{
	return (status & 0x04) ? readSample() : 0x80;	// TODO check 0x80
}

bool MSXTurboRPCM::getComp()
{
	// TODO also when D/A ??
	byte sample = (status & 0x10) ? hold : getSample();
	return sample < DValue;	// TODO check
}
