#include "pch.h"
#include "InvUploader.h"
#include "base64.h"
#include "base64.cpp"
#include <fstream>
#include <chrono>


BAKKESMOD_PLUGIN(InvUploader, "Uploads Inventory to PMRLR", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void InvUploader::onLoad()
{
	
	_globalCvarManager = cvarManager;
	
	cvarManager->executeCommand("cl_notifications_enabled_beta 1"); //Enables Toast Notifications.
	
	//Register the Toast Notifications
	cvarManager->registerNotifier("pmrlr_load_toast", [this](std::vector<std::string> args) { //Used when the plugin is loaded.
		gameWrapper->Toast("InvUploader Loaded", "The PMRLR Inventory Uploader plugin has loaded successfully!", "", 5.0, ToastType_OK);
	}, "", PERMISSION_ALL);
	cvarManager->registerNotifier("pmrlr_unload_toast", [this](std::vector<std::string> args) { //Used when the plugin is unloaded.
		gameWrapper->Toast("The PMRLR Inventory Uploader plugin has been unloaded, please reload it!", "", 5.0, ToastType_Warning);
	}, "", PERMISSION_ALL);
	
	cvarManager->executeCommand("pmrlr_load_toast"); 
	cvarManager->log("The PMRLR Inventory Uploader plugin has loaded successfully!");
	
	//Checks if the BetterInventoryExport plugin is installed, downloads it if not.
	if(!std::filesystem::exists(gameWrapper->GetBakkesModPath() / "plugins" / "BetterInventoryExport.dll")) {
		cvarManager->log("Required plugin not found! Installing plugin...");
		cvarManager->executeCommand("bpm_install 155");
		cvarManager->log("Plugin installed!");
	}
	
	//Loads the BetterInventoryExport plugin.
	cvarManager->executeCommand("plugin load BetterInventoryExport", false);
	
	//A simple function hook that calls InvSave() whenever online products (items) are changed. If confused, see https://bakkesmodwiki.github.io/functions/using_function_hooks/
	gameWrapper->HookEvent("Function TAGame.ProductsSave_TA.HandleOnlineProductsChanged", [this](std::string eventName) {
		cvarManager->log("Inventory update detected! Saving...");
		
		InvSave();
	});
	
	InvSave();
		
}

void InvUploader::onUnload()
{
	
	cvarManager->executeCommand("pmrlr_unload_toast");
	cvarManager->log("The PMRLR Inventory Uploader plugin has been unloaded, please reload it in the Plugin Manager or running \"plugin load InvUploader\" in the F6 Console.");
	
}

void InvUploader::InvSave()
{
	
	//Saves inventory to inventory.csv and uploads it.
	cvarManager->executeCommand("invent_dump_better csv");
	cvarManager->log("Inventory saved!");
	
	InvUpload();
	
}

void InvUploader::InvUpload()
{
	cvarManager->log("Uploading new inventory to PMRLR...");

	//This will create a string "encoded" that holds the inventory.csv file in base64 format that will later be passed into the http request.
	std::fstream file(gameWrapper->GetBakkesModPath() / "data" / "inventory.csv"); //File object for inventory.csv
	std::string line;
	std::string csv;

	while (file.good()) { //Converts all contents of inventory.csv to the "csv" string.

		std::getline(file, line);
		csv += line + "\n";

	}

	file.close();

	const std::string encodeThis = csv; //Creates a const version of "csv", required for the base64_encode() parameter.
	std::string encoded = base64_encode(encodeThis, false); // The string "encoded" now holds the base64 version of inventory.csv, we will pass it into the http request.
	
	char const* c = encoded.c_str(); //Turns the string "encoded" into bytes.
	
	//creates a readable format of the current time, which we will use in the filename.
	time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%B-%d-%Y_%Hhr-%Mmin-%Ssec_UTC", &tstruct);

    std::string currentTime = buf;
	
	//Creating the HTTP Request
	const auto url = fmt::format(""); //URL for the API goes here.
	const auto api_key = fmt::format(""); //Auth key goes here. (If you need it.)
	char text[] = R"({"message": "inventory_updated", "content": ")";
	char text2[] = R"(","sha": ""})";
	
	//Because C++ doesn't have a good way to parse Json, we do this sandwiched sort of solution where we take half of the request in "text", insert "encoded" as our content, and add "text2" to the end. 
	const auto requestbody = text + encoded + text2;
	
	CurlRequest req;
	req.verb = "PUT"; //The type of request is PUT because it allows us to commit to Github.
	req.url = url + "inventory_" +  currentTime + ".csv"; //The reason why the current time is the name of the file is twofold, first we have it to be able to track inventories and date them so we can compare lost items or gained items, and secondly its to avoid having a "sha" in the request. 
	req.headers["Authorization"] = api_key; 
	req.body = requestbody; 
	
	HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result) {
		LOG("Json result{}", result); //Logs the result for easy debugging.
		LOG("Your inventory has been uploaded to PMRLR successfully!");
	});

}
