// Server command and convar helpers.
// Included by swiftlybhoptimer_helper.cpp to keep helper features grouped without changing linkage.

void SwiftlyBhopTimerHelper::ServerCommand(const char *command, bool quiet)
{
	if (!g_pEngineServer2 || !command || !command[0])
	{
		return;
	}

	char buffer[256] = {};
	V_snprintf(buffer, sizeof(buffer), "%s\n", command);
	g_pEngineServer2->ServerCommand(buffer);
	if (!quiet)
	{
		SbtMessage("Executed server command: %s\n", command);
	}
}

void SwiftlyBhopTimerHelper::SetConVar(const char *name, const char *value, bool quiet)
{
	if (!name || !value)
	{
		return;
	}

	if (g_pCVar)
	{
		ConVarRefAbstract convar(g_pCVar->FindConVar(name, true));
		if (convar.IsConVarDataAvailable())
		{
			convar.SetString(value);
			if (!quiet)
			{
				SbtMessage("Applied convar directly: %s %s\n", name, value);
			}
			return;
		}
	}

	char buffer[256] = {};
	V_snprintf(buffer, sizeof(buffer), "%s %s\n", name, value);
	g_pEngineServer2->ServerCommand(buffer);
	if (!quiet)
	{
		SbtMessage("Queued convar command: %s %s\n", name, value);
	}
}

