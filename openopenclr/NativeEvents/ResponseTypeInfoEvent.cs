using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace openopenclr.NativeEvents
{
    internal class ResponseTypeInfoEvent : NativeEvent
    {
        internal string ErrorMessage { get; set; } = "";
        internal string PropertyText { get; set; } = "";
        internal List<string> InheritanceChain { get; set; } = new List<string>();

        internal bool IsError => !string.IsNullOrEmpty(ErrorMessage);

        internal override NativeEventId Id => NativeEventId.ResponseTypeInfo;

        internal static ResponseTypeInfoEvent Deserialize(JObject jsonObject)
        {
            string errorMsg = jsonObject["error"].ToObject<string>();
            if (!string.IsNullOrEmpty(errorMsg))
                return new ResponseTypeInfoEvent { ErrorMessage = errorMsg };

            return new ResponseTypeInfoEvent
            {
                ErrorMessage = errorMsg,
                PropertyText = jsonObject["propertyText"].ToObject<string>(),
                InheritanceChain = jsonObject["parentTypes"].ToObject<List<string>>()
            };
        }
    }
}
