namespace Sim8086;

public class Program
{
    private static readonly byte[] Memory = new byte[1024*1024];
    public static async Task<int> Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Usage: Sim8086 <input file>");
            return 1;
        }
        
        var inputFile = File.OpenRead(args[0]);
        if (!inputFile.CanRead)
        {
            Console.WriteLine("Can't read input file");
            return 2;
        }

        var readCount = await inputFile.ReadAsync(Memory, 0, Memory.Length);
        inputFile.Close();
        Console.WriteLine($"Read {readCount} bytes into memory");
        
        var outputFilename = Path.ChangeExtension(args[0], ".i");
        if (args.Length == 2)
            outputFilename = args[1];
        
        var instructions = Decoder.Decode(Memory);
        
        var lines = new[]
        {
            "bits 16"
        }.Concat(instructions.Select(i => i.Print()));

        await File.WriteAllLinesAsync(outputFilename, lines);
        
        return 0;
    }
}