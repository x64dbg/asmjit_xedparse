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

static bool StrDel(char* Source, const char* Needle)
{
    // Find the location in the string first
    char* loc = stristr(Source, Needle);

    if (!loc)
        return false;

    // "Delete" the word by shifting it over
    auto needleLen = strlen(Needle);

    memmove(loc, loc + needleLen, strlen(loc) - needleLen + 1);

    return true;
}


static Error ASMJIT_CDECL unknownSymbolHandler(AsmParser* parser, Operand* dst, const char* name, size_t len)
{
    unsigned long long value;
    if (CBXEDPARSE_UNKNOWN(parser->getUnknownSymbolHandlerData())(name, &value))
    {
        *dst = imm(value);
        return kErrorOk;
    }
    return kErrorInvalidLabel;
}

XEDPARSE_EXPORT XEDPARSE_STATUS XEDPARSE_CALL XEDParseAssemble(XEDPARSE* XEDParse)
{
    if (!XEDParse)
        return XEDPARSE_ERROR;

    // Only assemble one instruction
    auto len = strlen(XEDParse->instr);
    for (size_t i = 0; i < len; i++)
    {
        auto & ch = XEDParse->instr[i];
        if (ch == ';' || ch == '\r' || ch == '\n')
        {
            ch = '\0';
            break;
        }
    }

    bool short_command = StrDel(XEDParse->instr, "short ");

    // Setup CodeInfo
    CodeInfo codeinfo(XEDParse->x64 ? ArchInfo::kTypeX64 : ArchInfo::kTypeX86, 0, XEDParse->cip);

    // Setup CodeHolder
    CodeHolder code;
    auto err = code.init(codeinfo);
    if (err != kErrorOk)
    {
        strcpy_s(XEDParse->error, DebugUtils::errorAsString(err));
        *XEDParse->error = tolower(*XEDParse->error);
        return XEDPARSE_ERROR;
    }

    // Attach an assembler to the CodeHolder.
    X86Assembler a(&code);

    // Create AsmParser that will emit to X86Assembler.
    AsmParser p(&a);
    if (XEDParse->cbUnknown)
        p.setUnknownSymbolHandler(unknownSymbolHandler, XEDParse->cbUnknown);

    // Parse some assembly.
    err = p.parse(XEDParse->instr);

    // Error handling
    if (err != kErrorOk)
    {
        strcpy_s(XEDParse->error, DebugUtils::errorAsString(err));
        *XEDParse->error = tolower(*XEDParse->error);
        return XEDPARSE_ERROR;
    }

    // Check for unresolved relocations
    if(code._relocations.getLength())
    {
        strcpy_s(XEDParse->error, "unresolved relocation");
        return XEDPARSE_ERROR;
    }

    // If we are done, you must detach the Assembler from CodeHolder or sync
    // it, so its internal state and position is synced with CodeHolder.
    code.sync();

    // Now you can print the code, which is stored in the first section (.text).
    auto & buffer = code.getSectionEntry(0)->getBuffer();
    XEDParse->dest_size = std::min<unsigned int>((unsigned int)buffer.getLength(), XEDPARSE_MAXASMSIZE);
    if (short_command && XEDParse->dest_size > 2)
    {
        strcpy_s(XEDParse->error, "destination is out of range");
        return XEDPARSE_ERROR;
    }
    memcpy(XEDParse->dest, buffer.getData(), XEDParse->dest_size);

    return XEDPARSE_OK;
}