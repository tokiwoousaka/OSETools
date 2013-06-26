using System.Linq;
using System.Collections.Generic;
using System;

namespace OSC
{
    public enum OSECode1
    {
        NoOp = 0x00
    }
    
    public enum OSECode2
    {
        Cond = 0x04,
        Assert = 0x0D
    }
    
    public enum OSECode3
    {
        Copy = 0x10,
        PCopy = 0x1E
    }
    
    public enum OSECode4
    {
        Or = 0x10,
        Xor = 0x11,
        And = 0x12,
        Add = 0x14,
        Sub = 0x15,
        Multi = 0x16,
        ShiftL = 0x18,
        ShiftR = 0x19,
        PLimLowr = 0x1C,
        PLimUpr = 0x1D,
        CompEq = 0x20,
        CompNeq = 0x21,
        CompLess = 0x22,
        CompGreEq = 0x23,
        CompLesEq = 0x24,
        CompGret = 0x25,
        TestZero = 0x26,
        TestNZero = 0x27,
        PCompEq = 0x28,
        PCompNeq = 0x29,
        PCompLess = 0x2A,
        PCompGreEq = 0x2B,
        PCompLesEq = 0x2C,
        PCompGret = 0x2D
    }
    
    public enum OSECode6
    {
        Label = 0x01,
        LoadImm = 0x02,
        PLoadImm = 0x03
    }
    
    public enum OSECode8
    {
        LoadMem = 0x08,
        StoreMem = 0x09,
        PAdd = 0x0E,
        PDiff = 0x0F
    }

    public enum OSECode11
    {
        PCast = 0x1F
    }
}
