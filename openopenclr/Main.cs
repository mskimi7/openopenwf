using System.Threading;
using System.Windows.Forms;
using openopenclr.NativeEvents;

namespace openopenclr
{
    public static class Program
    {
        private static Inspector InspectorForm;
        private static int InspectorShowRequested;

        static void FormThread()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            Interlocked.Exchange(ref InspectorForm, new Inspector());
            for (; ;)
            {
                if (Interlocked.CompareExchange(ref InspectorShowRequested, 0, 1) == 1)
                    Application.Run(InspectorForm);
                else
                    Thread.Sleep(15);
            }
        }

        static void HandleEvent(NativeEvent evt)
        {
            switch (evt.Id)
            {
                case NativeEventId.ResponseTypeList:
                    InspectorForm.OnTypeListReceived((ResponseTypeListEvent)evt);
                    break;
                case NativeEventId.ResponseTypeInfo:
                    InspectorForm.OnTypeInfoReceived((ResponseTypeInfoEvent)evt);
                    break;
                case NativeEventId.ResponseShowInspector:
                    Interlocked.Exchange(ref InspectorShowRequested, 1);
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
                    Thread.Sleep(15);
            }
        }

        static int Main(string nativePointers)
        {
            NativeInterface.Initialize(nativePointers);

            // It's important to wait for all the managed DLLs to load before Warframe's entry point executes for... reasons...
            new Thread(FormThread).Start();
            while (InspectorForm == null || !InspectorForm.IsFormReady.WaitOne())
                Thread.Sleep(15); // wait until form is ready

            new Thread(ManagedThread).Start();
            return 0;
        }
    }
}
