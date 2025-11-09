using System.IO;

namespace openopenclr.NativeEvents
{
    internal class RequestSuppressMsgNotifyEvent : NativeEvent
    {
        private readonly bool shouldSuppress;

        internal override NativeEventId Id => NativeEventId.RequestSuppressMsgNotify;

        internal override void Serialize(BinaryWriter writer)
        {
            base.Serialize(writer);
            writer.Write(shouldSuppress);
        }

        internal RequestSuppressMsgNotifyEvent(bool shouldSuppress)
        {
            this.shouldSuppress = shouldSuppress;
        }
    }
}
