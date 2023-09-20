using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace OSBridge.Windows
{
    public sealed class FileOpenDialog
    {
        private readonly List<string> _filterSpec = new();

        private bool _clientClear;
        private bool _multi;
        private Guid? _clientGuid;
        private string _folderPath;
        private string _okButtonLabel;
        private string _title;

        public FileOpenDialog SetDefaultFolder(string path)
        {
            if (Directory.Exists(path))
                _folderPath = path;
            else if (File.Exists(path))
                _folderPath = Path.GetDirectoryName(path);

            return this;
        }

        public FileOpenDialog SetFilters(params string[] filterSpecs)
        {
            _filterSpec.Clear();
            if (filterSpecs != null)
                _filterSpec.AddRange(filterSpecs);
            return this;
        }

        public FileOpenDialog SetMultipleSelection(bool multiSelect)
        {
            _multi = multiSelect;
            return this;
        }

        public FileOpenDialog WithClientId(Guid id, bool clearData = false)
        {
            _clientGuid = id;
            _clientClear = clearData;
            return this;
        }

        public FileOpenDialog WithFilter(string filterSpec)
        {
            _filterSpec.Add(filterSpec);
            return this;
        }

        public FileOpenDialog WithFilter(string description, string extensions)
        {
            _filterSpec.Add($"{description} ({extensions.Trim('(', ')')})");
            return this;
        }

        public FileOpenDialog WithFilterComposite(string description, params string[] extensions)
        {
            if (extensions is { Length: > 0 })
                _filterSpec.Add($"{description} ({string.Join("; ", extensions)})");
            else
                _filterSpec.Add($"{description} (*.*)");
            return this;
        }
        public FileOpenDialog WithMultipleSelection()
        {
            _multi = true;
            return this;
        }

        public FileOpenDialog WithOkButtonLabel(string label)
        {
            _okButtonLabel = label;
            return this;
        }

        public FileOpenDialog WithTitle(string title)
        {
            _title = title;
            return this;
        }

        public FileOpenDialogResults Show() => Show(MakeParams());

        public Task<FileOpenDialogResults> ShowAsync() => ShowAsync(MakeParams());

        private static FileOpenDialogResults Show(FileOpenDialogParams p)
        {
            var hResults = IntPtr.Zero;
            var errc = API.FileOpenDialog(p, out hResults);
            if (errc != 0) Debug.WriteLine($"{nameof(FileOpenDialog)}.{nameof(Show)} call failed (HRESULT: 0x{errc:X08})");
            return new FileOpenDialogResults(hResults);
        }

        private static Task<FileOpenDialogResults> ShowAsync(FileOpenDialogParams p)
        {
            var tcs = new TaskCompletionSource<FileOpenDialogResults>();

            new Thread(() =>
            {
                var coInit = true;
                var errc = API.CoInitialize(IntPtr.Zero);
                if (errc < 0 && (uint)errc != 0x80010106 /* RPC_E_CHANGED_MODE */)
                {
                    coInit = false;
                    Debug.WriteLine($"Failed to initialize COM library (HRESULT: 0x{errc:X08})");
                }

                try
                {
                    using var result = Show(p);
                    result.FetchUris();
                    tcs.SetResult(result);
                }
                finally
                {
                    if (coInit)
                        API.CoUninitialize();
                }
            }).Start();

            return tcs.Task;
        }

        private FileOpenDialogParams MakeParams()
        {
            return new FileOpenDialogParams
            {
                bClearClientData = _clientClear,
                iFileType = 1,
                dwOptions = _multi
                    ? FileOpenDialogOptions.ALLOWMULTISELECT
                    : FileOpenDialogOptions.NONE,
                hwndOwner = Process.GetCurrentProcess().MainWindowHandle,
                pClientGuid = _clientGuid,
                pszFolder = Directory.Exists(_folderPath)
                    ? Path.GetFullPath(_folderPath)
                    : null,
                pszTitle = _title,
                pszOkButtonLabel = _okButtonLabel,
                rgFilterSpec = _filterSpec.Count > 0 
                    ? _filterSpec.ToArray()
                    : new[] { "All Files (*.*)" },
            };
        }
    }
}
