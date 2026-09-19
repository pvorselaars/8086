using System.Diagnostics;
using Sim8086;

namespace Tests;

public class Decoder(ITestOutputHelper output)
{
    public static IEnumerable<object[]> TestCases() => Directory.EnumerateFiles("Input", "*.asm").Select(x => new object[] { x });
    
    [Theory]
    [MemberData(nameof(TestCases))]
    public async Task Decodes(string inputFile)
    {
        var binaryFile = Path.ChangeExtension(inputFile, "bin");
        var intermediateFile = Path.ChangeExtension(inputFile, ".s");
        var comparisonFile =  Path.ChangeExtension(inputFile, ".compare");

        try
        {
            await Assemble(inputFile, binaryFile);
            
            var exitCode = await Program.Main([binaryFile, intermediateFile]);
            Assert.Equal(0, exitCode);
            
            await Assemble(intermediateFile, comparisonFile);
        
            var expected = await File.ReadAllBytesAsync(
                binaryFile,
                TestContext.Current.CancellationToken);
            
            var actual = await File.ReadAllBytesAsync(
                comparisonFile,
                TestContext.Current.CancellationToken);

            if (!actual.SequenceEqual(expected))
            {
                var actualHex = string.Join(
                    Environment.NewLine,
                    actual
                        .Chunk(16)
                        .Select(chunk => string.Join(" ", chunk.Select(b => $"{b:X2}"))));
                var expectedHex = string.Join(
                    Environment.NewLine,
                    expected
                        .Chunk(16)
                        .Select(chunk => string.Join(" ", chunk.Select(b => $"{b:X2}"))));
                

                output.WriteLine($"Expected:\n{expectedHex}\n");
                output.WriteLine($"Actual:\n{actualHex}");
            }

            Assert.Equal(expected, actual);
            
        }
        finally
        {
            File.Delete(binaryFile);
            File.Delete(intermediateFile);
            File.Delete(comparisonFile);
        }
    }

    private async Task Assemble(string inputFile, string outputFile)
    {
        var psi = new ProcessStartInfo
        {
            FileName = "nasm",
            Arguments = $"\"{inputFile}\" -o {outputFile}",
            UseShellExecute = false
        };

        using var process = Process.Start(psi)!;

        await process.WaitForExitAsync(TestContext.Current.CancellationToken);

        Assert.Equal(0, process.ExitCode);
    }
}