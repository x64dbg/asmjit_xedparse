// compile: cl /W3 xedparse-test.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>

typedef enum _XEDPARSE_STATUS
{
	XEDPARSE_ERROR = 0,
	XEDPARSE_OK = 1
} XEDPARSE_STATUS;

#define XEDPARSE_CALL //calling convention
#define XEDPARSE_MAXBUFSIZE 256
#define XEDPARSE_MAXASMSIZE 16

typedef bool (XEDPARSE_CALL* CBXEDPARSE_UNKNOWN)(const char* text, ULONGLONG* value);

#pragma pack(push,8)
typedef struct _XEDPARSE
{
	bool x64; // use 64-bit instructions
	ULONGLONG cip; //instruction pointer (for relative addressing)
	unsigned int dest_size; //destination size (returned by XEDParse)
	CBXEDPARSE_UNKNOWN cbUnknown; //unknown operand callback
	unsigned char dest[XEDPARSE_MAXASMSIZE]; //destination buffer
	char instr[XEDPARSE_MAXBUFSIZE]; //instruction text
	char error[XEDPARSE_MAXBUFSIZE]; //error text (in case of an error)
} XEDPARSE;
#pragma pack(pop)

typedef XEDPARSE_STATUS (XEDPARSE_CALL* XEDParseAssemble)(XEDPARSE* XEDParse);

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Expected argument: name of DLL implementing XEDParseAssemble interface, exit.\n");
		return EXIT_FAILURE;
	}
	
	printf("Loading '%s'...\n", argv[1]);
	HMODULE m = LoadLibrary(argv[1]);
	if (!m) {
		printf("Failed to load '%s', error %u, exit.\n", argv[1], GetLastError());
		return EXIT_FAILURE;
	}
	XEDParseAssemble p = (XEDParseAssemble) GetProcAddress(m, "XEDParseAssemble");

	XEDPARSE parse;
	memset(&parse, 0, sizeof(parse));
	parse.x64 = true;
	parse.cip = 0x14000000;
	strcpy_s(parse.instr, sizeof(parse.instr), "xchg eax, eax");

	if (p(&parse) != XEDPARSE_OK) {
		FreeLibrary(m);
		printf("XEDParseAssemble() failed, error '%s', exit.\n", parse.error);
		return EXIT_FAILURE;
	}

	printf("Got %u bytes: ", parse.dest_size);
	for (unsigned i = 0; i < parse.dest_size; i++)
		printf("%02x", parse.dest[i]);
	printf("\n");
	
	FreeLibrary(m);
	
	return EXIT_SUCCESS;
}
