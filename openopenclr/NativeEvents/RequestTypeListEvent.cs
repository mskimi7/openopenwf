using System.IO;

namespace openopenclr.NativeEvents
{
    internal class RequestTypeListEvent : NativeEvent
    {
        internal override NativeEventId Id => NativeEventId.RequestTypeList;

        internal override void Serialize(BinaryWriter writer)
        {
            base.Serialize(writer);
        }
    }
}
