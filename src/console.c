#include <stdint.h>
#include <main.h>
#include <string.h>
#include <print.h>
#include <console.h>

//note: important system calls are preceded by cli and succeeded by sti

uint16_t TextModeMakeChar(uint8_t fgbg, uint8_t chr) {
	uint8_t comb = fgbg;
	uint16_t comb2 = comb << 8 | chr;
	return comb2;
}
uint32_t g_vgaColorsToRGB[] = {
	0xFF000000,
	0xFF0000AA,
	0xFF00AA00,
	0xFF00AAAA,
	0xFFAA0000,
	0xFFAA00AA,
	0xFFAAAA00,
	0xFFAAAAAA,
	0xFF555555,
	0xFF5555FF,
	0xFF55FF55,
	0xFF55FFFF,
	0xFFFF5555,
	0xFFFF55FF,
	0xFFFFFF55,
	0xFFFFFFFF,
};

extern bool g_uses8by16Font;
Console g_debugConsole; // for LogMsg

Console* g_currentConsole = &g_debugConsole;

uint16_t* g_pBufferBase = (uint16_t*)(KERNEL_MEM_START + 0xB8000);
int g_textWidth = 80, g_textHeight = 25;

void SetConsole(Console* pConsole) {
	g_currentConsole = pConsole;
}
void ResetConsole() {
	g_currentConsole = &g_debugConsole;
}
void CoClearScreen(Console *this) {
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	if (this->type == CONSOLE_TYPE_TEXT) {
		uint16_t lolo = TextModeMakeChar(this->color, ' ');
		for (int i = 0; i < this->width*this->height; i++) this->textBuffer[i] = lolo;
	}
}
void CoInitAsText(Console *this) {
	this->curX = this->curY = 0;
	this->width  = g_textWidth;
	this->height = g_textHeight;
	this->type = CONSOLE_TYPE_TEXT;
	this->textBuffer = g_pBufferBase;
	this->color = DefaultConsoleColor;//default
	this->pushOrWrap = 0;//push
	uint16_t lolo = TextModeMakeChar(this->color, ' ');
	for (int i = 0; i < this->width*this->height; i++) this->textBuffer[i] = lolo;
}
void CoMoveCursor(Console* this) {
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	if (this->type == CONSOLE_TYPE_TEXT && this->textBuffer == g_pBufferBase) {
		uint16_t cursorLocation = this->curY * this->width + this->curX;
		WritePort(0x3d4, 14);
		WritePort(0x3d5, cursorLocation >> 8);
		WritePort(0x3d4, 15);
		WritePort(0x3d5, cursorLocation);
	}
}
void CoPlotChar (Console *this, int x, int y, char c) {
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	if (this->type == CONSOLE_TYPE_TEXT) {
		uint16_t chara = TextModeMakeChar (this->color, c);
		// TODO: add bounds check
		this->textBuffer [x + y * this->width] = chara;
	}
}
void CoScrollUpByOne(Console *this) {
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	if (this->pushOrWrap) {
		//CoClearScreen(this);
		this->curX = this->curY = 0;
		return;
	}
	if (this->type == CONSOLE_TYPE_TEXT) {
		memcpy (this->textBuffer, &this->textBuffer[this->width], this->width * (this->height - 1) * sizeof(short));
		//uint16_t* p = &this->textBuffer[this->width * (this->height - 1) * sizeof(short)];
		for (int i=0; i<this->width; i++)
		{
			CoPlotChar (this, i, this->height - 1, 0);
		}
	}
	else {
	}
}
bool g_shouldntUpdateCursor = false;
void CoPrintChar (Console* this, char c) {
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	switch (c) {
		case '\b':
			if (--this->curX < 0) {
				this->curX = this->width - 1;
				if (--this->curY < 0) this->curY = 0;
			}
			CoPlotChar(this, this->curX, this->curY, 0);
			break;
		case '\r': 
			this->curX = 0;
			break;
		case '\n': 
			this->curX = 0;
			this->curY++;
			while (this->curY >= this->height) {
				CoScrollUpByOne(this);
				this->curY--;
			}
			break;
		
		default: {
			CoPlotChar(this, this->curX, this->curY, c);
			// advance cursor
			if (++this->curX >= this->width) {
				this->curX = 0;
				this->curY++;
			}
			while (this->curY >= this->height) {
				CoScrollUpByOne(this);
				this->curY--;
			}
			break;
		}
	}
	if (!g_shouldntUpdateCursor) CoMoveCursor(this);
}
void CoPrintString (Console* this, const char *c) {
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	g_shouldntUpdateCursor = true;
	while (*c) CoPrintChar(this, *c++);
	g_shouldntUpdateCursor = false;
	CoMoveCursor(this);
}

void LogMsg (const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(g_currentConsole, cr);
	
	va_end(list);
}
void LogMsgNoCr (const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(g_currentConsole, cr);
	
	va_end(list);
}
const char* g_uppercaseHex = "0123456789ABCDEF";

void LogHexDumpData (void* pData, int size) {
	uint8_t* pDataAsNums = (uint8_t*)pData, *pDataAsText = (uint8_t*)pData;
	char c[7], d[4];
	c[5] = 0;   d[2] = ' ';
	c[6] = ' '; d[3] = 0;
	c[4] = ':';
	
	#define BYTES_PER_ROW 16
	for (int i = 0; i < size; i += BYTES_PER_ROW) {
		// print the offset
		c[0] = g_uppercaseHex[(i & 0xF000) >> 12];
		c[1] = g_uppercaseHex[(i & 0x0F00) >>  8];
		c[2] = g_uppercaseHex[(i & 0x00F0) >>  4];
		c[3] = g_uppercaseHex[(i & 0x000F) >>  0];
		LogMsgNoCr("%s", c);
		
		for (int j = 0; j < BYTES_PER_ROW; j++) {
			uint8_t p = *pDataAsNums++;
			d[0] = g_uppercaseHex[(p & 0xF0) >> 4];
			d[1] = g_uppercaseHex[(p & 0x0F) >> 0];
			LogMsgNoCr("%s", d);
		}
		LogMsgNoCr("   ");
		for (int j = 0; j < BYTES_PER_ROW; j++) {
			char c = *pDataAsText++;
			if (c < ' ') c = '.';
			LogMsgNoCr("%c",c);
		}
		LogMsg("");
	}
}
