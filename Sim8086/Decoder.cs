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
}

public static class Decoder
{
    record Field(Bits Bits, int Size, uint Value = 0);

    record Encoding(string Op, params Field[] Fields);
    
    static Encoding E(string op,  params Field[] fields) => new(op, fields);
    static Field L(uint value) => new(Bits.Literal, value == 0 ? 1 : 32 - BitOperations.LeadingZeroCount(value), value);
    
    static Field D => new (Bits.D, 1);
    static Field W => new (Bits.W, 1);
    static Field Mod => new (Bits.Mod, 2);
    static Field Reg => new (Bits.Reg, 3);
    static Field Rm => new (Bits.Rm, 3);

    private static readonly Encoding[] Encodings =
    [
        E("mov", L(0b100010), D, W, Mod, Reg, Rm)
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
                if (field.Bits == Bits.Literal && value != field.Value)
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
                Size = reader.BytesRead
            };
        }

        return null;
    }
}