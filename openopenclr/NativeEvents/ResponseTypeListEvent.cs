using System.Collections.Generic;
using System.IO;
using System.Text;

namespace openopenclr.NativeEvents
{
    internal class ResponseTypeListEvent : NativeEvent
    {
        internal List<string> AllTypes { get; } = new List<string>();

        internal override NativeEventId Id => NativeEventId.ResponseTypeList;

        internal static ResponseTypeListEvent Deserialize(BinaryReader reader)
        {
            ResponseTypeListEvent result = new ResponseTypeListEvent();
            
            int typeCount = reader.ReadInt32();

            for (long i = 0; i < typeCount; ++i)
            {
                int stringSize = reader.ReadInt32();
                result.AllTypes.Add(Encoding.ASCII.GetString(reader.ReadBytes(stringSize)));
            }

            return result;
        }
    }
}
