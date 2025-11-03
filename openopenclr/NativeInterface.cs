using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using openopenclr.NativeEvents;

namespace openopenclr
{
    internal static class NativeInterface
    {
        private delegate IntPtr GetNativeEventRawDelegate(out int dataSize);
        private static GetNativeEventRawDelegate GetNativeEventRaw;

        private delegate void FreeNativeMemoryDelegate(IntPtr ptr);
        private static FreeNativeMemoryDelegate FreeNativeMemory;

        internal static unsafe NativeEvent GetNativeEvent()
        {
            IntPtr evtData = GetNativeEventRaw(out int dataSize);
            if (evtData == IntPtr.Zero)
                return null;

            try
            {
                using (var ms = new UnmanagedMemoryStream((byte*)evtData.ToPointer(), dataSize))
                {

                }
            }
            finally
            {
                FreeNativeMemory(evtData);
            }

            return null;
        }

        /// <summary>
        /// Initialize function pointers to the native openopenwf.dll. The function pointers are stored as a sequence of stringified addresses.
        /// </summary>
        internal static void Initialize(string nativePointers)
        {
            string[] pointers = nativePointers.Split(',');

            try
            {
                GetNativeEventRaw = Marshal.GetDelegateForFunctionPointer<GetNativeEventRawDelegate>(new IntPtr(long.Parse(pointers[0])));
                FreeNativeMemory = Marshal.GetDelegateForFunctionPointer<FreeNativeMemoryDelegate>(new IntPtr(long.Parse(pointers[1])));
            }
            catch (IndexOutOfRangeException)
            {
                MessageBox.Show("Dear Developer, you forgot to add a new function pointer to the native DLL!!! Consult the InitCLR function.",
                    "Whooops", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
