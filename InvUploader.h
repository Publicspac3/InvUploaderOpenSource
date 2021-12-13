#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class InvUploader: public BakkesMod::Plugin::BakkesModPlugin
{

	virtual void onLoad();
	virtual void onUnload();

public:	
	void InvSave();
	void InvUpload();
	
};
