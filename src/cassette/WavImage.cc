// $Id$

#include "WavImage.hh"
#include "File.hh"


namespace openmsx {

WavImage::WavImage(const string& fileName)
	: length(0), buffer(0)
{
	File file(fileName);
	const char* name = file.getLocalName().c_str();
	
	SDL_AudioSpec wavSpec;
	Uint8* wavBuf;
	Uint32 wavLen;
	if (SDL_LoadWAV(name, &wavSpec, &wavBuf, &wavLen) == NULL) {
		string msg = string("CassettePlayer error: ") + SDL_GetError();
		throw MSXException(msg);
	}
	
	clock.setFreq(wavSpec.freq);
	SDL_AudioCVT audioCVT;
	if (SDL_BuildAudioCVT(
			&audioCVT, wavSpec.format, wavSpec.channels, wavSpec.freq,
			AUDIO_S16, 1, wavSpec.freq) == -1) {
		SDL_FreeWAV(wavBuf);
		throw MSXException("Couldn't build wav converter");
	}
	
	buffer = (Uint8*)malloc(wavLen * audioCVT.len_mult);
	audioCVT.buf = buffer;
	audioCVT.len = wavLen;
	memcpy(buffer, wavBuf, wavLen);
	SDL_FreeWAV(wavBuf);

	if (SDL_ConvertAudio(&audioCVT) == -1) {
		free(buffer);
		throw MSXException("Couldn't convert wav");
	}
	length = (int)(audioCVT.len * audioCVT.len_ratio) / 2;
}

WavImage::~WavImage()
{
	free(buffer);
}

short WavImage::getSampleAt(const EmuTime& time)
{
	int pos = clock.getTicksTill(time);
	return pos < length ? ((short*)buffer)[pos] : 0;
}

} // namespace openmsx
