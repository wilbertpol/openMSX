// $Id$

#include "VDPIODelay.hh"
#include "MSXCPU.hh"
#include "FileContext.hh"
#include "xmlx.hh"

namespace openmsx {

const XMLElement& VDPIODelay::getConfig()
{
	static XMLElement deviceElem("VDPIODelay");
	static bool init = false;
	if (!init) {
		init = true;
		deviceElem.addAttribute("id", "VDPIODelay");
		deviceElem.setFileContext(auto_ptr<FileContext>(new SystemFileContext()));
	}
	return deviceElem;
}

VDPIODelay::VDPIODelay(MSXIODevice* device_, const EmuTime& time)
	: MSXDevice(getConfig(), time),
	  MSXIODevice(getConfig(), time),
	  cpu(MSXCPU::instance()),
	  device(device_)
{
}

byte VDPIODelay::readIO(byte port, const EmuTime& time)
{
	delay(time);
	return device->readIO(port, lastTime.getTime());
}

void VDPIODelay::writeIO(byte port, byte value, const EmuTime& time)
{
	delay(time);
	device->writeIO(port, value, lastTime.getTime());
}

void VDPIODelay::delay(const EmuTime& time)
{
	if (cpu.isR800Active()) {
		lastTime += 57;	// 8us
		if (time < lastTime.getTime()) {
			cpu.wait(lastTime.getTime());
			return;
		}
	}
	lastTime.advance(time);
}

} // namespace openmsx
