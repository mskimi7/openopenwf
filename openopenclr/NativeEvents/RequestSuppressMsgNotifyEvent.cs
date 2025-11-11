using Newtonsoft.Json;

namespace openopenclr.NativeEvents
{
    internal class RequestSuppressMsgNotifyEvent : NativeEvent
    {
        private readonly bool shouldSuppress;

        internal override NativeEventId Id => NativeEventId.RequestSuppressMsgNotify;

        internal override string GetAsJsonSerialized()
        {
            return JsonConvert.SerializeObject(new
            {
                shouldSuppress = shouldSuppress
            });
        }

        internal RequestSuppressMsgNotifyEvent(bool shouldSuppress)
        {
            this.shouldSuppress = shouldSuppress;
        }
    }
}
