// $Id$

#ifndef FILENAMESETTING_HH
#define FILENAMESETTING_HH

#include "StringSetting.hh"

namespace openmsx {

class FileContext;

class FilenameSettingPolicy : public StringSettingPolicy
{
protected:
	explicit FilenameSettingPolicy(CommandController& commandController);
	void tabCompletion(std::vector<std::string>& tokens) const;
	std::string getTypeString() const;
};

class FilenameSetting : public SettingImpl<FilenameSettingPolicy>
{
public:
	FilenameSetting(CommandController& commandController,
	                const std::string& name, const std::string& description,
	                const std::string& initialValue);
};

} // namespace openmsx

#endif
