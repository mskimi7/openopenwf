using System;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json.Linq;

namespace openopenclr.NativeEvents
{
    internal class ResponseTypeInfoEvent : NativeEvent
    {
        internal string ErrorMessage { get; set; }
        internal Dictionary<uint, byte[]> PropertyTexts { get; set; } // key: property text acquisition flags
        internal List<string> InheritanceChain { get; set; }

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
                PropertyTexts = jsonObject["propertyTexts"].ToObject<Dictionary<uint, string>>().ToDictionary(kv => kv.Key, kv => Convert.FromBase64String(kv.Value)),
                InheritanceChain = jsonObject["parentTypes"].ToObject<List<string>>()
            };
        }
    }
}
