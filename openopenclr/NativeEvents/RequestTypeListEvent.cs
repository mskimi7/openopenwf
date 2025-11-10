using System.IO;

namespace openopenclr.NativeEvents
{
    internal class RequestTypeListEvent : NativeEvent
    {
        private readonly bool requestAllTypes;

        internal override NativeEventId Id => NativeEventId.RequestTypeList;

        internal override void Serialize(BinaryWriter writer)
        {
            base.Serialize(writer);
            writer.Write(requestAllTypes);
        }

        internal RequestTypeListEvent(bool requestAllTypes)
        {
            this.requestAllTypes = requestAllTypes;
        }
    }
}
