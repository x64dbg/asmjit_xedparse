#include "XEDParse.h"
#include <asmtk/asmtk.h>
#include <algorithm>

using namespace asmjit;
using namespace asmtk;

static char* stristr(const char* haystack, const char* needle)
{
    // Case insensitive strstr
    // http://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
    do
    {
        const char* h = haystack;
        const char* n = needle;
        while (tolower((unsigned char)*h) == tolower((unsigned char)*n) && *n)
        {
            h++;
            n++;
        }

        if (*n == 0)
            return (char*)haystack;

    } while (*haystack++);

    // Not found
    return nullptr;
}

static bool StrDel(char* Source, char* Needle, char StopAt = '\0')
{
    // Find the location in the string first
    char* loc = stristr(Source, Needle);

    if (!loc)
        return false;

    // "Delete" the word by shifting it over
    auto needleLen = strlen(Needle);

    memcpy(loc, loc + needleLen, strlen(loc) - needleLen + 1);

    return true;
}

XEDPARSE_EXPORT XEDPARSE_STATUS XEDPARSE_CALL XEDParseAssemble(XEDPARSE* XEDParse)
{
    if (!XEDParse)
        return XEDPARSE_ERROR;

    auto sep = strstr(XEDParse->instr, ";");
    if (!sep)
        sep = strstr(XEDParse->instr, "\n");
    if (sep)
        *sep = '\0';
    StrDel(XEDParse->instr, "short ");

    // Setup a CodeHolder for X64.
    CodeHolder code;
    CodeInfo codeinfo(XEDParse->x64 ? ArchInfo::kTypeX64 : ArchInfo::kTypeX86);
    codeinfo.setBaseAddress(XEDParse->cip);
    code.init(codeinfo);

    // Attach an assembler to the CodeHolder.
    X86Assembler a(&code);

    // Create AsmParser that will emit to X86Assembler.
    AsmParser p(&a);

    // Parse some assembly.
    Error err = p.parse(XEDParse->instr);

    // Error handling (use asmjit::ErrorHandler for more robust error handling).
    if (err != kErrorOk)
    {
        sprintf_s(XEDParse->error, "ERROR %08X (%s)", err, DebugUtils::errorAsString(err));
        return XEDPARSE_ERROR;
    }

    // If we are done, you must detach the Assembler from CodeHolder or sync
    // it, so its internal state and position is synced with CodeHolder.
    code.sync();

    // Now you can print the code, which is stored in the first section (.text).
    CodeBuffer& buffer = code.getSectionEntry(0)->buffer;
    XEDParse->dest_size = std::min<unsigned int>((unsigned int)buffer.length, XEDPARSE_MAXASMSIZE);
    memcpy(XEDParse->dest, buffer.data, XEDParse->dest_size);

    return XEDPARSE_OK;
}