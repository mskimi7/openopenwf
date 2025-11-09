using System.Threading;
using System.Windows.Forms;
using openopenclr.NativeEvents;

namespace openopenclr
{
    public static class Program
    {
        private static Inspector InspectorForm;

        static void FormThread()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            InspectorForm = new Inspector();
            Application.Run(InspectorForm);
        }

        static void HandleEvent(NativeEvent evt)
        {
            switch (evt.Id)
            {
                case NativeEventId.ResponseTypeList:
                    InspectorForm.OnTypeListReceived((ResponseTypeListEvent)evt);
                    break;
            }
        }

        static void ManagedThread()
        {
            for (; ;)
            {
                NativeEvent evt = NativeInterface.GetNativeEvent();
                if (evt != null)
                    HandleEvent(evt);
                else
                    Thread.Sleep(20);
            }
        }

        static int Main(string nativePointers)
        {
            NativeInterface.Initialize(nativePointers);

            new Thread(FormThread).Start();
            while (InspectorForm == null || !InspectorForm.IsFormReady.WaitOne())
                Thread.Sleep(15); // wait until form is ready

            new Thread(ManagedThread).Start();
            return 0;
        }
    }
}
