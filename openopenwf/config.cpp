#include "openwf.h"
#include "json.hpp"

#include <fstream>
#include <filesystem>

using json = nlohmann::json;

void OpenWFConfig::PrintToConsole()
{
	OWFLog("  serverHost = {}", g_Config.serverHost);
	OWFLog("  disableNRS = {}", g_Config.disableNRS);
	OWFLog("  httpPort = {}", g_Config.httpPort);
	OWFLog("  httpsPort = {}", g_Config.httpsPort);
	//OWFLog("  disableCLR = {}", g_Config.disableCLR);
	OWFLog("");
}

static void ParseConfig(const json& j)
{
	OpenWFConfig newConfig;
	newConfig.serverHost = j.value<std::string>("server_host", g_Config.serverHost);
	newConfig.disableNRS = j.value<bool>("disable_nrs_connection", g_Config.disableNRS);
	newConfig.httpPort = j.value<int>("http_port", g_Config.httpPort);
	newConfig.httpsPort = j.value<int>("https_port", g_Config.httpsPort);
	newConfig.disableCLR = j.value<bool>("disable_clr", g_Config.disableCLR);

	g_Config = newConfig;
}

void LoadConfig()
{
	std::wstring jsonConfigFile = g_wfExeDirectory + L"OpenWF\\Client Config.json";

	std::ifstream jsonFile(jsonConfigFile);
	if (!jsonFile)
	{
		OWFLog("No OpenWF Enabler config file, ignoring...");
		return;
	}

	std::string result;
	result.resize(std::filesystem::file_size(jsonConfigFile));
	if (!jsonFile.read(&result[0], result.size()))
	{
		OWFLog("OpenWF Enabler config file read failed, ignoring...");
		return;
	}

	try
	{
		ParseConfig(json::parse(result));
		OWFLog("Config loaded from {}", WideToUTF8(jsonConfigFile));
	}
	catch (const std::exception& ex)
	{
		OWFLog("Could not parse OpenWF Enabler config's JSON: {}", ex.what());
		OWFLog("Configuration file ignored...");
		return;
	}
}
