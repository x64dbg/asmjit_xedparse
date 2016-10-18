#include "XEDParse.h"
#include <asmtk/asmtk.h>
#include <algorithm>

using namespace asmjit;
using namespace asmtk;

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
    memcpy(XEDParse->dest, buffer.getData(), XEDParse->dest_size);

    return XEDPARSE_OK;
}