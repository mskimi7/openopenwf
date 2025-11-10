using System.IO;

namespace openopenclr.NativeEvents
{
    internal class RequestTypeInfoEvent : NativeEvent
    {
        private readonly string typeName;

        internal override NativeEventId Id => NativeEventId.RequestTypeInfo;

        internal override void Serialize(BinaryWriter writer)
        {
            base.Serialize(writer);
            writer.WriteInt32PrefixedString(typeName);
        }

        internal RequestTypeInfoEvent(string typeName)
        {
            this.typeName = typeName;
        }
    }
}
