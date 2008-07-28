// $Id$

#include "MSXPPI.hh"
#include "I8255.hh"
#include "Keyboard.hh"
#include "LedEvent.hh"
#include "LedStatus.hh"
#include "MSXCPUInterface.hh"
#include "MSXMotherBoard.hh"
#include "KeyClick.hh"
#include "CassettePort.hh"
#include "XMLElement.hh"
#include "RenShaTurbo.hh"
#include "serialize.hh"
#include <string>
#include <cassert>

namespace openmsx {

// MSXDevice

MSXPPI::MSXPPI(MSXMotherBoard& motherBoard, const XMLElement& config)
	: MSXDevice(motherBoard, config)
	, cassettePort(motherBoard.getCassettePort())
	, renshaTurbo(motherBoard.getRenShaTurbo())
	, prevBits(15)
	, selectedRow(0)
{
	bool keyGhosting = config.getChildDataAsBool("key_ghosting", true);
	bool keyGhostingSGCprotected = config.getChildDataAsBool(
				"key_ghosting_sgc_protected", true);
	std::string keyboardType = config.getChildData("keyboard_type", "int");
	bool hasKeypad = config.getChildDataAsBool("has_keypad", true);
	bool codeKanaLocks = config.getChildDataAsBool("code_kana_locks", false);
	bool graphLocks = config.getChildDataAsBool("graph_locks", false);
	keyboard.reset(new Keyboard(motherBoard.getScheduler(),
	                            motherBoard.getCommandController(),
	                            motherBoard.getEventDistributor(),
	                            motherBoard.getMSXEventDistributor(),
	                            keyboardType, hasKeypad,
	                            keyGhosting, keyGhostingSGCprotected,
	                            codeKanaLocks, graphLocks));
	const EmuTime& time = getCurrentTime();
	i8255.reset(new I8255(*this, time, motherBoard.getMSXCliComm()));
	click.reset(new KeyClick(motherBoard.getMSXMixer(), config));

	reset(time);
}

MSXPPI::~MSXPPI()
{
}

void MSXPPI::reset(const EmuTime& time)
{
	i8255->reset(time);
	click->reset(time);
}

void MSXPPI::powerDown(const EmuTime& /*time*/)
{
	getMotherBoard().getLedStatus().setLed(LedEvent::CAPS, false);
}

byte MSXPPI::readIO(word port, const EmuTime& time)
{
	switch (port & 0x03) {
	case 0:
		return i8255->readPortA(time);
	case 1:
		return i8255->readPortB(time);
	case 2:
		return i8255->readPortC(time);
	case 3:
		return i8255->readControlPort(time);
	default: // unreachable, avoid warning
		assert(false);
		return 0;
	}
}

byte MSXPPI::peekIO(word port, const EmuTime& time) const
{
	switch (port & 0x03) {
	case 0:
		return i8255->peekPortA(time);
	case 1:
		return i8255->peekPortB(time);
	case 2:
		return i8255->peekPortC(time);
	case 3:
		return i8255->readControlPort(time);
	default: // unreachable, avoid warning
		assert(false);
		return 0;
	}
}

void MSXPPI::writeIO(word port, byte value, const EmuTime& time)
{
	switch (port & 0x03) {
	case 0:
		i8255->writePortA(value, time);
		break;
	case 1:
		i8255->writePortB(value, time);
		break;
	case 2:
		i8255->writePortC(value, time);
		break;
	case 3:
		i8255->writeControlPort(value, time);
		break;
	default:
		assert(false);
	}
}


// I8255Interface

byte MSXPPI::readA(const EmuTime& time)
{
	return peekA(time);
}
byte MSXPPI::peekA(const EmuTime& /*time*/) const
{
	// port A is normally an output on MSX, reading from an output port
	// is handled internally in the 8255
	// TODO check this on a real MSX
	// TODO returning 0 fixes the 'get_selected_slot' script right after
	//      reset (when PPI directions are not yet set). For now this
	//      solution is good enough.
	return 0;
}
void MSXPPI::writeA(byte value, const EmuTime& /*time*/)
{
	getMotherBoard().getCPUInterface().setPrimarySlots(value);
}

byte MSXPPI::readB(const EmuTime& time)
{
	return peekB(time);
}
byte MSXPPI::peekB(const EmuTime& time) const
{
       if (selectedRow != 8) {
               return keyboard->getKeys()[selectedRow];
       } else {
               return keyboard->getKeys()[8] | renshaTurbo.getSignal(time);
       }
}
void MSXPPI::writeB(byte /*value*/, const EmuTime& /*time*/)
{
	// probably nothing happens on a real MSX
}

nibble MSXPPI::readC1(const EmuTime& time)
{
	return peekC1(time);
}
nibble MSXPPI::peekC1(const EmuTime& /*time*/) const
{
	return 15; // TODO check this
}
nibble MSXPPI::readC0(const EmuTime& time)
{
	return peekC0(time);
}
nibble MSXPPI::peekC0(const EmuTime& /*time*/) const
{
	return 15; // TODO check this
}
void MSXPPI::writeC1(nibble value, const EmuTime& time)
{
	if ((prevBits ^ value) & 1) {
		cassettePort.setMotor(!(value & 1), time);	// 0=0n, 1=Off
	}
	if ((prevBits ^ value) & 2) {
		cassettePort.cassetteOut(value & 2, time);
	}
	if ((prevBits ^ value) & 4) {
		getMotherBoard().getLedStatus().setLed(LedEvent::CAPS, !(value & 4));
	}
	if ((prevBits ^ value) & 8) {
		click->setClick(value & 8, time);
	}
	prevBits = value;
}
void MSXPPI::writeC0(nibble value, const EmuTime& /*time*/)
{
	selectedRow = value;
}


template<typename Archive>
void MSXPPI::serialize(Archive& ar, unsigned /*version*/)
{
	ar.template serializeBase<MSXDevice>(*this);
	ar.serialize("i8255", *i8255);

	// merge prevBits and selectedRow into one byte
	byte portC = (prevBits << 4) | (selectedRow << 0);
	ar.serialize("portC", portC);
	if (ar.isLoader()) {
		selectedRow = (portC >> 0) & 0xF;
		nibble bits = (portC >> 4) & 0xF;
		writeC1(bits, getCurrentTime());
	}
	ar.serialize("keyboard", *keyboard);
}
INSTANTIATE_SERIALIZE_METHODS(MSXPPI);

} // namespace openmsx
