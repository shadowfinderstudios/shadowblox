using System.Text;
using System.Text.Json;

namespace MasterServer.Utils;

/// <summary>
/// Helper for serializing/deserializing packets
/// </summary>
public static class PacketSerializer
{
    /// <summary>
    /// Write string to byte array
    /// </summary>
    public static void WriteString(List<byte> buffer, string str)
    {
        var bytes = Encoding.UTF8.GetBytes(str);
        WriteUInt16(buffer, (ushort)bytes.Length);
        buffer.AddRange(bytes);
    }

    /// <summary>
    /// Read string from byte array
    /// </summary>
    public static string ReadString(byte[] data, ref int offset)
    {
        var length = ReadUInt16(data, ref offset);
        var str = Encoding.UTF8.GetString(data, offset, length);
        offset += length;
        return str;
    }

    /// <summary>
    /// Write uint16 (2 bytes, big endian)
    /// </summary>
    public static void WriteUInt16(List<byte> buffer, ushort value)
    {
        buffer.Add((byte)(value >> 8));
        buffer.Add((byte)(value & 0xFF));
    }

    /// <summary>
    /// Read uint16 (2 bytes, big endian)
    /// </summary>
    public static ushort ReadUInt16(byte[] data, ref int offset)
    {
        var value = (ushort)((data[offset] << 8) | data[offset + 1]);
        offset += 2;
        return value;
    }

    /// <summary>
    /// Write uint32 (4 bytes, big endian)
    /// </summary>
    public static void WriteUInt32(List<byte> buffer, uint value)
    {
        buffer.Add((byte)(value >> 24));
        buffer.Add((byte)((value >> 16) & 0xFF));
        buffer.Add((byte)((value >> 8) & 0xFF));
        buffer.Add((byte)(value & 0xFF));
    }

    /// <summary>
    /// Read uint32 (4 bytes, big endian)
    /// </summary>
    public static uint ReadUInt32(byte[] data, ref int offset)
    {
        var value = (uint)((data[offset] << 24) | (data[offset + 1] << 16) |
                           (data[offset + 2] << 8) | data[offset + 3]);
        offset += 4;
        return value;
    }

    /// <summary>
    /// Write int32 (4 bytes, big endian)
    /// </summary>
    public static void WriteInt32(List<byte> buffer, int value)
    {
        WriteUInt32(buffer, (uint)value);
    }

    /// <summary>
    /// Read int32 (4 bytes, big endian)
    /// </summary>
    public static int ReadInt32(byte[] data, ref int offset)
    {
        return (int)ReadUInt32(data, ref offset);
    }

    /// <summary>
    /// Write bool (1 byte)
    /// </summary>
    public static void WriteBool(List<byte> buffer, bool value)
    {
        buffer.Add((byte)(value ? 1 : 0));
    }

    /// <summary>
    /// Read bool (1 byte)
    /// </summary>
    public static bool ReadBool(byte[] data, ref int offset)
    {
        return data[offset++] != 0;
    }

    /// <summary>
    /// Write double (8 bytes)
    /// </summary>
    public static void WriteDouble(List<byte> buffer, double value)
    {
        buffer.AddRange(BitConverter.GetBytes(value));
    }

    /// <summary>
    /// Read double (8 bytes)
    /// </summary>
    public static double ReadDouble(byte[] data, ref int offset)
    {
        var value = BitConverter.ToDouble(data, offset);
        offset += 8;
        return value;
    }

    /// <summary>
    /// Write JSON object as string
    /// </summary>
    public static void WriteJson<T>(List<byte> buffer, T obj)
    {
        var json = JsonSerializer.Serialize(obj);
        WriteString(buffer, json);
    }

    /// <summary>
    /// Read JSON object from string
    /// </summary>
    public static T? ReadJson<T>(byte[] data, ref int offset)
    {
        var json = ReadString(data, ref offset);
        return JsonSerializer.Deserialize<T>(json);
    }

    /// <summary>
    /// Create length-prefixed packet (like TCP handler)
    /// </summary>
    public static byte[] CreatePacket(List<byte> payload)
    {
        var packet = new List<byte>();
        WriteUInt32(packet, (uint)payload.Count);
        packet.AddRange(payload);
        return packet.ToArray();
    }
}
