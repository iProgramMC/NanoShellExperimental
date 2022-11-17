#include <main.h>
#include <memory.h>
#include <print.h>
#include <string.h>
#include <keyboard.h>

#define PATH_MAX 255

extern Console *g_currentConsole;
char g_cwd[PATH_MAX + 2];

void ShellInit()
{
	strcpy (g_cwd, "/");
}

void ShellExecuteCommand(char* p)
{
	TokenState state;
	state.m_bInitted = 0;
	char* token = Tokenize (&state, p, " ");
	if (!token)
		return;
	if (*token == 0)
		return;
	
	if (strcmp (token, "help") == 0)
	{
		LogMsg("NanoShell Experimental Shell Help!");
		LogMsg("cls         - Clears the screen!");
		LogMsg("xyzzy       - The magic word.");
	}
	else if (strcmp (token, "xyzzy") == 0)
	{
		LogMsg("Huzzah!");
	}
	else if (strcmp (token, "cls") == 0)
	{
		CoClearScreen(g_currentConsole);
		g_currentConsole->curX = 0;
		g_currentConsole->curY = 0;
	}
	else
	{
		LogMsg("Unknown command.  Please type 'help'.");
	}
}

void ShellRun()
{
	ShellInit();
	
	while (1)
	{
		LogMsgNoCr("shell>", g_cwd);
		char buffer[256];
		CoGetString(buffer, sizeof buffer);
		
		ShellExecuteCommand(buffer);
		
		hlt;
	}
}

