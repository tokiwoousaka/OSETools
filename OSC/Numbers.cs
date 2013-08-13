using System;
using System.Collections.Generic;
using System.Collections;
using System.Linq;
using System.Text;
using Alice.Extensions;

namespace OSC
{
    public class Numbers<T> : IEnumerable<T>
    {
        Dictionary<T, bool> data;
        Stack<Numbers<T>> local;

        public IEnumerable<T> Locked
        {
            get
            {
                return data.Where(x => x.Value).Select(x => x.Key);
            }
        }

        public Numbers(int start, int count, Func<int,T> caster)
        {
            data = Enumerable.Range(start, count).Select(caster).ToDictionary(x => x, _ => false);
            local = new Stack<Numbers<T>>();
        }

        public Numbers(Numbers<T> origin)
        {
            data = new Dictionary<T,bool>(origin.data);
            local = new Stack<Numbers<T>>(origin.local);
        }

        public T Lock()
        {
            var i = data.First(x => !x.Value).Key;
            data[i] = true;
            return i;
        }

        public void Free(params T[] indexes)
        {
            Free((IEnumerable<T>)indexes);
        }

        public void Free(IEnumerable<T> indexes)
        {
            indexes.ToArray().ForEach(x => data[x] = false);
        }

        public void Reset()
        {
            Free(data.Keys);
        }

        public void Local()
        {
            local.Push(new Numbers<T>(this));
            Reset();
        }

        public void Unlocal()
        {
            var x = local.Pop();
            this.data = x.data;
            this.local = x.local;
        }

        public IEnumerator<T> GetEnumerator()
        {
            while (data.Any(x => !x.Value))
                yield return Lock();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            while (data.Any(x => !x.Value))
                yield return Lock();
        }
    }
}
