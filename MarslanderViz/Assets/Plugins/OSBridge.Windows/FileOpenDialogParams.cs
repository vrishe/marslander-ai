using System;

namespace OSBridge.Windows
{
    internal class FileOpenDialogParams
    {
        public bool bClearClientData;
        public int iFileType;
        public FileOpenDialogOptions dwOptions;
        public IntPtr hwndOwner;
        public Guid? pClientGuid;
        public string pszFolder;
        public string pszDefaultFileName;
        public string pszTitle;
        public string pszOkButtonLabel;
        public string pszFileNameFieldLabel;
        public string[] rgFilterSpec;
    }
}
