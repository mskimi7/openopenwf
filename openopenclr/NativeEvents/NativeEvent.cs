using System.IO;

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
}
