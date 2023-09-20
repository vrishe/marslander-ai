using System;
using System.Runtime.InteropServices;

namespace OSBridge.Windows
{
    internal static partial class API
    {
        [DllImport("OSBridge_x64.dll")]
        public static extern long FileOpenDialog(
            [In, MarshalAs (UnmanagedType.CustomMarshaler,
                    MarshalTypeRef=typeof(FileOpenDialogParamsMarshaler))]
                FileOpenDialogParams dialogParams,
            [Out] out IntPtr hresults);

        [DllImport("OSBridge_x64.dll")]
        public static extern int GetResultsCount(IntPtr hresults);

        [DllImport("OSBridge_x64.dll")]
        [return: MarshalAs(UnmanagedType.LPWStr)]
        public static extern string GetResultUrlAt(IntPtr hresults, int index);

        [DllImport("OSBridge_x64.dll")]
        public static extern void RecycleResults(IntPtr hresults);

        [DllImport("ole32.dll")]
        public static extern int CoInitialize(IntPtr pvReserved);

        [DllImport("ole32.dll")]
        public static extern void CoUninitialize();
    }
}
