// Plugin metadata accessors.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

const char *SwiftlyBhopTimerHelper::GetAuthor()
{
	return "SwiftlyBhopTimer contributors";
}

const char *SwiftlyBhopTimerHelper::GetName()
{
	return "SwiftlyBhopTimer MetaMod Helper";
}

const char *SwiftlyBhopTimerHelper::GetDescription()
{
	return "Native helper for SwiftlyBhopTimer replay bot commands.";
}

const char *SwiftlyBhopTimerHelper::GetURL()
{
	return "https://github.com/";
}

const char *SwiftlyBhopTimerHelper::GetLicense()
{
	return "MIT";
}

const char *SwiftlyBhopTimerHelper::GetVersion()
{
#ifndef SBT_HELPER_VERSION
#define SBT_HELPER_VERSION "0.1.0"
#endif
	return SBT_HELPER_VERSION;
}

const char *SwiftlyBhopTimerHelper::GetDate()
{
	return __DATE__;
}

const char *SwiftlyBhopTimerHelper::GetLogTag()
{
	return "SwiftlyBhopTimerHelper";
}
