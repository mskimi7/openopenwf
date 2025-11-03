namespace openopenclr
{
    public class Program
    {
        static int Main(string nativePointers)
        {
            NativeInterface.Initialize(nativePointers);
            return 0;
        }
    }
}
