using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using openopenclr.NativeEvents;

namespace openopenclr
{
    internal static class NativeInterface
    {
        private delegate void FreeNativeMemoryDelegate(IntPtr ptr);
        private static FreeNativeMemoryDelegate FreeNativeMemory;

        private delegate IntPtr GetNativeEventRawDelegate(out ulong dataSize);
        private static GetNativeEventRawDelegate GetNativeEventRaw;

        private delegate void SendNativeEventRawDelegate([MarshalAs(UnmanagedType.LPArray)] byte[] data, ulong dataSize);
        private static SendNativeEventRawDelegate SendNativeEventRaw;

        internal static unsafe NativeEvent GetNativeEvent()
        {
            IntPtr evtData = GetNativeEventRaw(out ulong dataSize);
            if (evtData == IntPtr.Zero)
                return null;
            
            try
            {
                using (var ms = new UnmanagedMemoryStream((byte*)evtData.ToPointer(), (long)dataSize))
                {
                    using (var br = new BinaryReader(ms))
                    {
                        NativeEventId eventId = (NativeEventId)br.ReadByte();
                        Console.WriteLine($"Receiving a {eventId}");
                        switch (eventId)
                        {
                            case NativeEventId.ResponseTypeList:
                                return ResponseTypeListEvent.Deserialize(br);
                        }
                    }
                }
            }
            finally
            {
                FreeNativeMemory(evtData);
            }

            return null;
        }

        private static void SendNativeEvent(NativeEvent evt)
        {
            using (var ms = new MemoryStream())
            {
                using (var bw = new BinaryWriter(ms))
                {
                    evt.Serialize(bw);
                }

                byte[] arr = ms.ToArray();
                SendNativeEventRaw(arr, (ulong)arr.LongLength);
            }
        }

        internal static void RequestTypeListRefresh()
        {
            SendNativeEvent(new RequestTypeListEvent());
        }

        internal static void SuppressTreeNodeEvents(bool shouldSuppress)
        {
            SendNativeEvent(new RequestSuppressMsgNotifyEvent(shouldSuppress));
        }

        /// <summary>
        /// Initialize function pointers to the native openopenwf.dll. The function pointers are stored as a sequence of stringified addresses.
        /// </summary>
        internal static void Initialize(string nativePointers)
        {
            string[] pointers = nativePointers.Split(',');

            try
            {
                FreeNativeMemory = Marshal.GetDelegateForFunctionPointer<FreeNativeMemoryDelegate>(new IntPtr(long.Parse(pointers[0])));
                GetNativeEventRaw = Marshal.GetDelegateForFunctionPointer<GetNativeEventRawDelegate>(new IntPtr(long.Parse(pointers[1])));
                SendNativeEventRaw = Marshal.GetDelegateForFunctionPointer<SendNativeEventRawDelegate>(new IntPtr(long.Parse(pointers[2])));
            }
            catch (IndexOutOfRangeException)
            {
                MessageBox.Show("Dear Developer, you forgot to add a new function pointer to the native DLL!!! Consult the InitCLR function.",
                    "Whooops", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
