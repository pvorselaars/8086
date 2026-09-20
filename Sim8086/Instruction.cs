using System.Diagnostics;

namespace Sim8086;

public class Instruction(Dictionary<Bits, long> bits)
{
    public enum Format
    {
        RegRm,
        RegImmediate,
        RmImmediate,
        Acc,
    }
    
    public string Operation = "None";
    public Format InstructionFormat = Format.RegRm;
    public int Size = 0;

    private static readonly string[][] Registers =
    [
        [ "al", "ax"],
        [ "cl", "cx"],
        [ "dl", "dx"],
        [ "bl", "bx"],
        [ "ah", "sp"],
        [ "ch", "bp"],
        [ "dh", "si"],
        [ "bh", "di"],
    ];
    
    private long Get(Bits bit) => bits.GetValueOrDefault(bit, 0);

    private bool Has(Bits bit) => bits.ContainsKey(bit);
    
    private static string Register(long reg, long w) => Registers[reg][w];
    private string RmOperand(long mod, long rm, long w) => mod == 0b11 ? Register(rm, w) : MemoryOperand(mod, rm);
    
    private long Data(long w) => w == 0 ? Get(Bits.DataLo) : Get(Bits.DataLo) | (Get(Bits.DataHi) << 8);
    
    public string Print()
    {
        return InstructionFormat switch
        {
            Format.RegRm => PrintRegRm(),
            Format.RegImmediate => PrintRegImmediate(),
            Format.RmImmediate => PrintRmImmediate(),
            Format.Acc => PrintAcc(),
            _ => throw new NotSupportedException()
        };
    }

    private string PrintAcc()
    {
        var d = Get(Bits.D);
        
        var regOperand = "ax";
        var address = $"[{Get(Bits.DispLo) | Get(Bits.DispHi) << 8}]";
        
        var (dst, src) = d == 0 ? (regOperand, address) : (address, regOperand);

        return $"{Operation} {dst}, {src}";
    }

    public string PrintRegRm()
    {
        var d = Get(Bits.D);
        var w = Get(Bits.W);

        var regOperand = Register(Get(Bits.Reg), w);
        var rmOperand = RmOperand(Get(Bits.Mod), Get(Bits.Rm), w);
        
        var (dst, src) = d == 0 ? (rmOperand, regOperand) : (regOperand, rmOperand);
        
        return $"{Operation} {dst}, {src}";
    }
    
    public string PrintRegImmediate()
    {
        var w = Get(Bits.W);
        var data = Data(w);

        var regOperand = Register(Get(Bits.Reg), w);

        return $"{Operation} {regOperand}, {data}";
    }
    
    private string PrintRmImmediate()
    {
        var w = Get(Bits.W);
        var rmOperand = RmOperand(Get(Bits.Mod), Get(Bits.Rm), w);
        var data = Data(w);
        var prefix = w == 1 ? "word" : "byte";
        
        return $"{Operation} {rmOperand}, {prefix} {data}";
    }
    
    private static readonly string[] Addresses =
    [
        "bx + si",
        "bx + di",
        "bp + si",
        "bp + di",
        "si",
        "di",
        "bp",
        "bx"
    ];

    private string MemoryOperand(long mod, long rm)
    {
        if (mod == 0 && rm == 0b110)
            return $"[{Displacement(mod, rm)}]";

        var address = Addresses[rm];
        var displacement = Displacement(mod, rm);

        return displacement switch
        {
            0 => $"[{address}]",
            > 0 => $"[{address} + {displacement}]",
            _ => $"[{address} - {-displacement}]"
        };
    }
    
    private long Displacement(long mod, long rm) => mod switch
    {
        0b00 when rm == 0b110 => Get(Bits.DispLo) | (Get(Bits.DispHi) << 8),
        0b01 => SignExtend8(Get(Bits.DispLo)),
        0b10 => Get(Bits.DispLo) | (Get(Bits.DispHi) << 8),
        _ => 0
    };
    
    private static long SignExtend8(long value)
        => value >= 0x80 ? value - 0x100 : value;
    
}