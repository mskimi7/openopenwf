using Newtonsoft.Json;

namespace openopenclr.NativeEvents
{
    internal class RequestTypeListEvent : NativeEvent
    {
        private readonly bool requestAllTypes;

        internal override NativeEventId Id => NativeEventId.RequestTypeList;

        internal override string GetAsJsonSerialized()
        {
            return JsonConvert.SerializeObject(new
            {
                fetchAllTypes = requestAllTypes
            });
        }

        internal RequestTypeListEvent(bool requestAllTypes)
        {
            this.requestAllTypes = requestAllTypes;
        }
    }
}
