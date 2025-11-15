using Newtonsoft.Json.Linq;

namespace openopenclr.NativeEvents
{
    internal class ResponseShowInspectorEvent : NativeEvent
    {
        internal override NativeEventId Id => NativeEventId.ResponseShowInspector;

        internal static ResponseShowInspectorEvent Deserialize(JObject jsonObject)
        {
            return new ResponseShowInspectorEvent();
        }
    }
}
