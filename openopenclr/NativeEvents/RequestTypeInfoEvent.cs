using Newtonsoft.Json;

namespace openopenclr.NativeEvents
{
    internal class RequestTypeInfoEvent : NativeEvent
    {
        private readonly string typeName;

        internal override NativeEventId Id => NativeEventId.RequestTypeInfo;

        internal override string GetAsJsonSerialized()
        {
            return JsonConvert.SerializeObject(new
            {
                typeName = typeName
            });
        }

        internal RequestTypeInfoEvent(string typeName)
        {
            this.typeName = typeName;
        }
    }
}
