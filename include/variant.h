#ifndef _VARIANT_H
#define _VARIANT_H

struct Variant
{
	enum {
		TYPE_NONE,
		TYPE_STRING,
		TYPE_INT,
		TYPE_POINTER
	} m_type;
	
	char m_strContents[256];
	union {
		int   m_strLength;
		int   m_asInt;
		void* m_asPtr;
	};
};
typedef struct Variant Variant;

void Variant_InitAsString (Variant* pVar, const char* pString);
void Variant_InitAsInt    (Variant* pVar, int value);
void Variant_InitAsPointer(Variant* pVar, void* pointer);

#define C_MAX_ELEMENTS_IN_VARLIST 3
struct VariantList 
{
	Variant m_variants[C_MAX_ELEMENTS_IN_VARLIST];
};
typedef struct VariantList VariantList;

void     VariantList_Reset(VariantList* pVList);
Variant* VariantList_Get  (VariantList* pVList, int index);


#endif//_VARIANT_H

