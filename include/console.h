#ifndef _CONSOLE_H
#define _CONSOLE_H

#define DefaultConsoleColor 0x1F

enum ConsoleType {
	CONSOLE_TYPE_NONE, // uninitialized
	CONSOLE_TYPE_TEXT, // always full screen
	CONSOLE_TYPE_FRAMEBUFFER, // can either be the entire screen or just a portion of it. TODO
	CONSOLE_TYPE_SERIAL, // just plain old serial
};

typedef struct ConsoleStruct {
	int type; // ConsoleType enum
	int width, height; // width and height
	uint16_t *textBuffer; // unused in fb mode
	uint16_t color; // colors
	int curX, curY; // cursor X and Y positions
	bool pushOrWrap;// check if we should push whole screen up, or clear&wrap
	int font;
} Console;

extern Console g_debugConsole; // for LogMsg
extern Console g_debugSerialConsole; // for SLogMsg
extern uint32_t g_vgaColorsToRGB[];

void CoPlotChar (Console *this, int x, int y, char c);
void CoScrollUpByOne (Console *this);
void CoClearScreen (Console *this);
void CoPrintChar (Console* this, char c);
void CoPrintString (Console* this, const char *c);
void CoInitAsText (Console* this);
void CoInitAsGraphics (Console* this);
void CoInitAsSerial (Console* this);
void LogMsg (const char* fmt, ...);
void LogMsgNoCr (const char* fmt, ...);
void SLogMsg (const char* fmt, ...);
void SLogMsgNoCr (const char* fmt, ...);
void LogHexDumpData (void* pData, int size);
void ResetConsole();
void SetConsole(Console* pConsole);

#endif//_CONSOLE_H