namespace Sim8086;

public class Instruction(Dictionary<Bits, long> bits)
{
    public string Operation = "None";
    public int Size = 0;

    private string[][] registers =
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

    public string Print()
    {
        var d = Get(Bits.D);
        var w = Get(Bits.W);
        var reg = Get(Bits.Reg);
        var rm = Get(Bits.Rm);
        var mod = Get(Bits.Mod);

        var regOperand = registers[reg][w];
        var rmOperand = registers[rm][w];

        var (dst, src) = d == 0 ? (regOperand, rmOperand) : (rmOperand, regOperand);

        return $"{Operation} {src}, {dst}\n";
    }
}