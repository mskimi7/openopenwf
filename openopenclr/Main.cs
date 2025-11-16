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

            for (; ;)
            {
                if (Interlocked.CompareExchange(ref InspectorShowRequested, 0, 1) == 1)
                {
                    InspectorForm = new Inspector();
                    Application.Run(InspectorForm);
                    NativeInterface.SuppressTreeNodeEvents(false);
                    Interlocked.Exchange(ref InspectorShowRequested, 0);
                }
                else
                {
                    Thread.Sleep(50);
                }
            }
        }

        static void HandleEvent(NativeEvent evt)
        {
            Inspector form = InspectorForm; // store due to thread-safety

            switch (evt.Id)
            {
                case NativeEventId.ResponseTypeList:
                    form?.OnTypeListReceived((ResponseTypeListEvent)evt);
                    break;
                case NativeEventId.ResponseTypeInfo:
                    form?.OnTypeInfoReceived((ResponseTypeInfoEvent)evt);
                    break;
                case NativeEventId.ResponseShowInspector:
                    Interlocked.Exchange(ref InspectorShowRequested, 1);
                    break;
            }
        }

        static void EventThread()
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

            new Thread(FormThread).Start();
            new Thread(EventThread).Start();

            return 0;
        }
    }
}
