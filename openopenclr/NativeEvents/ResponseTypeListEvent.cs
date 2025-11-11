using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace openopenclr.NativeEvents
{
    internal class ResponseTypeListEvent : NativeEvent
    {
        internal List<string> AllTypes { get; set; } = new List<string>();

        internal override NativeEventId Id => NativeEventId.ResponseTypeList;

        internal static ResponseTypeListEvent Deserialize(JObject jsonObject)
        {
            return new ResponseTypeListEvent
            {
                AllTypes = jsonObject["types"].ToObject<List<string>>()
            };
        }
    }
}
