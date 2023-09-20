using System;
using System.Collections;
using System.Collections.Generic;

namespace OSBridge.Windows
{
    public sealed class FileOpenDialogResults : ICollection<Uri>, IEnumerable<Uri>, IDisposable
    {
        private IntPtr _hResults;

        private int _count = -1;
        private List<Uri> _resultsCache;

        public int Count
        {
            get
            {
                EnsureCount();
                return _count;
            }
        }

        public bool IsReadOnly => true;

        public FileOpenDialogResults(IntPtr hResults)
        {
            _hResults = hResults;
        }

        ~FileOpenDialogResults() { Dispose(false); }

        public void Dispose() { Dispose(true); }

        public IEnumerator<Uri> GetEnumerator()
        {
            return new Enumerator(this);
        }

        public void Add(Uri item)
        {
            throw new NotSupportedException();
        }

        public void Clear()
        {
            throw new NotSupportedException();
        }

        public bool Contains(Uri item)
        {
            FetchUris();
            return _resultsCache?.Contains(item) ?? false;
        }

        public void CopyTo(Uri[] array, int arrayIndex)
        {
            FetchUris();
            _resultsCache?.CopyTo(array, arrayIndex);
        }

        public bool Remove(Uri item)
        {
            throw new NotSupportedException();
        }

        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

        internal bool FetchUris(int maxCount = int.MaxValue)
        {
            EnsureCount();
            if (maxCount > _count) maxCount = _count;
            if (maxCount <= 0) return false;

            _resultsCache ??= new List<Uri>();
            while (_resultsCache.Count < maxCount)
            {
                var uriRaw = API.GetResultUrlAt(_hResults, _resultsCache.Count);
                if (!Uri.TryCreate(uriRaw, UriKind.Absolute, out var uri))
                    return false;

                _resultsCache.Add(uri);
            }

            return true;
        }

        private void Dispose(bool disposing)
        {
            API.RecycleResults(_hResults);
            _hResults = IntPtr.Zero;
        }

        private void EnsureCount()
        {
            if (_count < 0)
                _count = API.GetResultsCount(_hResults);
        }

        private bool MoveTo(int index)
        {
            EnsureCount();

            return index < _count && FetchUris(index + 1);
        }

        private class Enumerator : IEnumerator<Uri>
        {
            private readonly FileOpenDialogResults _host;

            private int _i;

            public Enumerator(FileOpenDialogResults host)
            {
                _host = host;
                Reset();
            }

            public Uri Current => _host._resultsCache[_i];

            object IEnumerator.Current => Current;

            public void Dispose()
            {
            }

            public bool MoveNext()
            {
                if (!_host.MoveTo(_i + 1))
                {
                    _i = _host._count;
                    return false;
                }

                ++_i;
                return true;
            }

            public void Reset()
            {
                _i = -1;
            }
        }
    }
}
