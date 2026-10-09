#ifndef __CONFIG_LOADER_H__
#define __CONFIG_LOADER_H__

#include "cocos2d.h"
#include "CommonDefines.h"

_CSTART

class ConfigLoader
{
	friend class AppDelegate;
public:

	void loadMainConfig(const std::string& configPath);
	void loadViewConfigs(const std::string& listPath);

private:
	ConfigLoader();
	void parseViewConfig(const std::string& configPath);
};

#define CL SL->getService<ConfigLoader>()

_CEND

#endif