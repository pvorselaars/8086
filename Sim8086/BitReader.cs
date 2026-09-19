namespace Sim8086;

public class BitReader(byte[] memory, int byteOffset = 0)
{
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

    public int BitsRead => _bitOffset;
    public int BytesRead => (_bitOffset + 7) / 8;
}