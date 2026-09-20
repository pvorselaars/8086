using System.Numerics;

namespace Sim8086;

public enum Bits
{
    Literal,
    D,
    W,
    Mod,
    Reg,
    Rm,
    DataLo,
    DataHi,
    DispLo,
    DispHi
}

public static class Decoder
{
    record Field(Bits Bits, int Size, int Value = -1);

    record Encoding(string Op, Instruction.Format Format, params Field[] Fields);
    
    static Encoding E(string op, Instruction.Format format, params Field[] fields) => new(op, format, fields);
    static Field L(uint value) => new(Bits.Literal, value == 0 ? 1 : 32 - BitOperations.LeadingZeroCount(value), (int)value);

    private static Field D(int value = -1) => new (Bits.D, 1, value);
    private static Field W(int value = -1) => new (Bits.W, 1, value);
    private static Field Mod(int value = -1) => new (Bits.Mod, 2, value);
    private static Field Reg(int value = -1) => new (Bits.Reg, 3, value);
    private static Field Rm(int value = -1) => new (Bits.Rm, 3, value);
    private static Field DataLo => new (Bits.DataLo, 8);
    private static Field DataHi => new (Bits.DataHi, 8);
    private static Field DispLo => new (Bits.DispLo, 8);
    private static Field DispHi => new (Bits.DispHi, 8);

    private static readonly Encoding[] Encodings =
    [
        E("mov", Instruction.Format.RegRm, L(0b100010), D(), W(), Mod(0b00), Reg(), Rm(0b110), DispLo, DispHi),
        E("mov", Instruction.Format.RegRm, L(0b100010), D(), W(), Mod(0b00), Reg(), Rm()),
        E("mov", Instruction.Format.RegRm, L(0b100010), D(), W(), Mod(0b01), Reg(), Rm(), DispLo),
        E("mov", Instruction.Format.RegRm, L(0b100010), D(), W(), Mod(0b10), Reg(), Rm(), DispLo, DispHi),
        E("mov", Instruction.Format.RegRm, L(0b100010), D(), W(), Mod(0b11), Reg(), Rm()),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(0), Mod(0b00), Reg(0b000), Rm(0b110), DispLo, DispHi, DataLo),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(0), Mod(0b00), Reg(0b000), Rm(), DataLo),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(0), Mod(0b01), Reg(0b000), Rm(), DispLo, DataLo),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(0), Mod(0b10), Reg(0b000), Rm(), DispLo, DispHi, DataLo),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(1), Mod(0b00), Reg(0b000), Rm(0b110), DispLo, DispHi, DataLo, DataHi),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(1), Mod(0b00), Reg(0b000), Rm(), DataLo, DataHi),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(1), Mod(0b01), Reg(0b000), Rm(), DispLo, DataLo, DataHi),
        E("mov", Instruction.Format.RmImmediate, L(0b1100011), W(1), Mod(0b10), Reg(0b000), Rm(), DispLo, DispHi, DataLo, DataHi),
        E("mov", Instruction.Format.RegImmediate, L(0b1011), W(0), Reg(), DataLo),
        E("mov", Instruction.Format.RegImmediate, L(0b1011), W(1), Reg(), DataLo, DataHi),
        E("mov", Instruction.Format.Acc, L(0b101000), D(), W(), DispLo, DispHi),
    ];
    
    public static List<Instruction> Decode(byte[] memory, int start = 0)
    {
        var offset = start;
        List<Instruction> instructions = [];

        while (offset < memory.Length)
        {
            var instruction = TryDecode(memory, offset);
            if (instruction is null)
                break;
            
            instructions.Add(instruction);
            
            offset += instruction.Size;
        }
        
        return instructions;
    }

    private static Instruction? TryDecode(byte[] memory, int offset)
    {
        
        foreach (var encoding in Encodings)
        {
            var reader = new BitReader(memory, offset);
            var bits = new Dictionary<Bits, long>();

            var matched = true;
            
            foreach (var field in encoding.Fields)
            {
                var value = reader.Read(field.Size);

                // TODO: add mask to encodings to test literal directly before instantiating BitReader
                if (field.Value != -1 && value != field.Value)
                {
                    matched = false;
                    break;
                }

                bits[field.Bits] = value;
            }
            
            if (!matched)
                continue;
            
            return new Instruction(bits)
            {
                Operation = encoding.Op,
                InstructionFormat = encoding.Format,
                Size = reader.BytesRead
            };
        }

        return null;
    }
}