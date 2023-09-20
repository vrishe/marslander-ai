using System;
using System.Runtime.InteropServices;

namespace OSBridge.Windows
{
    internal class FileOpenDialogParamsMarshaler : ICustomMarshaler
    {
        public static ICustomMarshaler GetInstance(string s)
        {
            return new FileOpenDialogParamsMarshaler();
        }

        public void CleanUpManagedData(object o)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            var fodpn = Marshal.PtrToStructure<FileOpenDialogParamsNative_>(pNativeData);
            Marshal.FreeHGlobal(pNativeData);

            Marshal.FreeHGlobal(fodpn.pClientGuid);
            Marshal.FreeHGlobal(fodpn.pszDefaultFolder);
            Marshal.FreeHGlobal(fodpn.pszFileName);
            Marshal.FreeHGlobal(fodpn.pszTitle);
            Marshal.FreeHGlobal(fodpn.pszOkButtonLabel);
            Marshal.FreeHGlobal(fodpn.pszFileNameFieldLabel);

            IntPtr[] rgFilterSpec = new IntPtr[fodpn.cFileTypes];
            Marshal.Copy(fodpn.rgFilterSpec, rgFilterSpec, 0, rgFilterSpec.Length);
            Marshal.FreeHGlobal(fodpn.rgFilterSpec);
            for (var i = 0; i < rgFilterSpec.Length; ++i)
                Marshal.FreeHGlobal(rgFilterSpec[i]);
        }

        public int GetNativeDataSize()
        {
            return Marshal.SizeOf<FileOpenDialogParamsNative_>();
        }

        public IntPtr MarshalManagedToNative(object obj)
        {
            var fodp = obj as FileOpenDialogParams;

            IntPtr pClientGuid = IntPtr.Zero;
            if (fodp.pClientGuid.HasValue)
            {
                pClientGuid = Marshal.AllocHGlobal(Marshal.SizeOf<Guid>());
                Marshal.StructureToPtr(fodp.pClientGuid, pClientGuid, false);
            }

            IntPtr rgFilterSpec = IntPtr.Zero;
            if (fodp.rgFilterSpec != null)
            {
                IntPtr[] strings = new IntPtr[fodp.rgFilterSpec.Length];
                for (var i = 0; i < strings.Length; ++i)
                    strings[i] = Marshal.StringToHGlobalUni(fodp.rgFilterSpec[i]);

                if (strings.Length > 0)
                {
                    rgFilterSpec = Marshal.AllocHGlobal(strings.Length * Marshal.SizeOf<IntPtr>());
                    Marshal.Copy(strings, 0, rgFilterSpec, strings.Length);
                }
            }

            var fodpn = new FileOpenDialogParamsNative_
            {
                bClearClientData = fodp.bClearClientData,
                cFileTypes = (uint)(fodp.rgFilterSpec?.Length ?? 0),
                iFileType = (uint)fodp.iFileType,
                dwOptions = (uint)fodp.dwOptions,
                hwndOwner = fodp.hwndOwner,
                pClientGuid = pClientGuid,
                pszDefaultFolder = Marshal.StringToHGlobalUni(fodp.pszFolder),
                pszFileName = Marshal.StringToHGlobalUni(fodp.pszDefaultFileName),
                pszTitle = Marshal.StringToHGlobalUni(fodp.pszTitle),
                pszOkButtonLabel = Marshal.StringToHGlobalUni(fodp.pszOkButtonLabel),
                pszFileNameFieldLabel = Marshal.StringToHGlobalUni(fodp.pszFileNameFieldLabel),
                rgFilterSpec = rgFilterSpec,
            };

            var p = Marshal.AllocCoTaskMem(Marshal.SizeOf<FileOpenDialogParamsNative_>());
            Marshal.StructureToPtr(fodpn, p, false);

            return p;
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            throw new NotSupportedException();
        }

        [StructLayout(LayoutKind.Sequential)]
        struct FileOpenDialogParamsNative_
        {
            public bool bClearClientData;
            public uint cFileTypes;
            public uint iFileType;
            public uint dwOptions;
            public IntPtr hwndOwner;
            public IntPtr pClientGuid;
            public IntPtr pszDefaultFolder;
            public IntPtr pszFileName;
            public IntPtr pszTitle;
            public IntPtr pszOkButtonLabel;
            public IntPtr pszFileNameFieldLabel;
            public IntPtr rgFilterSpec;
        }
    }
}
