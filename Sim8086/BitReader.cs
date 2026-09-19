namespace Sim8086;

public class BitReader(byte[] memory, int byteOffset = 0)
{
    private readonly int _startBitOffset = byteOffset * 8;
    private int _bitOffset = byteOffset * 8;

    public uint Read(int count)
    {
        uint result = 0;

        for (var i = 0; i < count; i++)
        {
            var byteOffset = _bitOffset / 8;
            var bitOffset = 7 - (_bitOffset % 8);

            var bit = (memory[byteOffset] >> bitOffset) & 1;

            result = (result << 1) | (uint)bit;

            _bitOffset++;
        }

        return result;
    }

    public int BitsRead => _bitOffset - _startBitOffset;
    public int BytesRead => (BitsRead + 7) / 8;
}