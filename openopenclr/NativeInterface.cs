using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using Newtonsoft.Json.Linq;
using openopenclr.NativeEvents;

namespace openopenclr
{
    internal static class NativeInterface
    {
        private delegate void FreeNativeMemoryDelegate(IntPtr ptr);
        private static FreeNativeMemoryDelegate FreeNativeMemory;

        internal delegate void LogToConsoleDelegate([MarshalAs(UnmanagedType.LPStr)] string msg);
        internal static LogToConsoleDelegate LogToConsole;

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
                        JObject eventData = JObject.Parse(Encoding.UTF8.GetString(br.ReadBytes((int)(ms.Length - ms.Position))));

                        switch (eventId)
                        {
                            case NativeEventId.ResponseTypeList:
                                return ResponseTypeListEvent.Deserialize(eventData);
                            case NativeEventId.ResponseTypeInfo:
                                return ResponseTypeInfoEvent.Deserialize(eventData);
                            case NativeEventId.ResponseShowInspector:
                                return ResponseShowInspectorEvent.Deserialize(eventData);
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
                    bw.Write((byte)evt.Id);
                    bw.Write(Encoding.UTF8.GetBytes(evt.GetAsJsonSerialized()));
                }

                byte[] arr = ms.ToArray();
                SendNativeEventRaw(arr, (ulong)arr.LongLength);
            }
        }

        internal static void RequestTypeList(bool requestAll)
        {
            SendNativeEvent(new RequestTypeListEvent(requestAll));
        }

        internal static void RequestTypeInfo(string typeName)
        {
            SendNativeEvent(new RequestTypeInfoEvent(typeName));
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
                LogToConsole = Marshal.GetDelegateForFunctionPointer<LogToConsoleDelegate>(new IntPtr(long.Parse(pointers[1])));
                GetNativeEventRaw = Marshal.GetDelegateForFunctionPointer<GetNativeEventRawDelegate>(new IntPtr(long.Parse(pointers[2])));
                SendNativeEventRaw = Marshal.GetDelegateForFunctionPointer<SendNativeEventRawDelegate>(new IntPtr(long.Parse(pointers[3])));
            }
            catch (IndexOutOfRangeException)
            {
                MessageBox.Show("Dear Developer, you forgot to add a new function pointer to the native DLL!!! Consult the InitCLR function.",
                    "Whooops", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
