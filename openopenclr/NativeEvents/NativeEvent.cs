using System;
using System.IO;
using System.Text;

namespace openopenclr.NativeEvents
{
    internal enum NativeEventId
    {
        RequestTypeList = 0,
        ResponseTypeList = 1,
        RequestTypeInfo = 2,
        ResponseTypeInfo = 3,
        RequestSuppressMsgNotify = 4
    }

    internal abstract class NativeEvent
    {
        internal abstract NativeEventId Id { get; }
        internal virtual void Serialize(BinaryWriter writer)
        {
            writer.Write((byte)Id);
        }
    }

    public static class BinaryStreamExtensions
    {
        public static string ReadInt32PrefixedString(this BinaryReader br)
        {
            int length = br.ReadInt32();
            return Encoding.ASCII.GetString(br.ReadBytes(length));
        }

        public static void WriteInt32PrefixedString(this BinaryWriter bw, string s)
        {
            bw.Write(s.Length);
            bw.Write(Encoding.ASCII.GetBytes(s));
        }
    }
}
