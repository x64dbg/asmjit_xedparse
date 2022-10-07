# asmjit_xedparse

asmjit_xedparse is a wrapper library around asmjit that exposes an [XEDParse](https://github.com/x64dbg/XEDParse) interface:

```
enum XEDPARSE_STATUS
{
    XEDPARSE_ERROR = 0,
    XEDPARSE_OK = 1
};
struct XEDPARSE
{
    bool x64; // use 64-bit instructions
    ULONGLONG cip; //instruction pointer (for relative addressing)
    unsigned int dest_size; //destination size (returned by XEDParse)
    CBXEDPARSE_UNKNOWN cbUnknown; //unknown operand callback
    unsigned char dest[XEDPARSE_MAXASMSIZE]; //destination buffer
    char instr[XEDPARSE_MAXBUFSIZE]; //instruction text
    char error[XEDPARSE_MAXBUFSIZE]; //error text (in case of an error)
};

XEDPARSE_STATUS XEDParseAssemble(XEDPARSE* XEDParse);
```

[x64dbg](https://github.com/x64dbg/x64dbg) makes use of this DLL for its feature to assemble MASM-like plaintext instructions.
